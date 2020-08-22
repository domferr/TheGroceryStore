/**
 * Implementazione di una double linked list FIFO.
 */

#include "../include/list.h"
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>

//TODO fare questa doc
static void add_tail(list_t *list, node_t *node);
static void add_head(list_t *list, node_t *node);
static void *remove_node(list_t *list, node_t *node);
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

static void add_head(list_t *list, node_t *node) {
    if (node != NULL) {
        node->prev = NULL;
        node->next = list->head;
        if (list->head == NULL) {
            list->tail = node;
        } else {
            (list->head)->prev = node;
        }
        list->head = node;
        list->size++;
    }
}

static void add_tail(list_t *list, node_t *node) {
    if (node != NULL) {
        node->next = NULL;
        node->prev = list->tail;
        if (list->tail == NULL) {
            list->head = node;
        } else {
            list->tail->next = node;
        }
        list->tail = node;
        list->size++;
    }
}

int push(list_t *list, void *elem) {
    node_t *node = (node_t *) malloc(sizeof(node_t));
    if (node == NULL)
        return -1;
    node->elem = elem;
    add_head(list, node);
    return 0;
}

static void *remove_node(list_t *list, node_t *node) {
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
    return remove_node(list, list->tail);
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

void append(list_t *l1, list_t *l2) {
    if (l1 && l2) {
        add_tail(l1, l2->head);
        free(l2);
    }
}

int mergesort(list_t *first, list_t *second, int (*compare)(void*, void*)) {
    if (!first || !second || !compare) {
        errno = EINVAL;
        return -1;
    }
    node_t *n1 = first->tail, *n2 = second->tail, *curr;
    list_t merged = {NULL, NULL, 0};

    while(n1 != NULL || n2 != NULL) {
        curr = NULL;
        if (n1 == NULL) {
            curr = n2;
            n2 = n2->prev;
        } else if (n2 == NULL) {
            curr = n1;
            n1 = n1->prev;
        } else if (compare(n1->elem, n2->elem) <= 0) {  //il primo è minore o uguale al secondo
            curr = n1;
            n1 = n1->prev;
        } else {
            curr = n2;
            n2 = n2->prev;
        }
        add_head(&merged, curr);
    }

    first->head = merged.head;
    first->tail = merged.tail;
    first->size = merged.size;
    free(second);
    return 0;
}

list_t *mergesort_k_lists(list_t **lists, int k, int (*compare)(void*, void*)) {
    int i, j, last = k-1;
    int total = 0;
    for(i=0; i<k; i++) {
        total += lists[i]->size;
    }
    i = 0;
    while (last != 0) {
        j = last;
        i = 0;
        while(i < j) {
            if (mergesort(lists[i], lists[j], compare) == -1)
                return NULL;
            j--;
            i++;
            if (i >= j)
                last = j;
        }
    }
    if (total != lists[0]->size)
        printf("MERGE SORT SBAGLIATOOOOOOOO\n");
    return lists[0];
}