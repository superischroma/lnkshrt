#ifndef LNKSHRT_H
#define LNKSHRT_H

#include <stdbool.h>

/* map.c */

typedef struct map_t
{
    char** key;
    void** value;
    unsigned size;
    unsigned capacity;
} map_t;

map_t* map_init(void);
void* map_put(map_t* map, char* key, void* value);
void* map_get(map_t* map, char* key);
void map_print(map_t* map);
void map_free(map_t* map);
void map_deep_free(map_t* map);

/* lnkshrt.c */

#define METHOD_GET 0
#define METHOD_POST 1

#define MIME_TEXT_HTML 0

#define RESPONSE_CODE_OK 200

typedef struct request_details_t
{
    unsigned method;
    char* endpoint;
    map_t* query;
    map_t* headers;
    char* body;
} request_details_t;

typedef struct response_details_t
{
    char* mime;
    char* code;
    char* data;
} response_details_t;

char* construct_request_details(request_details_t* rd, char* raw_req);

/* util.c */

int nstrcmp(char* s1, char* s2, int n);
char* copy_until(char* str, unsigned* copied, bool (*predicate)(char));
char* read_entire_file(char* filename);
int fngets(char* buffer, int n, FILE* file);

#endif