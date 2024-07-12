//
// Created by lvzhi on 2024/7/10.
//

#include <iostream>
#include <thread>
#include <fstream>
#include <filesystem>
#include <unordered_map>
#include "HashTable.h"
#include <string>
#include "random.cpp"
#include "Server.h"

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
      for (int i = 0; i < 2; ++i) {
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

int main() {
  std::ofstream out("output_20_f.txt");
  std::streambuf *coutbuf = std::cout.rdbuf(); //保存旧的 buf
  std::cout.rdbuf(out.rdbuf()); //重定向 std::cout 到 out.txt

  Server server(8812);
  server.start();

//  int total_keys = 50000;
//  double hot_key_percentage = 0.2;
//  bool all_hot = false;
//  KeyValueGenerator generator(total_keys, hot_key_percentage=hot_key_percentage, all_hot=all_hot);
//  HashTable<std::string, std::string> kv_store(10);
//  int count = 0;
//  std::string str;
//  DataCollector collector;
//  std::thread collection_thread(&DataCollector::collect_data, &collector);
//  std::cout << "here" << std::endl;
//
//  while (DataCollector::doing) {
//    KeyValuePair kvp = generator.generate_key_value_pair();
//    KeyValuePair kvp2 = generator.generate_key_value_pair();
//    kv_store.put(kvp.key, kvp.value);
//    kv_store.read(kvp2.key);
//    DataCollector::increment_load();
//  }
//  std::cout << "Hot Hits : " << kv_store.data->hotHit << std::endl;
//  std::cout << "Adjust hotpot : " << kv_store.data->adjustHotpot << std::endl;
//  std::cout << "All access count : " << kv_store.data->allAccessCount << std::endl;
//  kv_store.print();
}
