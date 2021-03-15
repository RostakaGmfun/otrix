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
    }

    hash_map_node<K> *find(const K &k) const
    {
        const size_t idx = hash_func_(k) % table_size_;
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

    void insert(hash_map_node<K> *node)
    {
        const size_t idx = hash_func_(node->key) % table_size_;
        intrusive_list_push_back(table_[idx], &node->list_node);
    }

    void remove(hash_map_node<K> *node)
    {
        const size_t idx = hash_func_(node->key) % table_size_;
        intrusive_list_delete(table_[idx], node);
    }

private:
    size_t table_size_;
    size_t (*hash_func_)(const K &k);
    intrusive_list **table_;
};

} // namespace otrix
