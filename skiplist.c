#include "skiplist.h"

/*
 * compare_keys
 * orders two byte keys lexicographically, shorter key first on a common prefix
 * @param a first key
 * @param a_size size of the first key
 * @param b second key
 * @param b_size size of the second key
 * return <0 if a sorts before b, 0 if equal, >0 if a sorts after b
 */
static int compare_keys(const uint8_t* a, size_t a_size, const uint8_t* b, size_t b_size) {
    size_t min_size = a_size < b_size ? a_size : b_size;

    int cmp = memcmp(a, b, min_size);
    if (cmp != 0) {
        return cmp;
    }

    if (a_size < b_size) return -1;
    if (a_size > b_size) return 1;

    return 0;
}

static inline int node_is_sentinel(const node* n) { return n->flags & SENTINEL_NODE_FLAG; }

static inline int generate_random_level(float probability, int max_level) {
    int level = 1;

    while ((rand() / (double)RAND_MAX) <= probability && level < max_level) {
        level++;
    }

    return level;
}

int skiplist_new(float probability, int max_level, list** skiplist) {
    if (probability <= 0.0f || probability >= 1.0f || max_level <= 0 || !skiplist) {
        return -1;
    }

    list* new_list = (list*)malloc(sizeof(list));
    if (!new_list) {
        return -1;
    }

    // head/tail + forward pointers, one per level
    size_t total_size = sizeof(node) + (sizeof(node*) * (size_t)max_level);

    node* head = (node*)malloc(total_size);
    if (!head) {
        free(new_list);
        return -1;
    }

    node* tail = (node*)malloc(total_size);
    if (!tail) {
        free(head);
        free(new_list);
        return -1;
    }

    for (int i = 0; i < max_level; i++) {
        head->forward[i] = tail;
        tail->forward[i] = NULL;
    }

    head->flags = SENTINEL_NODE_FLAG;
    head->key = NULL;
    head->value = NULL;
    head->key_size = 0;
    head->value_size = 0;

    tail->flags = SENTINEL_NODE_FLAG;
    tail->key = NULL;
    tail->value = NULL;
    tail->key_size = 0;
    tail->value_size = 0;

    new_list->current_level = 1;
    new_list->max_level = max_level;
    new_list->probability = probability;
    new_list->head = head;
    new_list->tail = tail;

    *skiplist = new_list;

    return 0;
}

node* skiplist_create_node(uint8_t* key, size_t key_size, uint8_t* value, size_t value_size,
                           int level) {
    if (!key || key_size == 0 || !value || value_size == 0 || level <= 0) {
        return NULL;
    }

    size_t node_size = sizeof(node) + (sizeof(node*) * (size_t)level);

    node* new_node = (node*)malloc(node_size);
    if (!new_node) {
        return NULL;
    }

    uint8_t* kv_buffer = (uint8_t*)malloc(key_size + value_size);
    if (!kv_buffer) {
        free(new_node);
        return NULL;
    }

    memcpy(kv_buffer, key, key_size);
    memcpy(kv_buffer + key_size, value, value_size);

    new_node->flags = 0;
    new_node->key = kv_buffer;
    new_node->value = kv_buffer + key_size;
    new_node->key_size = key_size;
    new_node->value_size = value_size;

    /* A node at level i has i forward pointers, indexed from 0...i-1 */
    for (int i = 0; i < level; i++) {
        new_node->forward[i] = NULL;
    }

    return new_node;
}

/*
 * skiplist_find_predecessor_node
 * finds a node that immediately preceeds the node with key
 * @param list the list to search the predecessor node
 * @param key the target key
 * @param key_size the target key_size
 * @param update array to hold all predecessor node at each list level,
 *               filled for indices 0 .. current_level - 1
 * return the predecessor node on success, NULL on failure
 */
