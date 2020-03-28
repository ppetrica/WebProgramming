#ifndef HASH_MAP_H_
#define HASH_MAP_H_


#include <stdlib.h>


struct hash_map_bucket_node {
    char *key;
    char *value;

    struct hash_map_bucket_node *next;
};


struct hash_map {
    struct hash_map_bucket_node **buckets;
    size_t n_buckets;
};


int hash_map_init(struct hash_map *map, size_t n_buckets);

void hash_map_free(struct hash_map *map);

struct hash_map_bucket_node *hash_map_get(const struct hash_map *map, const char *key);

int hash_map_add(struct hash_map *map, const char *key, const char *value);


#endif
