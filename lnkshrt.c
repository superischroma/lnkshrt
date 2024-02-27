#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/mman.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>

#include "lnkshrt.h"

#define PORT 8080
#define BUFFER_SIZE 52430000

#define QUERY_BUFFER_SIZE 16

bool crd_endpoint_predicate(char c)
{
    return c == '?' || c == ' ';
}

bool crd_query_name_predicate(char c)
{
    return c == '=' || c == '&';
}

bool crd_query_value_predicate(char c)
{
    return crd_query_name_predicate(c) || c == ' ';
}

bool crd_header_name_predicate(char c)
{
    return c == ':';
}

bool crd_header_value_predicate(char c)
{
    return c == '\r' || c == '\n';
}

bool string_end_predicate(char c)
{
    return c == '\0';
}

bool space_predicate(char c)
{
    return c == ' ';
}

unsigned empty_query_name_map(char* name)
{
    return 0;
}

void create_shortened_link(char* destination, char* endpoint)
{
    char buffer[1024];
    snprintf(buffer, 1024, "links/%s", endpoint);
    FILE* file = fopen(buffer, "w");
    fputs(destination, file);
    fclose(file);
}

char* get_shortened_link_destination(char* endpoint)
{
    if (strlen(endpoint) <= 1)
        return NULL;
    char buffer[1024];
    snprintf(buffer, 1024, "links%s", endpoint);
    FILE* file = fopen(buffer, "r");
    if (file == NULL)
        return NULL;
    int dest_len = fread(buffer, sizeof(char), 1024, file);
    char* destination = malloc(dest_len + 1);
    strncpy(destination, buffer, dest_len);
    return destination;
}

#define malloc_const_response(str) \
    resp->data = malloc(sizeof str); \
    strcpy(resp->data, str);

// GET /
response_details_t* index_get_handler(request_details_t* rd)
{
    response_details_t* resp = calloc(1, sizeof *resp);
    resp->mime = "text/html";
    resp->code = "200 OK";
    resp->data = read_entire_file("front/index.html");
    return resp;
}

// GET /resources/style.css
response_details_t* stylesheet_get_handler(request_details_t* rd)
{
    response_details_t* resp = calloc(1, sizeof *resp);
    resp->mime = "text/css";
    resp->code = "200 OK";
    resp->data = read_entire_file("front/style.css");
    return resp;
}

// GET /resources/script.js
response_details_t* script_get_handler(request_details_t* rd)
{
    response_details_t* resp = calloc(1, sizeof *resp);
    resp->mime = "text/javascript";
    resp->code = "200 OK";
    resp->data = read_entire_file("front/script.js");
    return resp;
}

// 404 handler
response_details_t* not_found_handler(request_details_t* rd)
{
    response_details_t* resp = calloc(1, sizeof *resp);
    resp->mime = "text/plain";
    resp->code = "404 Not Found";
    malloc_const_response("The requested resource could not be found on this server.");
    return resp;
}

// POST /api/shorten
response_details_t* shorten_post_handler(request_details_t* rd)
{
    response_details_t* resp = calloc(1, sizeof *resp);
    char* body = rd->body;
    resp->mime = "text/plain";
    unsigned dest_len = 0;
    char* destination = copy_until(body, &dest_len, space_predicate);
    if (!rd->body[dest_len]) // only found destination, no endpoint
    {
        resp->code = "400 Bad Request";
        malloc_const_response("No endpoint provided.");
        return resp;
    }
    body += dest_len + 1;
    unsigned endpoint_len = 0;
    char* endpoint = copy_until(body, &endpoint_len, string_end_predicate);
    create_shortened_link(destination, endpoint);
    free(destination);
    free(endpoint);
    resp->code = "201 Created";
    malloc_const_response("Success.");
    return resp;
}

void* find_request_handler(request_details_t* rd)
{
    if (rd->method == METHOD_GET)
    {
        if (!strcmp(rd->endpoint, "/"))
            return index_get_handler;
        if (!strcmp(rd->endpoint, "/resources/style.css"))
            return stylesheet_get_handler;
        if (!strcmp(rd->endpoint, "/resources/script.js"))
            return script_get_handler;
    }
    if (rd->method == METHOD_POST)
    {
        if (!strcmp(rd->endpoint, "/api/shorten"))
            return shorten_post_handler;
    }
    return not_found_handler;
}

