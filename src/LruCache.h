#pragma once
/**
 * author:zhouqing
 * todo:memory allocator
 */
#include <iostream>
#include <vector>
#include <map>
#include <memory>
//#include <boost/thread/recursive_mutex.hpp>
#include <sstream>
//#include <mutex>
#include <glog/logging.h>
#include "ConcurrentHashMap.h" 
#include <tbb/concurrent_queue.h>
#include <atomic>
//namespace lrucache{
using std::chrono::system_clock;
using std::chrono::milliseconds;

const int TRYCOUNT = 2;
template<typename k, typename v>
class LruCache {
public:
    typedef struct Node {
        k key;
        v data;

        std::shared_ptr<Node> pre = nullptr;
        std::shared_ptr<Node> next = nullptr;
        Node (const k& key_, const v& data_):key(key_),data(data_),pre(nullptr),next(nullptr),emptyCount(-1){
        }
        Node ():pre(nullptr),next(nullptr),emptyCount(-1){}
        ~Node(){
            pre = nullptr;next = nullptr;
        }
        bool evicted = false;
    }Node;
private:
    int capacity;
    int size = 0;
    int64_t duration;
    boost::recursive_mutex mutex;
    ConcurrentHashMap<k, std::shared_ptr<Node>> map;
    ConcurrentHashMap<k, int64_t>  logMap[2];
    short index = 0;

    tbb::concurrent_queue<std::shared_ptr<Node>> evictQueue;
    std::atomic_flag lockLog = ATOMIC_FLAG_INIT;
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
//        while(!headGuard.test_and_set());
        auto temp = head->next;
        head->next = newNode;
        newNode->pre = head;
        newNode->next = temp;
        temp->pre = newNode;
//        headGuard.clear();
    }
    
    void extract(std::shared_ptr<Node> node) {
        auto nextNode = node->next;
        auto preNode = node->pre;
        preNode->next = nextNode;
        nextNode->pre = preNode;
        node->pre = node->next = nullptr;
    }
    
    bool del(std::shared_ptr<Node> node, const std::string& sour = "not redolog") {
/*
        if(nullptr == node){
            LOG(INFO) << "delete node is nullptr";
            return false;
        }
        if (node->pre ==nullptr || node->next == nullptr) {
            LOG(INFO) << "cant not delete tail or head node! or the node is not in cache, source:" << sour << std::endl;
            return false;
        }
*/
//        extract(node);
        node->evicted = true;
        evictQueue.push(node);
        map.erase(node->key);
        LOG(INFO) << "del:" << node->key << std::endl;
        return true;
    }

    // async run in single thread
    int clearEvictQueue() {
        int cnt = 0;
        while(true) {
            std::share_ptr<Node> t = nullptr;
            evictQueue.pop(t);
            if(t != nullptr){
                cnt++;
                extract(t);
                std::cout << "delete:" << t->key;
            }
        }
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
        valNode->evict = true; //mark as deleted
        //valNode->accessTime = ct;
        //extract(valNode);
        std::share_ptr<Node> newNode = std::make_shared<Node>(valNode->key,valNode->val);
        insertHead(newNode);
        map.put(key,newNode);//delete in map
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
        std::async(std::launch::async,clearEvictQueue);
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
    T get(const k& key) {
            auto valNode = map.get(key);
            if(nullptr != valNode){
                logMap[index].put(key,currenttime);
                return valNode->val;
            }
            return NULL;
    }

    int set(const k& key, const T& val) {
        if(lockLog.test_and_set(std::memory_order_acquire)) {
            reDoLog();
            lockLog(std::memory_order_release);
        }
        LOG(INFO) << "put:" << key << std::endl;
        if (map.contains(key)) {
            updateNode(key);
        } else {
            auto newNode = std::make_shared<Node>(key, val);
            newNode->accessTime = curtime;
            insertHead(newNode);
            map.put(key, newNode);
        }
        return miss;
    }
};        
//}//lrucache namespace end

