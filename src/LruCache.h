#pragma once
/**
 * author:zhouqing
 */
#include <iostream>
#include <vector>
#include <map>
#include <memory>
#include <boost/thread/recursive_mutex.hpp>
#include <sstream>
#include <mutex>
#include <glog/logging.h>
#include "ConcurrentHashMap.h" 
//namespace lrucache{
using std::chrono::system_clock;
using std::chrono::milliseconds;


template<typename k, typename v>
class LruCache {
public:
    typedef struct Node {
        k key;
        v data;

        int64_t accessTime;
        int emptyCount;

        std::shared_ptr<Node> pre;
        std::shared_ptr<Node> next;
        Node (const k& key_, const v& data_):key(key_),data(data_),pre(nullptr),next(nullptr),emptyCount(-1){
        }
        Node ():pre(nullptr),next(nullptr),emptyCount(-1){}
        ~Node(){
        }

    }Node;
private:
    int capacity;
    int size = 0;
    int64_t duration;
    boost::recursive_mutex mutex;
    ConcurrentHashMap<k, std::shared_ptr<Node>> map;
    ConcurrentHashMap<k, int64_t>  logMap[2];
    short index = 0;
    std::shared_ptr<Node> head;
    std::shared_ptr<Node> tail;
public:
    void swap(){
        index != index;
    }
    int64_t currentTimeMill(){
        auto start = system_clock::now();
        int64_t time = (std::chrono::duration_cast<milliseconds>(start.time_since_epoch())).count();
        return time;
    }
    void insertHead(std::shared_ptr<Node> newNode) {
        auto temp = head->next;
        head->next = newNode;
        newNode->pre = head;
        newNode->next = temp;
        temp->pre = newNode;
    }
    
    void extract(std::shared_ptr<Node> node) {
        /*
        if(nullptr == node){
            LOG(INFO) << "extract node is nullptr";
            return;
        }
        if (node->pre ==nullptr || node->next == nullptr) {
            LOG(INFO) << "cant not extract tail or head node! or the node is not in cache" << std::endl;
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
            LOG(INFO) << "delete node is nullptr";
            return false;
        }
        if (node->pre ==nullptr || node->next == nullptr) {
            LOG(INFO) << "cant not delete tail or head node! or the node is not in cache, source:" << sour << std::endl;
            return false;
        }
        extract(node);
        LOG(INFO) << "del:" << node->key << std::endl;
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
    


    void updateNode(const k& key, int64_t ct){
        std::shared_ptr<Node> valNode = map.get(key);
        valNode->accessTime = ct;
        extract(valNode);
        LOG(INFO) << "extract:" << valNode->key << std::endl;
        insertHead(valNode);
    }
    
    void updateNode(const k& key) {
        updateNode(key,currentTimeMill());
    }

    void reDoLog(){
        swap();
        auto bak = 1 - index;
        auto itr = logMap[bak].begin();
        auto end = logMap[bak].end();
        auto ct = currentTimeMill();
        for(; itr != end; itr++) {
            auto innerKey = itr->first;
            std::shared_ptr<Node> valNode = map.get(innerKey);
            if(valNode != nullptr) {
                if((itr->second == -1)&& del(valNode, "redolog")){
                    // expired
                    size--;
                    map.erase(innerKey);
                } else {
                    updateNode(innerKey, ct);
                }
            }
//            logMap[bak].erase(itr->first);
        }
        logMap[bak].clear();
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
        this->size = 0;
    }

    int volume(){
        return this->size;
    }

    void printAll(){
        boost::recursive_mutex::scoped_lock lock(mutex);
        std::stringstream ss;
        auto it = head->next;
        while (it != nullptr && it != tail) {
           ss << it->key << ",";
           it = it->next;
        }
        LOG(ERROR) << ss.str();
    }
    int getAllPresent(const std::vector<k>& keys,
        std::map<k, v> & values, std::vector<k> & missingKeys) {
        int hits = 0;
        auto currenttime = currentTimeMill();
        for (auto key : keys) {
            if(!map.contains(key)) {
                missingKeys.push_back(key);
                continue;
            }
            auto valNode = map.get(key);
            if(nullptr == valNode) {
                missingKeys.push_back(key);
                continue;
            }
            if(valNode->accessTime / 1000 + this->duration < currenttime / 1000) {
                logMap[index].put(key, -1);
                missingKeys.push_back(key);
//                LOG(ERROR) << "expired";
                continue;
            }
            if(valNode->emptyCount >= 0 && valNode->emptyCount < 2) {
                missingKeys.push_back(key);
                continue;
            }
            hits++;
            if(valNode->emptyCount == -1) {
                values.insert(std::pair<k,v>(key, valNode->data));
            }
            logMap[index].put(key,currenttime);
        }
//        LOG(ERROR) << "keysize:" << keys.size() << " miss keysize:" << miss;
        return hits;
    }

    int insertNewElements(const std::vector<k>& keys, const std::map<k, v> & values) {
        boost::recursive_mutex::scoped_lock lock(mutex);
        size_t count = keys.size();
        int miss = 0;
        reDoLog();
        if(count == 0) return 0;
        auto curtime = currentTimeMill();
        for(int i = 0; i < count; i++) {
            const auto& key = keys[i];
            auto it = values.find(key);
            if(it == values.end()) {
                auto valNode = map.get(key);
                if (nullptr == valNode) {
                    auto newNode = std::make_shared<Node>();
                    newNode->accessTime = curtime;
                    newNode->key = key;
                    newNode->emptyCount = 0;
                } else {
                    if (valNode->emptyCount < 0) continue;
                    else {
                        if(valNode->emptyCount < 1)valNode->emptyCount ++;
                    }
                }
                miss++;
                continue;
            }
            const auto& val = it->second;
            LOG(INFO) << "put:" << key << std::endl;
            if (map.contains(key)) {
                updateNode(key);
            } else {
                LOG(INFO) << "size:" << size << " " << map.size() << std::endl;
                if (size >= capacity) {
                    deleteLast();
                }
                auto newNode = std::make_shared<Node>(key, val);
                newNode->accessTime = curtime;
                insertHead(newNode);
                map.put(key, newNode);
                size++;
            }
        }
//        LOG(ERROR) << "insert miss:" << miss;
        return miss;
    }
};        
//}//lrucache namespace end

