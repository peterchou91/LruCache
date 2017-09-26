/**
 * author:zhouqing
 */
#include <iostream>
#include <tbb/concurrent_hash_map.h>
#include <vector>
#include <memory>
#include <boost/thread/recursive_mutex.hpp>
#include "ConcurrentHashMap.h" 
#include <atomic>
#include <mutex>
namespace lrucache{
using std::chrono::system_clock;
using std::chrono::milliseconds;
int64_t currentTime(){
	auto start = system_clock::now();
	int64_t time = (std::chrono::duration_cast<milliseconds>(start.time_since_epoch())).count();
	return time;
}


template<typename k, typename v>
class LruCache {
public:
	typedef struct Node {
        k key;
		v data;
		int64_t accessTime;
		std::shared_ptr<Node> pre;
		std::shared_ptr<Node> next;
		Node (const k& key_, const v& data_):key(key_),data(data_),pre(nullptr),next(nullptr){
		}
		Node (){}
		~Node(){
		}

	}Node;
private:
	int capacity;
	int size;
	int64_t duration;
	boost::recursive_mutex mutex;
	ConcurrentHashMap<k, std::shared_ptr<Node>> map;
	ConcurrentHashMap<k, int64_t> logMap;
    ConcurrentHashMap<k, int64_t> insertlogMap;
	std::shared_ptr<Node> head;
    std::shared_ptr<Node> tail;
public:
	void insertHead(std::shared_ptr<Node> newNode) {
			auto temp = head->next;
			head->next = newNode;
			newNode->next = temp;
			temp->pre = newNode;
			newNode->pre = head;
    }
	
	void extract(std::shared_ptr<Node> node) {
		/*	
		if(nullptr == node){
			std::cout << "extract node is nullptr";
			return;
		}
		if (node->pre ==nullptr || node->next == nullptr) {
			std::cout << "cant not extract tail or head node! or the node is not in cache" << std::endl;
			return;
		}
		*/
		auto nextNode = node->next;
		auto preNode = node->pre;
		preNode->next = nextNode;
		nextNode->pre = preNode;
		node->pre = node->next = nullptr;

	}
	
	bool del(std::shared_ptr<Node> node, const std::string& sour = "not redolog") {
		if(nullptr == node){
			std::cout << "delete node is nullptr";
			return false;
		}
		if (node->pre ==nullptr || node->next == nullptr) {
			std::cout << "cant not delete tail or head node! or the node is not in cache, source:" << sour << std::endl;
			return false;
		}
		extract(node);
        std::cout << "del:" << node->data << std::endl;
        return true;
	}

    /**
     * delete last element from cache
     */
	void deleteLast() {
        auto key = tail->pre->key;
		if(del(tail->pre)){
            size--;
            map.erase(key);
        }
    }
	


    void updateNode(const k& key){
        std::shared_ptr<Node> valNode = map.get(key);
        auto ct = currentTime();
        valNode->accessTime = ct;
        extract(valNode);
        std::cout << "extract:" << valNode->key << std::endl;
        insertHead(valNode);
    }

    void reDoLog(ConcurrentHashMap<k, int64_t> logMap){
        auto itr = logMap.begin();
        auto end = logMap.end();
        for(; itr != end; itr++) {
            auto innerKey = itr->first;
            std::shared_ptr<Node> valNode = map.get(innerKey);
            if(valNode != nullptr) {
                if((itr->second == -1)&& del(valNode, "redolog")){
                    size--;
                    map.erase(innerKey);
                } else
                    updateNode(innerKey);
            }
            logMap.erase(itr->first);

        }
    }
public:
    explicit LruCache(int cap = 10,int64_t duration = 1000 * 10):capacity(cap){
        this->duration = duration;
        head = std::make_shared<Node>();
        tail = std::make_shared<Node>();
        head->next = tail;
        tail->pre = head;
        head->pre = nullptr;
        tail->next = nullptr;
    }
	void put(const std::vector<k>& keys, const std::vector<v>& vals) {
        boost::recursive_mutex::scoped_lock lock(mutex);
        size_t count = keys.size();
        reDoLog(logMap);
        for(int i = 0; i < count; i++) {
            const auto& key = keys[i];
            const auto& val = vals[i];
            std::cout << "put:" << key << std::endl;
            if (map.contains(key)) {
                updateNode(key);
            } else {
                std::cout << "size:" << size << " " << map.size() << std::endl;
                if (size >= capacity) {
                    std::cout << "put " << key << " need to del " << "size:" << size << std::endl;
                    deleteLast();
                }
                auto newNode = std::make_shared<Node>(key, val);
                newNode->accessTime = currentTime();
                insertHead(newNode);
                map.put(key, newNode);
                size++;
            }
        }

	}
	
	bool get(const k& key, v& val) {
		if(!map.contains(key)) return false;
		auto valNode = map.get(key);
        if(nullptr == valNode) {
            std::cout << "exists but get failed ! key:" << key;
            boost::recursive_mutex::scoped_lock lock(mutex);
            valNode = map.get(key);
        }
        if(nullptr == valNode) return false;
        auto currenttime = currentTime();
        if(valNode->accessTime + this->duration > currenttime) {
            logMap.put(key, -1);
            return false;
        }
		val = valNode->data;
		//todo record access log
		logMap.put(key,currenttime);
		return true;
	}
    int volume(){
        return this->size;
    }

    void printAll(){
        auto it = map.begin();
        auto end = map.end();
        for(; it != end; it++){
            std::cout << it->first << ",";

        }
    }
};		
}//lrucache namespace end

