#ifndef LIST_H_
#define LIST_H_

/**
 * @file list.t
 *
 * Implements circular intrusive doubly linked list list.
 */

#include <stddef.h>
#include <stdbool.h>
#include "assert.h"

typedef struct list {
    struct list *next;
    struct list *prev;
} list_t;

#define list_entry(list_ptr, struct_name, member_name) \
    ((struct_name*)((char*)(list_ptr) - offsetof(struct_name, member_name)))

typedef bool (*list_predicate)(const list_t *node, void *context);

static inline list_t *list_init(list_t *node)
{
    assert(node != NULL);
    node->prev = node;
    node->next = node;
    return node;
}

/**
 * Returns tail of the list.
 *
 * @note The list is assumed to be circular.
 */
static inline list_t *list_get_tail(list_t *head)
{
    assert(head != NULL);
    return head->prev;
}

/**
 * @param[in] head Start of the list.
 * @param[in] tail Node to attach at the end of the list pointed by head.
 *
 * @return Tail.
 * @retval NULL On failure.
 */
static inline list_t *list_push_back(list_t *head, list_t *tail)
{
    assert(head != NULL && tail != NULL);

    list_t *last = list_get_tail(head);

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
static inline void list_unlink_node(list_t *node)
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
static inline list_t *list_find_first(list_t *head, list_predicate predicate,
        void *context)
{
    assert(head != NULL && predicate != NULL);

    list_t *current = head;
    do {
        if (predicate(current, context)) {
            return current;
        }
    } while ((current = current->next));

    return NULL;
}

/**
 * Deletes first entry for which the predicate returns true.
 *
 * @retval New head of list.
 */
static inline list_t *list_delete_first(list_t *head, list_predicate predicate,
        void *context)
{
    list_t *to_delete = list_find_first(head, predicate, context);
    if (to_delete != NULL) {
        if (to_delete == head) {
            head = to_delete->next;
        }

        list_unlink_node(to_delete);
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
static inline list_t *list_insert(list_t *prev, list_t *node)
{
    assert(prev != NULL && node != NULL);

    list_t *tmp = node->next;
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

#endif // INTRUSIVE_LIST_H_
