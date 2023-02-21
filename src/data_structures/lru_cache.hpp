#pragma once

#include <list>
#include <unordered_map>
#include <tuple>
#include <cassert>

namespace dp {

template<typename Key, typename T, size_t Capacity, typename Hash = std::hash<Key>>
class LruCache {
private:
    using EntryPair = std::tuple<Key, T>;
    using CacheList = std::list<EntryPair>;
    using CacheMap = std::unordered_map<Key, typename CacheList::iterator, Hash>;

public:
    size_t size() const {
        return m_cache_list.size();
    }

    bool try_get(const Key &key, T &output) {
        auto map_it = m_cache_map.find(key);
        if (map_it == m_cache_map.end()) {
            return false;
        } else {
            auto list_it = map_it->second;
            output = std::get<1>(*list_it);
            m_cache_list.erase(list_it);
            m_cache_list.push_front(*list_it);
            return true;
        }
    }

    void add(const Key &key, const T &entry) {
        m_cache_list.emplace_front(key, entry);
        if (size() > Capacity) {
            size_t removed = m_cache_map.erase(std::get<0>(m_cache_list.back()));
            assert(removed == 1);
            m_cache_list.pop_back();
        }
    }

private:
    CacheList m_cache_list;
    CacheMap m_cache_map;
};

} // namespace dp
