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
//#include <glog/logging.h>
#include "ConcurrentHashMap.h" 
#include <tbb/concurrent_queue.h>
#include <atomic>
#include <future>
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
        Node (const k& key_, const v& data_):key(key_),data(data_),pre(nullptr),next(nullptr){
        }
        Node ():pre(nullptr),next(nullptr){}
        ~Node(){
            pre = nullptr;next = nullptr;
            std::cout << "~Node delete node:" << key << std::endl;
        }
        bool evicted = false;
    }Node;
private:
    int capacity;
    std::shared_ptr<Node> head;
    std::shared_ptr<Node> tail;
    ConcurrentHashMap<k, std::shared_ptr<Node>> map;
    ConcurrentHashMap<k, int64_t>  logMap[2];
    short index = 0;

    tbb::concurrent_bounded_queue<std::shared_ptr<Node>> evictQueue;
    std::atomic_flag lockLog = ATOMIC_FLAG_INIT;
    std::thread thread_;
    bool shutdown = false;
public:
    void swap(){
        index = 1 - index;
    }
    /*
    int64_t currentTimeMill(){
        auto start = system_clock::now();
        int64_t time = (std::chrono::duration_cast<milliseconds>(start.time_since_epoch())).count();
        return time;
    }*/
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
        node->evicted = true;
        evictQueue.push(node);
        map.erase(node->key);
        //LOG(INFO) << "del:" << node->key << std::endl;
        std::cout << "del:" << node->key << std::endl;
        return true;
    }

    // async run in single thread
    int clearEvictQueue() {
        std::cout << "bg thread" << std::endl;
        int cnt = 0;
        while(!shutdown) {
            std::shared_ptr<Node> t = nullptr;
            evictQueue.pop(t);
            std::cout << "notify" << std::endl;
            if(t != nullptr){
                cnt++;
                extract(t);
                std::cout << "delete:" << t->key << std::endl;
            }
        }
    }

    /**
     * delete last element from cache
     */
    void deleteLast() {
        auto key = tail->pre->key;
        if(del(tail->pre)){
            map.erase(key);
        }
    }
    
    void updateNode(const k& key){
        std::shared_ptr<Node> valNode = map.get(key);
        updateNode(valNode);
    }

    void updateNode(std::shared_ptr<Node> valNode){
        valNode->evicted = true; //mark as deleted

        //valNode->accessTime = ct;
        //extract(valNode);
        std::shared_ptr<Node> newNode = std::make_shared<Node>(valNode->key,valNode->data);
        insertHead(newNode);
        map.put(valNode->key,newNode);//delete in map
        std::cout << "push:" << valNode->key << std::endl;
        evictQueue.push(valNode);
    }
    
    void reDoLog(){
        swap();
        auto bak = 1 - index;
        auto itr = logMap[bak].begin();
        auto end = logMap[bak].end();
        for(; itr != end; itr++) {
            auto innerKey = itr->first;
            std::cout << "redo" << bak << ":" << innerKey << std::endl;
            std::shared_ptr<Node> valNode = map.get(innerKey);
            if(valNode != nullptr) {
                updateNode(valNode);
            }
//            logMap[bak].erase(itr->first);
        }
        logMap[bak].clear();
    }
public:
    int volume(){
        return this->map.size();
    }

    int evictQueueSize(){
        return this->evictQueue.size();
    }




    explicit LruCache(int cap = 10):capacity(cap){
        head = std::make_shared<Node>();
        tail = std::make_shared<Node>();
        head->next = tail;
        tail->pre = head;
        head->pre = nullptr;
        tail->next = nullptr;
    }
    virtual ~LruCache(){
        tail = head = nullptr;
        shutdown = true;
        //if(thread_.joinable()) thread_.join();

    }
    void init(){
        std::thread thread(&LruCache::clearEvictQueue,this);
        std::swap(thread,thread_);
        thread_.detach();
    }

    void join(){
        thread_.join();
    }

    bool get(const k& key, v& t) {
            auto valNode = map.get(key);
            if(nullptr != valNode){
                std::cout << "put log" << index << ":" << key << std::endl;
                logMap[index].put(key,1);
                t = std::move(valNode->data);
                return true;
            }
            return false;
    }

    void set(const k& key, const v& val) {
        if(!lockLog.test_and_set(std::memory_order_acquire)) {
            reDoLog();
            lockLog.clear(std::memory_order_release);
        }
        //LOG(INFO) << "put:" << key << std::endl;
        if (map.contains(key)) {
            updateNode(key);
        } else {
            auto newNode = std::make_shared<Node>(key, val);
            insertHead(newNode);
            map.put(key, newNode);
        }
    }
    void erase(const k& key){
        auto node = map.get(key);
        if(nullptr != node){
            evictQueue.push(node);
            map.erase(key);
        }
    }
    void printAll(){
        std::stringstream ss;
        auto it = head->next;
        while (it != nullptr && it != tail) {
            if(!it->evicted)
                ss << it->key << ",";
            it = it->next;
        }
        std::cout << ss.str() << std::endl;
        //LOG(ERROR) << ss.str();
    }
};        
//}//lrucache namespace end

