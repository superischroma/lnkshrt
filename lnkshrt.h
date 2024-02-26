#ifndef LNKSHRT_H
#define LNKSHRT_H

/* lnkshrt.c */

#define METHOD_GET 0
#define METHOD_POST 1

#define MIME_TEXT_HTML 0

#define RESPONSE_CODE_OK 200

typedef struct request_details_t
{
    unsigned method;
    char* endpoint;
    char** query;
} request_details_t;

typedef struct response_details_t
{
    char* mime;
    char* code;
    char* data;
} response_details_t;

/* util.c */
int nstrcmp(char* s1, char* s2, int n);
char* copy_until(char* str, unsigned* copied, bool (*predicate)(char));
char* read_entire_file(char* filename);

#endif