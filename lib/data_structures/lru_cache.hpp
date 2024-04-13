#pragma once

#include <list>
#include <unordered_map>
#include <tuple>
#include <optional>
#include <cassert>

namespace dp {

template<typename Key, typename T, size_t Capacity, typename Hash = std::hash<Key>>
class LruCache {
public:
    using EntryPair = std::tuple<Key, T>;

private:
    using CacheList = std::list<EntryPair>;
    using CacheMap = std::unordered_map<Key, typename CacheList::iterator, Hash>;

public:
    [[nodiscard]] size_t size() const {
        return m_cache_list.size();
    }

    std::optional<T> try_get(const Key &key) {
        typename CacheMap::iterator map_it = m_cache_map.find(key);
        if (map_it == m_cache_map.end()) {
            return std::nullopt;
        } else {
            move_to_front(map_it);
            return std::get<1>(*map_it->second);
        }
    }

    /**
     * @brief Adds new entry to the cache under given key.
     *
     * If the key already exists, replaces the existing entry with the new one.
     * If the cache is full, removes the least recently used entry.
     *
     * @return If an entry has been removed in the process, returns that key-entry pair; otherwise returns nullopt
     */
    std::optional<EntryPair> add(const Key &key, const T &entry) {
        typename CacheMap::iterator map_it = m_cache_map.find(key);
        if (map_it != m_cache_map.end()) {
            T &e = std::get<1>(*map_it->second);
            EntryPair removed{key, e};
            e = entry;
            move_to_front(map_it);
            return removed;
        } else {
            m_cache_list.emplace_front(key, entry);
            m_cache_map[key] = m_cache_list.begin();
            if (size() > Capacity) {
                Key removed_key = std::get<0>(m_cache_list.back());
                map_it = m_cache_map.find(removed_key);
                assert(map_it != m_cache_map.end());
                const T &removed_entry = std::get<1>(*map_it->second);
                EntryPair removed_pair{removed_key, removed_entry};
                [[maybe_unused]] size_t removed = m_cache_map.erase(removed_key);
                assert(removed == 1);
                m_cache_list.pop_back();
                return removed_pair;
            } else {
                return std::nullopt;
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
