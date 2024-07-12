//
// Created by lvzhi on 2024/7/10.
//

#ifndef HOTRING_HASHTABLE_H
#define HOTRING_HASHTABLE_H

#include "HotRing.h"

template<typename K, typename V>
class HashTable {
private:
  std::vector<HotRing<K, V>*> buckets;
  size_t hashBitNum;
  std::hash<K> hasher;
  const int MAX_NUM_PER_RING = 8;

  size_t hash(const K& key) const {
    return hasher(key) & ((1 << hashBitNum)-1);
  }

public:
  StatisticalData* data;
  explicit HashTable(size_t num): hashBitNum(num), buckets(1<<num) {
    data = new StatisticalData;
    for (int i = 0; i < buckets.size(); i++)
    {
      buckets[i] = new HotRing<K, V>(hashBitNum, data);
    }
  }

  std::optional<V> read(const K& key);
  void put(const K& key, const V& value);
  void print();
  void extend();
  void rehash(size_t index);
};

template<typename K, typename V>
void HashTable<K, V>::rehash(size_t index) {
  size_t oldBitNum = buckets[index]->hashBitNum.load();
  auto oldRing = buckets[index];
  int oldSize = 1 << oldBitNum;
  auto ringPair = buckets[index]->split(hasher);
  size_t initIndex = index & ((1 << oldBitNum)-1);
  while (initIndex < buckets.size()) {
    buckets[initIndex] = ringPair.first;
    buckets[initIndex + oldSize] = ringPair.second;
    initIndex += 2*oldSize;
  }
  delete oldRing;
}

template<typename K, typename V>
void HashTable<K, V>::extend() {
  std::cout << "extend" << std::endl;
  int oldSize = 1 << hashBitNum;
  // 默认2倍rehash
  hashBitNum += 1;
  buckets.resize(1 << hashBitNum);
  for (int i = oldSize; i < buckets.size(); i++)
  {
    buckets[i] = buckets[i-oldSize];
  }
}

template<typename K, typename V>
void HashTable<K, V>::print() {
  int index = 0;
  for (auto ring : buckets) {
    if (ring->quantity > 0) {
      std::cout << "index : " << index << std::endl;
      ring->print();
    }
    index++;
  }
}

template<typename K, typename V>
std::optional<V> HashTable<K, V>::read(const K &key) {
  size_t index = hash(key);
//  std::cout << "read " << index << std::endl;
  return buckets[index]->find(key);
}

template<typename K, typename V>
void HashTable<K, V>::put(const K &key, const V &value) {
  size_t index = hash(key);
//  std::cout << "put " << index << std::endl;
  buckets[index]->insert(key, value);
  if (buckets[index]->quantity.load() >= MAX_NUM_PER_RING) {
    if (buckets[index]->hashBitNum == hashBitNum) {
      extend();
    }
    rehash(index);
  }
}

#endif //HOTRING_HASHTABLE_H
