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
    [[nodiscard]] size_t size() const {
        return m_cache_list.size();
    }

    bool try_get(const Key &key, T &output) {
        typename CacheMap::iterator map_it = m_cache_map.find(key);
        if (map_it == m_cache_map.end()) {
            return false;
        } else {
            output = std::get<1>(*map_it->second);
            move_to_front(map_it);
            return true;
        }
    }

    void add(const Key &key, const T &entry) {
        typename CacheMap::iterator map_it = m_cache_map.find(key);
        if (map_it != m_cache_map.end()) {
            std::get<1>(*map_it->second) = entry;
            move_to_front(map_it);
        } else {
            m_cache_list.emplace_front(key, entry);
            m_cache_map[key] = m_cache_list.begin();
            if (size() > Capacity) {
                Key removed_key = std::get<0>(m_cache_list.back());
                [[maybe_unused]]
                size_t removed = m_cache_map.erase(removed_key);
                assert(removed == 1);
                m_cache_list.pop_back();
            }
        }
    }

private:
    CacheList m_cache_list;
    CacheMap m_cache_map;

    void move_to_front(const typename CacheMap::iterator &map_it) {
        typename CacheList::iterator list_it = map_it->second;
        m_cache_list.push_front(*list_it);
        m_cache_map[map_it->first] = m_cache_list.begin();
        m_cache_list.erase(list_it);
    }
};

} // namespace dp
