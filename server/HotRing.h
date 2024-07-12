//
// Created by lvzhi on 2024/7/10.
//

#ifndef HOTRING_HOTRING_H
#define HOTRING_HOTRING_H

#include <atomic>
#include <vector>
#include <cstdint>
#include <optional>
#include <algorithm>
#include <limits>

struct StatisticalData {
    int hotHit = 0;
    int adjustHotpot = 0;
    int allAccessCount = 0;
};

template<typename K, typename V>
class HotRing {
private:
  std::atomic<bool> active;
  static constexpr size_t SAMPLE_THRESHOLD = 20;

public:
  struct Item {
      K key;
      V value;
      std::atomic<Item*> next;
      std::atomic<uint16_t> accessCount;
      std::size_t totalAccessCount;
      std::size_t totalAccessCountAsHot;

      Item(const K& k, const V& v) : key(k), value(v), next(nullptr),
          accessCount(0), totalAccessCount(0), totalAccessCountAsHot(0) {}
  };
  std::atomic<uint16_t> totalCount;
  std::atomic<Item*> head;
  std::atomic<size_t> hashBitNum;
  std::atomic<size_t> quantity;
  StatisticalData* data;

  explicit HotRing(size_t hashBitNum_, StatisticalData* data_): head(nullptr), totalCount(0), active(false),
      hashBitNum(hashBitNum_), quantity(0), data(data_) {}

  std::optional<V> find(const K& key);
  void insert(const K& key, const V& value, int totalAccessCount=0, int totalCountAsHot=0);
  void checkAndStartSampling(bool isHotAccess);
  void adjustHotspot();
  void print();
  std::pair<HotRing<K, V>*, HotRing<K, V>*> split(std::hash<K> hasher);

  ~HotRing() {
    Item* head_item = head.load();
    if (head_item) {
      Item* current = head_item->next.load();
      while (current != head_item) {
        Item* next = current->next.load();
        delete current;
        current = next;
      }
      delete head_item;
    }
  }
};

template<typename K, typename V>
std::pair<HotRing<K, V> *, HotRing<K, V> *> HotRing<K, V>::split(std::hash<K> hasher) {
  auto * ring1 = new HotRing<K, V>(hashBitNum+1, data);
  auto * ring2 = new HotRing<K, V>(hashBitNum+1, data);
  Item* head_item = head.load();
  if (!head_item) return std::make_pair(ring1, ring2);
  Item* current = head_item;
  do {
    auto index = hasher(current->key) & ((1 << (hashBitNum+1))-1);
    if (index < (1 << hashBitNum)) {
      ring1->insert(current->key, current->value, current->totalAccessCount, current->totalAccessCountAsHot);
    }
    else {
      ring2->insert(current->key, current->value, current->totalAccessCount, current->totalAccessCountAsHot);
    }
    current = current->next.load();
  } while (current != head_item);
  return std::make_pair(ring1, ring2);
}

template<typename K, typename V>
void HotRing<K, V>::adjustHotspot() {
  data->adjustHotpot += 1;
  Item* head_item = head.load();
  if (!head_item) return;

  Item* current = head_item;
  std::vector<std::pair<Item*, uint16_t>> items;
  uint32_t total = 0;

  do {
    items.emplace_back(current, current->accessCount.load());
    total += current->accessCount.load();
    current->accessCount.store(0);
    current = current->next.load();
  } while (current != head_item);

  if (totalCount == 0) return;

  size_t optimalPos = 0;
  double minCost = std::numeric_limits<double>::max();

  for (size_t i = 0; i < items.size(); i++) {
    double cost = 0;
    for (size_t j = 0; j < items.size(); j++) {
      size_t distance = (j >= i) ? (j - i) : (items.size() - i + j);
      cost += (items[j].second / static_cast<double>(total)) * distance;
    }
    if (cost < minCost) {
      minCost = cost;
      optimalPos = i;
    }
  }

  head.store(items[optimalPos].first);
  totalCount.store(0);
  active.store(false);
}

template<typename K, typename V>
void HotRing<K, V>::checkAndStartSampling(bool isHotAccess) {
  this->totalCount += 1;
  if (this->totalCount.load() >= SAMPLE_THRESHOLD) {
    if (this->active.load()) {
      this->active.store(false);
      adjustHotspot();
    } else if (!isHotAccess) {
      this->active.store(true);
      Item* head_item = head.load();
      if (head_item) {
        head_item->accessCount.store(0);
        Item *current = head_item->next.load();
        while (current != head_item) {
          current->accessCount.store(0);
          current = current->next.load();
        }
      }
    }
    this->totalCount.store(0);
  }
}

template<typename K, typename V>
void HotRing<K, V>::print() {
  if (!head) return;
  Item* current = head.load();
  do {
    std::cout << "key : " << current->key << " value : " << current->value
        << " total count : " << current->totalAccessCount
        << " total count as hot : " << current->totalAccessCountAsHot << std::endl;

    current = current->next.load();
  } while (current != head);
}

template<typename K, typename V>
std::optional<V> HotRing<K, V>::find(const K &key) {
  data->allAccessCount += 1;
  Item* current = this->head.load();
  if (!current) return std::nullopt;
  do {
    Item* next = current->next;
    if (current->key == key) {
      if (active.load()) {
        current->accessCount += 1;
      }
      current->totalAccessCount += 1;
      bool isHoot = (current == head.load());
      if (isHoot) {
        current->totalAccessCountAsHot += 1;
        data->hotHit += 1;
      }
      checkAndStartSampling(isHoot);
      return current->value;
    } else if (current->key < key && next->key > key) {
      break;
    }
    current = current->next.load();
  } while (current != this->head.load());
  return std::nullopt;
}

template<typename K, typename V>
void HotRing<K, V>::insert(const K &key, const V &value, int totalAccessCount, int totalCountAsHot) {
  data->allAccessCount += 1;
  Item* newItem = new Item(key, value);
  Item* head_item = head.load();
  newItem->totalAccessCountAsHot = totalCountAsHot;
  newItem->totalAccessCount = totalAccessCount;
  if (!head_item) {
    // head为nullptr
    newItem->next.store(newItem);
    if (head.compare_exchange_weak(head_item, newItem)) {
      quantity.fetch_add(1);
      return;
    }
  } else {
    Item* current = head_item;
    Item* insertAfter = nullptr;
    Item* maxItem = head_item;
    bool foundPos = false;
    do {
      if (current->key == key) {
        current->totalAccessCount += 1;
        bool isHoot = (current == head.load());
        if (active.load()) {
          current->accessCount += 1;
        }
        if (isHoot) {
          current->totalAccessCountAsHot += 1;
          data->hotHit += 1;
        }
        checkAndStartSampling(isHoot);
        current->value = value;
        delete newItem;
        return;
      }
      Item* next = current->next.load();
      if (current->key <= key && key < next->key) {
        insertAfter = current;
        foundPos = true;
        break;
      }
      if (next == head && current->key > next->key && (key > current->key || key < next->key)) {
        insertAfter = current;
        foundPos = true;
        break;
      }
      if (maxItem->key < current->key) {
        maxItem = current;
      }
      current = next;
    } while (current != head_item);

    if (!foundPos) {
      insertAfter = maxItem;
    }

    newItem->next.store(insertAfter->next.load());
    Item* expected = newItem->next.load();
    insertAfter->next.compare_exchange_weak(expected, newItem);
  }
  quantity.fetch_add(1);
}

#endif //HOTRING_HOTRING_H
