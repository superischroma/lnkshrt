#include <netinet/in.h>
#include <sys/socket.h>
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

unsigned empty_query_name_map(char* name)
{
    return 0;
}

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

void* find_request_handler(request_details_t* rd)
{
    if (rd->method == METHOD_GET)
    {
        if (!strcmp(rd->endpoint, "/"))
            return index_get_handler;
        if (!strcmp(rd->endpoint, "/resources/style.css"))
            return stylesheet_get_handler;
    }
    return NULL;
}

void* find_query_name_map(request_details_t* rd)
{
    if (!strcmp(rd->endpoint, "/") ||
        !strcmp(rd->endpoint, "/resources/style.css"))
        return empty_query_name_map;
    return NULL;
}

char* construct_request_details(request_details_t* rd, char* raw_req)
{
    if (!nstrcmp(raw_req, "GET", 3))
        rd->method = METHOD_GET;
    else if (!nstrcmp(raw_req, "POST", 4))
        rd->method = METHOD_POST;

    for (; *raw_req != ' '; ++raw_req); // jump to right before the endpoint
    ++raw_req;
    unsigned endpoint_len = 0;
    char* endpoint = rd->endpoint = copy_until(raw_req, &endpoint_len, crd_endpoint_predicate);
    raw_req += endpoint_len;
    rd->query = calloc(QUERY_BUFFER_SIZE, sizeof(char*));
    for (unsigned i = 0; i < 16; ++i)
        rd->query[i] = NULL;
    if (*raw_req == '?')
    {
        while (*(raw_req++) != ' ')
        {
            unsigned query_name_len = 0;
            char* query_name = copy_until(raw_req, &query_name_len, crd_query_name_predicate);
            raw_req += query_name_len;
            unsigned (*query_name_map)(char*) = find_query_name_map(rd);
            unsigned query_index = query_name_map(query_name);
            free(query_name);
            if (*(raw_req++) == '&')
            {
                rd->query[query_index] = "true";
                continue;
            }
            unsigned query_value_len = 0;
            char* query_value = copy_until(raw_req, &query_value_len, crd_query_value_predicate);
            raw_req += query_value_len;
            rd->query[query_index] = query_value;
        }
    }
    return raw_req;
}


void free_request_details(request_details_t* rd)
{
    free(rd->endpoint);
    for (unsigned i = 0; i < 16; ++i)
        free(rd->query[i]);
    free(rd->query);
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
        request_details_t* rd = calloc(1, sizeof *rd);
        buffer = construct_request_details(rd, buffer);
        printf("%d, %s\n", rd->method, rd->endpoint);
        response_details_t* (*handler)(request_details_t*) = find_request_handler(rd);
        response_details_t* resp = handler(rd);
        char* resp_buffer = malloc(BUFFER_SIZE);
        int resp_len = snprintf(resp_buffer, BUFFER_SIZE, "HTTP/1.1 %s\r\nContent-Type: %s\r\n\r\n%s", resp->code, resp->mime, resp->data);
        send(client_fd, resp_buffer, resp_len, 0);
        free(resp_buffer);
        free_response_details(resp);
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