static node* skiplist_find_predecessor_node(list* list, uint8_t* key, size_t key_size,
                                            node** update) {
    if (!list || !key || key_size == 0) {
        return NULL;
    }

    node* current = list->head;
    for (int i = list->current_level - 1; i >= 0; i--) {
        node* next = current->forward[i];
        while (next && !node_is_sentinel(next) &&
               compare_keys(next->key, next->key_size, key, key_size) < 0) {
            current = next;
            next = current->forward[i];
        }

        if (update) {
            update[i] = current;
        }
    }

    return current;
}

int skiplist_get_value(list* list, uint8_t* key, size_t key_size, uint8_t** value,
                       size_t* value_size) {
    if (!list || !key || key_size == 0 || !value || !value_size) {
        return -1;
    }

    *value = NULL;
    *value_size = 0;

    node* pred = skiplist_find_predecessor_node(list, key, key_size, NULL);
    if (!pred) {
        return -1;
    }

    node* target = pred->forward[0];
    if (!target || node_is_sentinel(target) ||
        compare_keys(target->key, target->key_size, key, key_size) != 0) {
        return SKIPLIST_ERR_NOT_FOUND;
    }

    uint8_t* copy = (uint8_t*)malloc(target->value_size);
    if (!copy) {
        return -1;
    }

    memcpy(copy, target->value, target->value_size);

    *value = copy;
    *value_size = target->value_size;

    return 0;
}

int skiplist_insert_value(list* list, uint8_t* key, size_t key_size, uint8_t* value,
                          size_t value_size) {
    if (!list || !key || key_size == 0 || !value || value_size == 0) {
        return -1;
    }

    node* update[list->max_level];

    node* pred = skiplist_find_predecessor_node(list, key, key_size, update);
    if (!pred) {
        return -1;
    }

    node* target = pred->forward[0];
    if (target && !node_is_sentinel(target) &&
        compare_keys(target->key, target->key_size, key, key_size) == 0) {
        /* the key and value share one allocation, so a replacement value of a
         * different size needs a fresh buffer rather than a memcpy in place */
        uint8_t* kv_buffer = (uint8_t*)malloc(key_size + value_size);
        if (!kv_buffer) {
            return -1;
        }

        memcpy(kv_buffer, key, key_size);
        memcpy(kv_buffer + key_size, value, value_size);

        free(target->key);

        target->key = kv_buffer;
        target->value = kv_buffer + key_size;
        target->key_size = key_size;
        target->value_size = value_size;

        return 0;
    }

    int level = generate_random_level(list->probability, list->max_level);

    if (level > list->current_level) {
        for (int i = list->current_level; i < level; i++) {
            update[i] = list->head;
        }

        list->current_level = level;
    }

    node* new_node = skiplist_create_node(key, key_size, value, value_size, level);
    if (!new_node) {
        return -1;
    }

    /* link at the node's own height, not the list's */
    for (int i = 0; i < level; i++) {
        new_node->forward[i] = update[i]->forward[i];
        update[i]->forward[i] = new_node;
    }

    return 0;
}

int skiplist_delete(list* list, uint8_t* key, size_t key_size) {
    if (!list || key_size == 0 || !key) return -1;

    node* update[list->max_level];

    node* pred = skiplist_find_predecessor_node(list, key, key_size, update);
    if (!pred) return -1;

    node* target = pred->forward[0];

    if (pred->forward[0] &&
        compare_keys(key, key_size, pred->forward[0]->key, pred->forward[0]->key_size) == 0) {
        for (int i = 0; i < list->current_level; i++) {
            if (update[i]->forward[i] != target) break;

            update[i]->forward[i] = target->forward[i];
        }

        free(target->key);
        free(target);
    }

    while (list->current_level > 0 && list->head->forward[list->current_level]) {
        list->current_level--;
    }

    return 0;
}

int skiplist_destroy(list** list) {
    if (!list || !*list) return -1;

    node* current = (*list)->head;
    node* next_node = NULL;

    while (current != (*list)->tail) {
        next_node = current->forward[0];
        free(current->key);
        free(current);

        current = next_node;
    }

    free((*list)->tail);
    free(*list);
    *list = NULL;

    return 0;
}
