#pragma once
#include "Models.h"
#include <string>
#include <vector>
#include <list>
#include <unordered_map>
#include <optional>

class RouteCache {
private:
    size_t capacity;
    
    // stores std::vector<std::string> (The raw path of Segment IDs)
    std::list<std::pair<std::string, std::vector<std::string>>> lruList;
    std::unordered_map<std::string, decltype(lruList.begin())> cacheMap;

public:
    RouteCache(size_t cap = 1000);

    // Returns the raw path
    std::optional<std::vector<std::string>> get(const std::string& queryKey);

    // Takes the raw path
    void put(const std::string& queryKey, const std::vector<std::string>& path);
};