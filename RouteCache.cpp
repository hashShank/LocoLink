#include "RouteCache.h"

RouteCache::RouteCache(size_t cap) : capacity(cap) {}

std::optional<std::vector<std::string>> RouteCache::get(const std::string& queryKey) {
    auto it = cacheMap.find(queryKey);
    
    if (it == cacheMap.end()) {
        return std::nullopt; 
    }
    
    lruList.splice(lruList.begin(), lruList, it->second);
    return it->second->second;
}

void RouteCache::put(const std::string& queryKey, const std::vector<std::string>& path) {
    auto it = cacheMap.find(queryKey);
    
    if (it != cacheMap.end()) {
        lruList.splice(lruList.begin(), lruList, it->second);
        it->second->second = path;
        return;
    }
    
    if (cacheMap.size() == capacity) {
        auto lastNode = lruList.back();
        cacheMap.erase(lastNode.first);
        lruList.pop_back();            
    }
    
    lruList.emplace_front(queryKey, path);
    cacheMap[queryKey] = lruList.begin();
}