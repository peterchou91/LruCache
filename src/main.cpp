#include <thread>
#include "LruCache.h"
using namespace std;
void run(lrucache::LruCache<int, int>& cache, int index){
    std::vector<int> keyVec;
    std::vector<int> valVec;
    for(int i = index; i < index + 15; i++){
        keyVec.clear();
        valVec.clear();
        keyVec.push_back(0);
        valVec.push_back(0);
        cache.put(keyVec, valVec);
        keyVec.clear();
        valVec.clear();
        keyVec.push_back(i);
        valVec.push_back(i);
        cache.put(keyVec, valVec);

    }
    int* result = new int(-1);
    bool s = cache.get(44, *result);
    std::cout << "size:" << cache.volume() << std::endl;
    std::cout << "sucess:" << s << std::endl;
    std::cout << "result:" << *result << std::endl;
}
int main(){

	lrucache::LruCache<int, int> cache;
    std::thread thread(
        [&cache](){
            run(cache,0);
        }
    );
    std::thread thread1(
            [&cache](){
                run(cache, 15);
            }
    );
    std::thread thread3(
            [&cache](){
                run(cache,30);
            }
    );
    thread.join();
    thread1.join();
    thread3.join();
    cache.printAll();


    /*
    ConcurrentHashMap<int, int> cmap;
    for(int i = 0; i < 10; i++){
        cmap.put(i, i);
    }
    auto it = cmap.begin();
    for(;it != cmap.end(); it++){

        if(it->first == 6) {
            cmap.erase(it->first);
        }
        cout << it->first << endl;
    }
    cout << "size:" << cmap.size() << endl;
    cout << "buckets:" << cmap.bucket_count() << endl;
     */


}
