#ifndef OTRIX_INTRUSIVE_LIST_H
#define OTRIX_INTRUSIVE_LIST_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "common/assert.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file intrusive_list.h
 *
 * Implements circular intrusive doubly linked list list.
 */

struct intrusive_list {
    struct intrusive_list *next;
    struct intrusive_list *prev;
};

#define container_of(list_ptr, struct_name, member_name) \
    ((struct_name*)((uint8_t*)(list_ptr) - offsetof(struct_name, member_name)))

typedef bool (*list_predicate)(const struct intrusive_list *node,
        void *context);

static inline struct intrusive_list *intrusive_list_init(struct intrusive_list *node)
{
    kASSERT(node != NULL);
    node->prev = node;
    node->next = node;
    return node;
}

/**
 * Returns tail of the list.
 *
 * @note The list is assumed to be circular.
 */
static inline struct intrusive_list *intrusive_list_get_tail(struct intrusive_list *head)
{
    kASSERT(head != NULL);
    return head->prev;
}

/**
 * @param[in] head Start of the list.
 * @param[in] tail Node to attach at the end of the list pointed by head.
 *
 * @return Tail.
 * @retval NULL On failure.
 */
static inline struct intrusive_list *intrusive_list_push_back(struct intrusive_list *head,
        struct intrusive_list *tail)
{
    kASSERT(tail != NULL);

    if (NULL == head) {
        intrusive_list_init(tail);
        return tail;
    }

    struct intrusive_list *last = intrusive_list_get_tail(head);

    last->next = tail;
    tail->prev = last;
    tail->next = head;
    head->prev = tail;

    return tail;
}

/**
 * Unlinks the given node from it's neightbours,
 * thus deleting it from the list.
 */
static inline void intrusive_list_unlink_node(struct intrusive_list *node)
{
    if (node == NULL) {
        return;
    }

    node->prev->next = node->next;
    node->next->prev = node->prev;
}

/**
 * Returns first node for which the given predicate returns true.
 *
 * @param[in] head          Head of the list to search.
 * @param[in] predicate     Search predicate.
 * @param[in] context       Context for predicate function.
 *
 * @return Pointer to found node.
 * @retval NULL if node matching given predicate was not found in the list.
 */
static inline struct intrusive_list *intrusive_list_find_first(struct intrusive_list *head,
        list_predicate predicate, void *context)
{
    kASSERT(head != NULL && predicate != NULL);

    struct intrusive_list *current = head;
    do {
        if (predicate(current, context)) {
            return current;
        }
    } while ((current = current->next));

    return NULL;
}

/**
 * Deletes entry from list.
 *
 * @retval New head of list.
 */
static inline struct intrusive_list *intrusive_list_delete(struct intrusive_list *head, struct intrusive_list *node)
{
    if (node == head) {
        // List is made of a single item to be deleted
        if (node == node->next) {
            return NULL;
        }
        head = node->next;
    }

    intrusive_list_unlink_node(node);
    return head;
}

/**
 * Deletes first entry for which the predicate returns true.
 *
 * @retval New head of list.
 */
static inline struct intrusive_list *intrusive_list_delete_first(struct intrusive_list *head,
        list_predicate predicate, void *context)
{
    struct intrusive_list *to_delete =
        intrusive_list_find_first(head, predicate, context);
    if (to_delete != NULL) {
        if (to_delete == head) {
            head = to_delete->next;
        }

        intrusive_list_unlink_node(to_delete);
        return head;
    }

    return head;
}

/**
 * Inserts node after prev.
 *
 * @param[in] prev  List node to insert after.
 * @param[in] node  List node to insert.
 *
 * @return Pointer to inserted node.
 */
static inline struct intrusive_list *intrusive_list_insert(struct intrusive_list *prev,
        struct intrusive_list *node)
{
    kASSERT(prev != NULL && node != NULL);

    struct intrusive_list *tmp = node->next;
    node->next = prev->next;
    if (node->next != NULL) {
        node->next->prev = node;
    }
    node->prev = prev;
    prev->next = node;
    node->next = tmp;
    if (tmp != NULL) {
        tmp->prev = node;
    }

    return node;
}

#ifdef __cplusplus
}
#endif

#endif // OTRIX_INTRUSIVE_LIST_H
