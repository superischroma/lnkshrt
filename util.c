#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "lnkshrt.h"

int nstrcmp(char* s1, char* s2, int n)
{
    for (; n > 0 && *s1 && *s2; ++s1, ++s2, --n)
        if (*s1 != *s2)
            return *s1 - *s2;
    return 0;
}

char* copy_until(char* str, unsigned* copied, bool (*predicate)(char)) 
{
    for (char* s = str; !predicate(*s) && *s; ++s, ++(*copied));
    char* substr = malloc(*copied + 1);
    substr[*copied] = '\0';
    memcpy(substr, str, *copied);
}

char* read_entire_file(char* filename)
{
    int file_fd = open(filename, O_RDONLY);
    if (file_fd == -1)
        return NULL;
    struct stat file_stats;
    fstat(file_fd, &file_stats);
    off_t fsize = file_stats.st_size;
    char* buffer = malloc(fsize);
    read(file_fd, buffer, fsize);
    return buffer;
}