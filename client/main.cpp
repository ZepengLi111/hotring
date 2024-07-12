//
// Created by lvzhi on 2024/7/9.
//

#include <string>
#include <vector>
#include <random>
#include <chrono>
#include <algorithm>
#include <unordered_set>
#include <iostream>
#include <thread>
#include <atomic>
#include "Client.h"

class DataCollector {
private:
    std::vector<int> loads_per_minute;
    std::chrono::steady_clock::time_point start_time;

public:
    static bool doing;
    static std::atomic<int> current_load;
    DataCollector() {
      start_time = std::chrono::steady_clock::now();
    }

    static void increment_load() {
      current_load.fetch_add(1);
    }

    void collect_data() {
      for (int i = 0; i < 5; ++i) {
        std::this_thread::sleep_for(std::chrono::seconds(60));
//        std::this_thread::sleep_for(std::chrono::minutes(1));
        loads_per_minute.push_back(DataCollector::current_load.exchange(0));
        std::cout << "collect! " << loads_per_minute[i] << std::endl;
      }
      doing = false;
    }

    void print_data() {
      std::cout << "Loads per minute:\n";
      for (int i = 0; i < loads_per_minute.size(); ++i) {
        std::cout << "Minute " << i+1 << ": " << loads_per_minute[i] << "\n";
      }
    }
};

std::atomic<int> DataCollector::current_load{0};
bool DataCollector::doing = true;

struct KeyValuePair {
    std::string key;
    std::string value;
};

class KeyValueGenerator {
private:
    std::vector<std::string> cold_keys;
    std::vector<std::string> hot_keys;
    std::default_random_engine generator;
    std::uniform_real_distribution<> dis;

    std::string generate_random_string(size_t length) {
      const char charset[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
      const size_t charset_size = sizeof(charset) - 1;

      std::string result;
      result.reserve(length);

      std::uniform_int_distribution<int> distribution(0, charset_size - 1);

      for (size_t i = 0; i < length; ++i) {
        result += charset[distribution(generator)];
      }

      return result;
    }

public:
    bool all_hot;
    KeyValueGenerator(int total_keys, double hot_key_percentage=0.2, bool all_hot_=true)
            : generator(std::chrono::system_clock::now().time_since_epoch().count()),
              dis(0.0, 1.0), all_hot(all_hot_) {
      std::unordered_set<std::string> unique_keys;
      while (unique_keys.size() < total_keys) {
        unique_keys.insert(generate_random_string(8));
      }
      std::vector<std::string> all_keys(unique_keys.begin(), unique_keys.end());

      int hot_key_count = static_cast<int>(total_keys * hot_key_percentage);
      std::shuffle(all_keys.begin(), all_keys.end(), generator);
      if (!all_hot) {
        hot_keys = std::vector<std::string>(all_keys.begin(), all_keys.begin() + hot_key_count);
        cold_keys = std::vector<std::string>(all_keys.begin() + hot_key_count, all_keys.end());
      } else {
        hot_keys = std::vector<std::string>(all_keys.begin(), all_keys.end());
      }
      std::cout << hot_keys.size() << std::endl;
    }

    KeyValuePair generate_key_value_pair() {
      KeyValuePair kvp;
      if (all_hot) {
        kvp.key = hot_keys[std::uniform_int_distribution<>(0, hot_keys.size() - 1)(generator)];
      }
      else {
        if (dis(generator) < 0.8) {  // 80% 概率生成热点键
          kvp.key = hot_keys[std::uniform_int_distribution<>(0, hot_keys.size() - 1)(generator)];
        } else {
          kvp.key = cold_keys[std::uniform_int_distribution<>(0, cold_keys.size() - 1)(generator)];
        }
      }
      kvp.value = generate_random_string(16);
      return kvp;
    }

    size_t get_hot_keys_count() const { return hot_keys.size(); }
    size_t get_cold_keys_count() const { return cold_keys.size(); }
};

int main() {
  int total_keys = 50000;
  double hot_key_percentage = 0.2;
  bool all_hot = false;
  KeyValueGenerator generator(total_keys, hot_key_percentage=hot_key_percentage, all_hot=all_hot);
  Client client("127.0.0.1", 8812);
//  size_t count = 0;

  DataCollector collector;
  std::thread collection_thread(&DataCollector::collect_data, &collector);
  std::cout << "here" << std::endl;

  while (DataCollector::doing) {
    KeyValuePair kvp = generator.generate_key_value_pair();
    KeyValuePair kvp2 = generator.generate_key_value_pair();
    std::string message = kvp.key + " " + kvp.value + " " + kvp2.key;
    client.send_message(message);
    std::string response = client.receive_message();
//    std::cout << "Received: " << response << std::endl;
    collector.increment_load();
//    std::cout << count << std::endl;
  }
//  collection_thread.join();

  return 0;
}

