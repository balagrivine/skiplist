#include <assert.h>
#include <stdio.h>

#include "../skiplist.h"

void test_skiplist_new() {
    list* skiplist = NULL;
    float probability = 0.5;
    int max_level = 16;

    int rc = skiplist_new(probability, max_level, &skiplist);

    assert(rc == 0);
    assert(skiplist != NULL);
    assert(skiplist->max_level = max_level);
    assert(skiplist->head->flags & SENTINEL_NODE_FLAG);

    for (int i = 0; i < max_level; i++) {
        assert(skiplist->head->forward[i] != NULL);
    }

    free(skiplist->head);
    free(skiplist->tail);

    skiplist = NULL;
    probability = 1.5;
    max_level = 0;

    rc = skiplist_new(probability, max_level, &skiplist);
    assert(rc == -1);
}

void test_skiplist_create_node() {
    list* skiplist = NULL;
    float probability = 0.5;
    int max_level = 16;

    uint8_t* key = (uint8_t*)"key";
    uint8_t* value = (uint8_t*)"value";
    int level = 5;

    assert(skiplist_new(probability, max_level, &skiplist) == 0);

    node* new_node =
            skiplist_create_node(key, strlen((char*)key), value, strlen((char*)value), level);

    assert(new_node != NULL);
    assert(memcmp(key, new_node->key, new_node->key_size) == 0);
    assert(memcmp(value, new_node->value, new_node->value_size) == 0);

    for (int i = 0; i < level; i++) {
        assert(new_node->forward[i] == NULL);
    }

    key = (uint8_t*)"";
    value = (uint8_t*)"";
    level = 0;

    assert(skiplist_create_node(key, strlen((char*)key), value, strlen((char*)value), level) ==
           NULL);

    free(new_node->key);
    free(new_node);
}

void test_skiplist_insert() {
    list* skiplist = NULL;
    float probability = 0.5;
    int max_level = 16;

    uint8_t* key = (uint8_t*)"key";
    uint8_t* value = (uint8_t*)"value";

    assert(skiplist_new(probability, max_level, &skiplist) == 0);

    int rc = skiplist_insert_value(skiplist, key, strlen((char*)key), value, strlen((char*)value));
    assert(rc == 0);

    uint8_t* ret_value = NULL;
    size_t ret_val_size;

    assert(skiplist_get_value(skiplist, key, strlen((char*)key), &ret_value, &ret_val_size) == 0);
    assert(memcmp(value, ret_value, ret_val_size) == 0);

    value = (uint8_t*)"new_value";
    size_t new_val_size;

    assert(skiplist_insert_value(skiplist, key, strlen((char*)key), value, strlen((char*)value)) ==
           0);

    assert(skiplist_get_value(skiplist, key, strlen((char*)key), &ret_value, &new_val_size) == 0);
    assert(memcmp(value, ret_value, new_val_size) == 0);

    assert(skiplist_destroy(&skiplist) == 0);
}

int main(void) {
    test_skiplist_new();
    test_skiplist_create_node();
    test_skiplist_insert();
}