// raw_req should be a raw HTTP request string like:
// <method> <endpoint>[query] HTTP/1.1
// <header-name>: <header-value>
// ...
//
// [body]
char* construct_request_details(request_details_t* rd, char* raw_req)
{
    if (!nstrcmp(raw_req, "GET", 3))
        rd->method = METHOD_GET;
    else if (!nstrcmp(raw_req, "POST", 4))
        rd->method = METHOD_POST;

    for (; *raw_req != ' '; ++raw_req); // jump to right before the endpoint
    ++raw_req; // go the endpoint
    unsigned endpoint_len = 0;
    char* endpoint = rd->endpoint = copy_until(raw_req, &endpoint_len, crd_endpoint_predicate);
    raw_req += endpoint_len;
    rd->query = map_init();
    rd->headers = map_init();
    if (*raw_req == '?')
    {
        while (*(raw_req++) != ' ')
        {
            unsigned query_name_len = 0;
            char* query_name = copy_until(raw_req, &query_name_len, crd_query_name_predicate);
            raw_req += query_name_len;
            if (*raw_req == '&')
            {
                map_put(rd->query, query_name, "true");
                continue;
            }
            ++raw_req;
            unsigned query_value_len = 0;
            char* query_value = copy_until(raw_req, &query_value_len, crd_query_value_predicate);
            raw_req += query_value_len;
            map_put(rd->query, query_name, query_value);
        }
    }
    for (; *raw_req != '\n'; ++raw_req); // jump after the HTTP/1.1
    ++raw_req; // jump past the newline
    while (*raw_req != '\r' && *raw_req != '\n')
    {
        unsigned header_name_len = 0;
        char* header_name = copy_until(raw_req, &header_name_len, crd_header_name_predicate);
        raw_req += header_name_len + 2;
        unsigned header_value_len = 0;
        char* header_value = copy_until(raw_req, &header_value_len, crd_header_value_predicate);
        raw_req += header_value_len;
        if (*raw_req == '\r')
            ++raw_req;
        ++raw_req;
        map_put(rd->headers, header_name, header_value);
    }
    if (*raw_req == '\r')
        ++raw_req;
    ++raw_req;
    unsigned body_len = 0;
    rd->body = copy_until(raw_req, &body_len, string_end_predicate);
    return raw_req;
}


void free_request_details(request_details_t* rd)
{
    free(rd->endpoint);
    map_deep_free(rd->query);
    map_deep_free(rd->headers);
    free(rd->body);
    free(rd);
}

void free_response_details(response_details_t* rd)
{
    free(rd->data);
    free(rd);
}

void* handle_client(void* arg)
{
    int client_fd = *((int*) arg);
    char* buffer = malloc(BUFFER_SIZE);
    char* buffer_start = buffer;

    ssize_t bytes_received = recv(client_fd, buffer, BUFFER_SIZE, 0);

    if (bytes_received > 0)
    {
        printf("%.*s\n", bytes_received, buffer_start);
        request_details_t* rd = calloc(1, sizeof *rd);
        buffer = construct_request_details(rd, buffer);
        printf("%d, %s\n", rd->method, rd->endpoint);
        char* destination = get_shortened_link_destination(rd->endpoint);
        char* resp_buffer = malloc(BUFFER_SIZE);
        if (destination)
        {
            int resp_len = snprintf(resp_buffer, BUFFER_SIZE, "HTTP/1.1 301 Moved Permanently\r\nLocation: %s", destination);
            send(client_fd, resp_buffer, resp_len, 0);
            free(destination);
        }
        else
        {
            response_details_t* (*handler)(request_details_t*) = find_request_handler(rd);
            response_details_t* resp = handler(rd);
            int resp_len = snprintf(resp_buffer, BUFFER_SIZE, "HTTP/1.1 %s\r\nContent-Type: %s\r\n\r\n%s", resp->code, resp->mime, resp->data);
            send(client_fd, resp_buffer, resp_len, 0);
            free_response_details(resp);
        }
        free(resp_buffer);
        free_request_details(rd);
    }

    close(client_fd);
    free(buffer_start);
    return NULL;
}

int main(void)
{
    int server_fd;
    struct sockaddr_in server_addr;

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("error: socket failed\n");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr*) &server_addr, sizeof(server_addr)) < 0)
    {
        printf("error: bind failed\n");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 10) < 0)
    {
        printf("error: listen failed\n");
        exit(EXIT_FAILURE);
    }

    printf("server now listening on port %d\n", PORT);

    while (true)
    {
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        int* client_fd = malloc(sizeof(int));

        if ((*client_fd = accept(server_fd, (struct sockaddr*) &server_addr, &client_addr_len)) < 0)
        {
            printf("error: accept failed\n");
            continue;
        }

        pthread_t thread_id;
        pthread_create(&thread_id, NULL, handle_client, (void*) client_fd);
        pthread_detach(thread_id);
    }

    close(server_fd);
    return 0;
}