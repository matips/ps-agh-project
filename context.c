#include <stdlib.h>
#include <dirent.h>
#include <stdio.h> 
#include "context.h"
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

unsigned long hash(char *str) {
    unsigned long hash = 5381;
    int c;

    while ((c = *str++))
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash;
}

int context_hash(void) {
    DIR *d;
    struct dirent *dir;
    d = opendir("/dev/");
    long counted_hash = 0;
    if (d) {
        while ((dir = readdir(d)) != NULL) {
            counted_hash += hash(dir->d_name);
        }

        closedir(d);
    }

    return counted_hash;
}

char *transform_path(char *root, const char *path) {
    int size, root_length;
    char context_hash_str[20], *result;

    root_length = size = strlen(root);
    if (root_length && root[root_length - 1] == '/') {
        root[root_length - 1] = 0;
    } else {
        root_length += 1;
    }
    int counted_hash = context_hash();
    counted_hash = (counted_hash < 0) ? (-1) * counted_hash : counted_hash;
    sprintf(context_hash_str, "%d", counted_hash);

    size += strlen(path);
    size += strlen(context_hash_str);
    size += 2;

    result = malloc(sizeof(char) * size);
    sprintf(result, "%s/%s", root, context_hash_str);
    struct stat st = {0};

    if (stat(result, &st) == -1) {
        mkdir(result, 0700);
    }

    sprintf(result, "%s/%s%s", root, context_hash_str, path);
    return result;
}


