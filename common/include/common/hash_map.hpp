#pragma once

#include "common/list.h"

namespace otrix {

template <typename K>
struct hash_map_node
{
    hash_map_node()
    {}
    hash_map_node(const K &k): key(k)
    {}
    intrusive_list list_node;
    K key;
};

template<typename K>
hash_map_node<K> *hash_map_node_ptr(intrusive_list *list_node)
{
    return container_of(list_node, hash_map_node<K>, list_node);
}

template <typename K>
class hash_map
{
public:
    hash_map(size_t table_size, size_t (*hash_func)(const K &k)):
        table_size_(table_size), hash_func_(hash_func)
    {
        table_ = new intrusive_list*[table_size];
        for (size_t i = 0; i < table_size; i++) {
            table_[i] = nullptr;
        }
    }

    virtual ~hash_map()
    {
        delete[] table_;
    }

    hash_map_node<K> *find(const K &k) const
    {
        const size_t idx = hash_func_(k) % table_size_;
        return find(idx, k);
    }

    // NOTE: The caller should guarantee that there is no duplicate by calling find() bedorehand
    void insert(hash_map_node<K> *node)
    {
        const size_t idx = hash_func_(node->key) % table_size_;
        table_[idx] = intrusive_list_push_back(table_[idx], &node->list_node);
    }

    void remove(hash_map_node<K> *node)
    {
        const size_t idx = hash_func_(node->key) % table_size_;
        table_[idx] = intrusive_list_delete(table_[idx], &node->list_node);
    }

private:

    hash_map_node<K> *find(size_t idx, const K &k) const
    {
        intrusive_list *head = table_[idx];

        if (nullptr == head) {
            return nullptr;
        }

        do {
            if (hash_map_node_ptr<K>(head)->key == k) {
                return hash_map_node_ptr<K>(head);
            }
            head = head->next;
        } while (head != table_[idx]);

        return nullptr;
    }

    size_t table_size_;
    size_t (*hash_func_)(const K &k);
    intrusive_list **table_;
};

template <typename K, typename V>
class pooled_hash_map: public hash_map<K>
{
public:

    struct entry_t
    {
        V value;
        hash_map_node<K> hm_node;
    };

    pooled_hash_map(size_t pool_size, size_t table_size, size_t (*hash_func)(const K &k)): hash_map<K>(table_size, hash_func),
        pool_size_(pool_size)
    {
        pool_ = new entry_t[pool_size_];
        pool_head_ = nullptr;
        for (size_t i = 0; i < pool_size; i++) {
            intrusive_list_init(&pool_[i].hm_node.list_node);
            pool_head_ = intrusive_list_push_back(pool_head_, &pool_[i].hm_node.list_node);
        }
    }

    ~pooled_hash_map() override
    {
        for (size_t i = 0; i < pool_size_; i++) {
            intrusive_list_unlink_node(&pool_[i].hm_node.list_node);
        }
        delete [] pool_;
    }

    static entry_t *to_entry(hash_map_node<K> *node)
    {
        return container_of(node, entry_t, hm_node);
    }

    entry_t *insert(const K &key, V &&value)
    {
        if (nullptr == pool_head_) {
            return nullptr;
        }
        intrusive_list *new_item = pool_head_;
        entry_t *p_entry = container_of(hash_map_node_ptr<K>(new_item), entry_t, hm_node);
        p_entry->value = value;
        p_entry->hm_node.key = key;
        pool_head_ = intrusive_list_delete(pool_head_, new_item);
        hash_map<K>::insert(&p_entry->hm_node);
        return p_entry;
    }

    void remove(hash_map_node<K> *node)
    {
        hash_map<K>::remove(node);
        if ((void *)node >= (void *)pool_ && (void *)node < (void *)(pool_ + pool_size_)) {
            // Only add those nodes that belong to the pool
            pool_head_ = intrusive_list_push_back(pool_head_, &node->list_node);
        }
    }

    void remove(const K &key)
    {
        hash_map_node<K> *node = hash_map<K>::find(key);
        if (nullptr != node) {
            remove(node);
        }
    }

private:
    size_t pool_size_;
    entry_t *pool_;
    intrusive_list *pool_head_;
};

} // namespace otrix
