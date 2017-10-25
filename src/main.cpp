#include <thread>
#include "LruCache.h"
#include <chrono>
using namespace std;

int main(){
    LruCache<int,int> cache(6);
    cache.init();
    cache.set(1,1);
    cache.set(2,2);
    cache.set(3,3);
    cache.set(4,1);
    cache.set(5,2);
    cache.set(6,3);
    cache.printAll();
    int ret;
    cache.get(4,ret);
    cache.get(6,ret);
    cache.set(7,2);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    cache.printAll();
    cache.erase(3);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    cache.printAll();
    std::cout << "queue size:" << cache.evictQueueSize() << std::endl;

}