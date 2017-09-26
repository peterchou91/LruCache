#pragma once

#include <tbb/concurrent_hash_map.h>
#include <iostream>

/**
 *  * self-use ConcurrentHashMap which key is string
 *   */
template<typename T, typename V> class ConcurrentHashMap : public  tbb::concurrent_hash_map<T,V>{

private:
    typedef typename tbb::concurrent_hash_map<T, V>::accessor InnerAccessor;
    typedef typename tbb::concurrent_hash_map<T, V>::const_accessor ConstAccessor;

public:
//    tbb::concurrent_hash_map<T, V> innerConcurrentMap;
public:
    ~ConcurrentHashMap() {}

    V get(const T& key);

    void put(const T& key, const V& value);

    bool contains(const T& key);

};


template <typename T, typename V>
V ConcurrentHashMap<T, V>::get(const T& key) {
    ConstAccessor accessor;
    if (this->find(accessor, key)) {
        return (V)accessor->second;
    } else {
        return nullptr;
    }
}

template <typename T, typename V>
void ConcurrentHashMap<T, V>::put(const T& key, const V& value) {
    InnerAccessor a;
    this->insert(a, key);
    a->second = value;
}

template <typename T, typename V>
bool ConcurrentHashMap<T, V>::contains(const T& key) {
    ConstAccessor a;
    if (this->find(a, key)) return true;
    return false;
}

