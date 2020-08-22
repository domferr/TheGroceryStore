/**
 * Implementazione di una double linked list FIFO.
 */

#include "../include/list.h"
#include <stdlib.h>

static int internal_foreach(node_t *node, int (*fun)(void*, void*), void *args);

list_t *list_create(void) {
    list_t *list = (list_t*) malloc(sizeof(list_t));
    if (list == NULL)
        return NULL;
    list->size = 0;
    list->head = NULL;
    list->tail = NULL;
    return list;
}

int list_destroy(list_t *list, void (*free_fun)(void *)) {
    clear(list, free_fun);
    free(list);
    return 0;
}

int push(list_t *list, void *elem) {
    node_t *node = (node_t *) malloc(sizeof(node_t));
    if (node == NULL)
        return -1;
    node->elem = elem;
    node->prev = NULL;
    node->next = list->head;
    if (list->head == NULL) {
        list->tail = node;
    } else {
        (list->head)->prev = node;
    }

    list->head = node;
    list->size++;
    return 0;
}

static void *remove_n(list_t *list, node_t *node) {
    void *elem = NULL;
    node_t *prev_node;
    node_t *next_node;
    if (node != NULL) {
        elem = node->elem;
        prev_node = node->prev;
        next_node = node->next;
        if (list->head == node)
            list->head = next_node;
        if (list->tail == node)
            list->tail = prev_node;
        if (prev_node != NULL)
            prev_node->next = next_node;
        if (next_node != NULL)
            next_node->prev = prev_node;
        list->size--;
        free(node);
    }
    return elem;
}

void *pop(list_t *list) {
    return remove_n(list, list->tail);
}

void clear(list_t *list, void (*free_fun)(void*)) {
    void *elem;
    while(list->size > 0) {
        elem = pop(list);
        if (free_fun != NULL)
            free_fun(elem);
    }
}

int foreach(list_t *list, int (*fun)(void*, void*), void *args) {
    return internal_foreach(list->tail, fun, args);
}

static int internal_foreach(node_t *node, int (*fun)(void*, void*), void *args) {
    if (node != NULL) {
        if (fun(node->elem, args) == -1)
            return -1;
        return internal_foreach(node->prev, fun, args);
    }
    return 0;
}

void merge(list_t *l1, list_t *l2) {
    if (l1 && l2) {
        if (l2->head) { //se l2 ha almeno un elemento
            l2->head->prev = l1->tail;
            if (l1->tail) { //se l1 ha almeno un elemento
                l1->tail->next = l2->head;
            } else {    //Se l1 è vuoto
                l1->head = l2->head;
            }
            l1->tail = l2->tail;
        }
        l1->size += l2->size;
        free(l2);
    }
}
