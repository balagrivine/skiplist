#ifndef __SKIPLIST_H__
#define __SKIPLIST_H__

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define SENTINEL_NODE_FLAG 0x01

/* skiplist_get_value returns this when the key is absent, to keep a miss
 * distinguishable from a bad argument or an allocation failure (-1) */
#define SKIPLIST_ERR_NOT_FOUND -2

typedef struct node {
    uint8_t flags;
    uint8_t* key;
    uint8_t* value;
    size_t key_size;
    size_t value_size;
    struct node* forward[];
} node;

typedef struct list {
    int current_level;
    int max_level;
    float probability;
    struct node* head;
    struct node* tail;
} list;

/*
 * skiplist_new
 * initializes a new skip list, passing ownership of the new list to the caller
 * @param probability promition probability to randomize level generation
 * @param the ceiling to cap level growth in the list
 * @param skiplist stores the newly initialized list
 * return 0 on success, -1 on failure
 */
int skiplist_new(float probability, int max_level, list** skiplist);

/*
 * skiplist_create_node
 * initializes a new node to be inserted into the skiplist
 * the node is allocated with exactly @level forward pointers; its height is
 * not stored, since a node reached at level i is by construction taller than i
 * @param key the node key in bytes
 * @param key_size the size of the key
 * @param value the node value in bytes
 * @param value_size size of the value
 * @param level the height of the node, must be >= 1
 * return initialized node on success, NULL of failure
 */
node* skiplist_create_node(uint8_t* key, size_t key_size, uint8_t* value, size_t value_size,
                           int level);

/*
 * skiplist_get_value
 * finds a node that contains an exact match to the key,
 * and returns its value and value size
 * @param list the list to search the predecessor node
 * @param key the target key
 * @param key_size the target key_size
 * @param value receives a malloc'd copy of the value, owned by the caller
 * @param value_size the size of the value
 * return 0 on success, SKIPLIST_ERR_NOT_FOUND if absent, -1 on failure
 */
int skiplist_get_value(list* list, uint8_t* key, size_t key_size, uint8_t** value,
                       size_t* value_size);

/*
 * skiplist_insert_value
 * inserts a new value into the skiplist
 * @param list the list to insert the node into
 * @param key the key to be inserted
 * @param key_size size of the key
 * @param value the value to be inserted
 * @param value_size the size of the value
 * return 0 on success, -1 on failure
 */
int skiplist_insert_value(list* list, uint8_t* key, size_t key_size, uint8_t* value,
                          size_t value_size);

/*
 * skiplist_delete
 * Finds a value associated with key, and deletes its node
 * @param list the list to delete the node from
 * @param key the key to be used for lookup
 * @param key_size the size of the key
 * return 0 on success, -1 on failure
 */
int skiplist_delete(list* list, uint8_t* key, size_t key_size);

/*
 * skiplist_destroy
 * Destructs a skiplists structure, reclaiming all its allocated memory
 * @param list the list to be destroyed
 * return 0 on success, -1 on failure
 */
int skiplist_destroy(list** list);

#endif /* __SKIPLIST_H__ */
