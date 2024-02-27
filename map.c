#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "lnkshrt.h"

static unsigned long hash(char* str)
{
    unsigned long hash = 5381;
    int c;

    while (c = *str++)
        hash = ((hash << 5) + hash) + c;

    return hash;
}

static void map_realloc(map_t* map, unsigned capacity)
{
    map->key = realloc(map->key, capacity * sizeof(char*));
    map->key = realloc(map->value, capacity * sizeof(void*));
    for (unsigned i = map->capacity; i < capacity; ++i)
        map->key[i] = map->value[i] = NULL;
    map->capacity = capacity;
}

map_t* map_init(void)
{
    map_t* map = calloc(1, sizeof *map);
    map->capacity = 50;
    map->key = calloc(map->capacity, sizeof(char*));
    map->value = calloc(map->capacity, sizeof(void*));
    for (unsigned i = 0; i < map->capacity; ++i)
        map->key[i] = map->value[i] = NULL;
    map->size = 0;
    return map;
}

void* map_put(map_t* map, char* key, void* value)
{
    if (map->size >= map->capacity * 0.5)
        map_realloc(map, map->capacity * 2);
    unsigned long index = hash(key) % map->capacity;
    for (;; index = (index + 1 == map->capacity ? 0 : index + 1))
    {
        if (map->key[index] == NULL)
        {
            map->key[index] = key;
            map->value[index] = value;
            ++(map->size);
            break;
        }
    }
    return value;
}

void* map_get(map_t* map, char* key)
{
    unsigned long index = hash(key) % map->capacity;
    for (;; index = (index + 1 == map->capacity ? 0 : index + 1))
    {
        if (map->key[index] != NULL && !strcmp(map->key[index], key))
            return map->value[index];
    }
    return NULL;
}

void map_print(map_t* map)
{
    printf("map{");
    for (unsigned i = 0, c = true; i < map->capacity; ++i)
    {
        if (map->key[i] != NULL)
        {
            if (!c)
                printf(", ");
            c = false;
            printf("%s=%s", map->key[i], map->value[i]);
        }
    }
    printf("}\n");
}

void map_free(map_t* map)
{
    free(map->key);
    free(map->value);
    free(map);
}

void map_deep_free(map_t* map)
{
    for (unsigned i = 0; i < map->capacity; ++i)
        if (map->key[i] != NULL)
        {
            free(map->key[i]);
            free(map->value[i]);
        }
    map_free(map);
}

