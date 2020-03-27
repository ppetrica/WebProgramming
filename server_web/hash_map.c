#include "hash_map.h"
#include <string.h>
#include <stdlib.h>


int hash_map_init(struct hash_map *map, size_t n_buckets) {
    if (!map || !n_buckets)
        return -1;

    map->buckets = (struct hash_map_bucket_node **)malloc(n_buckets * sizeof(struct hash_map_bucket_node *));
    if (!map->buckets)
        return -1;

    memset(map->buckets, 0, n_buckets * sizeof(struct hash_map_bucket_node *));

    map->n_buckets = n_buckets;

    return 0;
}


static size_t hash_func(const char *key) {
    size_t h = 0;
    while (*key) {
        h += *key;

        ++key;
    }

    return h;
}


static struct hash_map_bucket_node *create_node(const char *key, const char *value) {
    struct hash_map_bucket_node *node = (struct hash_map_bucket_node *)malloc(sizeof(struct hash_map_bucket_node));
    if (!node)
        return 0;

    size_t l = strlen(key) + 1;
    node->key = (char *)malloc(l + 1);
    if (!node->key) {
        free(node);

        return 0;
    }

    memcpy(node->key, key, l);

    l = strlen(value) + 1;
    node->value = (char *)malloc(l + 1);
    if (!node->value) {
        free(node->key);
        free(node);

        return 0;
    }

    memcpy(node->value, value, l);

    node->next = NULL;

    return node;
}


int hash_map_add(struct hash_map *map, const char *key, const char *value) {
    size_t index = hash_func(key) % map->n_buckets;

    struct hash_map_bucket_node *node = create_node(key, value);
    if (!node)
        return -1;

    node->next = map->buckets[index];

    map->buckets[index] = node;

    return 0;
}


struct hash_map_bucket_node *hash_map_get(const struct hash_map *map, const char *key) {
    size_t index = hash_func(key) % map->n_buckets;

    struct hash_map_bucket_node *node = map->buckets[index];
    while (node && strcmp(key, node->key))
        node = node->next;

    return node;
}


void hash_map_free(struct hash_map *map) {
    if (map) {
        for (size_t i = 0; i < map->n_buckets; ++i) {
            struct hash_map_bucket_node *head = map->buckets[i];
            while (head) {
                free(head->key);
                free(head->value);

                struct hash_map_bucket_node *freed = head;
                head = head->next;

                free(freed);
            }
        }
        free(map->buckets);
    }
}

