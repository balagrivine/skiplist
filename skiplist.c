#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define ERR_NOT_FOUND -2
#define ERR_MEM -3
#define SKIP_LIST_SENTINEL_NODE_FLAG 0x01

typedef struct Node{
    uint8_t flags;
    uint8_t *key;
    uint8_t *value;
    size_t key_size;
    size_t value_size;
    struct Node *forward[];
} Node;

typedef struct List{
    int max_level;
    int level;
    float probability;
    Node *head;
    Node *tail;
} List;

int skip_list_new(List **skip_list, int max_level, float probability);
int skip_list_insert_node(List *list, uint8_t *key, uint32_t key_size, uint8_t *value,
                            uint32_t value_size);
int skip_list_generate_random_level(float probability, int max_level);
Node *skiplist_find_node(List *list, Node **update, uint8_t *key, size_t key_size);

static inline int compare_keys(uint8_t *key1, size_t key1_size, uint8_t *key2, size_t key2_size){
    size_t min_size = key1_size < key2_size ? key1_size : key2_size;

    int cmp = memcmp(key1, key2, min_size);

    if (cmp != 0) return cmp;

    if (key1_size < key2_size) return -1;

    if (key1_size > key2_size) return 1;

    return 0;
}

static inline Node *skip_list_create_node(List *list, uint8_t *key, uint32_t key_size,
                                            uint8_t *value, uint32_t value_size)
{
    if(!list || !key || key_size <= 0 || !value || value_size <= 0){
        return NULL;
    }

    size_t node_size = (sizeof(Node*) * (sizeof(Node) + 1));
    Node *new_node = (Node*)malloc(node_size);
    if (!new_node) return NULL;

    for(int i = 0; i < list->level; i++){
        new_node->forward[i] = list->tail;
    }

    new_node->flags = 0;
    new_node->key = key;
    new_node->key_size = key_size;
    new_node->value_size = value_size;
    new_node->value = value;

    return new_node;
}

int skip_list_insert_node(List *list, uint8_t *key, uint32_t key_size, uint8_t *value,
                            uint32_t value_size)
{
    if(!list || !key || key_size <= 0 || !value || value_size <= 0){
        return -1;
    }

    Node *update[list->max_level];

    Node *target = skiplist_find_node(list, update, key, key_size);
    if (!(target->flags & SKIP_LIST_SENTINEL_NODE_FLAG) &&
        compare_keys(target->key, target->key_size, key, key_size) == 0)
    {
        uint8_t *val = (uint8_t*)malloc(sizeof(target->key_size));
        if (!val){
            return ERR_MEM;
        }

        memcpy(target->value, val, target->value_size);
        target->value_size = value_size;

        return 0;
    }

    int level = skip_list_generate_random_level(list->probability, list->max_level);
    if(level > list->level){
        for (int i = list->level; i < level; i++){
            update[i] = list->head;
        }

        list->level = level;
    }

    Node *new_node = skip_list_create_node(list, key, key_size, value, value_size);
    if (!new_node) return ERR_MEM;

    for (int i = 0; i < list->level; i++){
        new_node->forward[i] = update[i]->forward[i];
        update[i]->forward[i] = new_node;
    }

    return 0;
}

int skip_list_new(List **skiplist, int max_level, float probability){
    if(!max_level || probability < 0.0f || probability >= 1.0f){
        return -1;
    }

    *skiplist = NULL;

    List *new_list = (List *)malloc(sizeof(List));
    if (!new_list){
        return -1;
    }

    size_t node_size = sizeof(Node) + (sizeof(Node*) * (max_level + 1));
    Node *header = (Node *)malloc(node_size);
    if (!header){
        free(new_list);

        return -1;
    }

    Node *tail = (Node *)malloc(node_size);
    if (!header){
        free(new_list);
        free(header);

        return -1;
    }

    for (int i = 0; i <= max_level; i++){
        header->forward[i] = tail;
        tail->forward[i] = tail;
    }

    header->flags |= SKIP_LIST_SENTINEL_NODE_FLAG;
    tail->flags |= SKIP_LIST_SENTINEL_NODE_FLAG;

    new_list->head = header;
    new_list->tail = tail;
    new_list-> level = 1;
    new_list->max_level = max_level;
    new_list->probability = probability;

    *skiplist = new_list;

    return 0;
}

Node *skiplist_find_node(List *list, Node **update, uint8_t *key, size_t key_size){
    if(!list || !key || key_size <= 0){
        return NULL;
    }

    Node *current = list->head;

    for(int i = list->level - 1; i >= 0; i--){
        Node *next = current->forward[i];
        while(!(next->flags & SKIP_LIST_SENTINEL_NODE_FLAG) && compare_keys(next->key, next->key_size, key, key_size) < 0){
            next = current;
            current = current->forward[i];
        }

        if (update) update[i] = current;
    }

    return current->forward[0];
}

int skip_list_generate_random_level(float probability, int max_level){
    int level = 1;

    while((rand() / (double)RAND_MAX) <= probability && level < max_level){
        level++;
    }

    return level;
}

int main(void){
    List *list = NULL;
    const int max_level = 10;
    const float probability = 0.5;

    int rc = skip_list_new(&list, max_level, probability);
    if (rc != 0){
        return -1;
    }

    uint8_t *key = (uint8_t*)"key";
    uint8_t *val = (uint8_t*)"value";
    size_t key_size = strlen((char*)key);
    size_t val_size = strlen((char*)val);

    rc = skip_list_insert_node(list, key, key_size, val, val_size);
    if (rc != 0){
        printf("Skiplist insert returned with error code %d\n", rc);
        return -1;
    }

    Node *node = skiplist_find_node(list, NULL, key, key_size);
    if (!node) {
        printf("Invalid args\n");
        return -1;
    }

    if (!(node->flags & SKIP_LIST_SENTINEL_NODE_FLAG) &&
        compare_keys(node->key, node->key_size, key, key_size) == 0) {
        printf("Key found; key = %.*s, value = %.*s\n", (int)key_size, key, (int)val_size, val);
    } else{
        printf("Key not found\n");
        return -1;
    }

    return 0;
}
