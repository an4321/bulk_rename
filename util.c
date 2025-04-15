#include <ctype.h>
#include <errno.h>
#include <limits.h> // for PATH_MAX
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "util.h"

void die(const char *message) {
    if (errno) {
        perror(message);
    } else {
        printf("ERROR: %s\n", message);
    }
    exit(1);
}

char *copy_str(char *dest, const char *src, size_t size) {
    if (size == 0) return NULL;

    strncpy(dest, src, size - 1);
    dest[size - 1] = '\0';

    return dest;
}

char *trim(const char *str) {
    if (str == NULL) {
        return NULL;
    }

    size_t len = strlen(str);
    size_t start = 0;
    size_t end = len - 1;

    while (isspace((unsigned char)str[start])) {
        start++;
    }

    while (end > start && isspace((unsigned char)str[end])) {
        end--;
    }

    size_t trimmed_len = end - start + 1;
    char *trimmed_str = malloc(trimmed_len + 1);

    if (trimmed_str == NULL) {
        return NULL;
    }

    strncpy(trimmed_str, str + start, trimmed_len);
    trimmed_str[trimmed_len] = '\0';

    return trimmed_str;
}

void write_command_to_file(char *filename, char *command) {
    char output[OUTPUT_SIZE];

    FILE *file = fopen(filename, "w");
    if (file == NULL) die("opening file");

    // running the command
    FILE *pipe = popen(command, "r");
    if (!pipe) die("running command");

    // reading the output
    fread(output, 1, OUTPUT_SIZE, pipe);
    pclose(pipe);

    fprintf(file, "%s", output);
    fclose(file);
}

void expand_path(char *path) {
    char expanded_path[PATH_MAX];
    char resolved_path[PATH_MAX];
    struct stat sb;

    // handle ~
    if (path[0] == '~') {
        const char *home_dir = getenv("HOME");
        if (!home_dir) {
            die("$HOME environment variable not set");
        }
        strcpy(path, expanded_path);
        if (stat(path, &sb) != 0) {
            perror("stat");
            die("Failed to stat expanded path");
        }
        if (!S_ISDIR(sb.st_mode)) {
            fprintf(stderr, "Error: '%s' is not a directory\n", path);
            exit(1);
        }
        return;
    }

    // handle .
    if (strcmp(path, ".") == 0) {
        if (getcwd(path, PATH_MAX) == NULL) {
            perror("getcwd() error");
            die("Failed to get current working directory");
        }
        return;
    }

    // handle "./" (Relative to Current)
    if (strncmp(path, "./", 2) == 0) {
        if (getcwd(expanded_path, sizeof(expanded_path)) == NULL) {
            perror("getcwd() error");
            die("Failed to get current working directory");
        }
        if (stat(path, &sb) != 0) {
            perror("stat");
            die("Failed to stat resolved path");
        }
        if (!S_ISDIR(sb.st_mode)) {
            fprintf(stderr, "Error: '%s' is not a directory\n", path);
            exit(1);
        }
        return; // Path is now absolute and a directory
    }

    // handle absolute or relative paths with ".." or "/"
    if (path[0] == '/' || strchr(path, '/') != NULL) {
        if (realpath(path, resolved_path) == NULL) {
            perror("realpath() error");
            die("Failed to resolve path");
        }
        strcpy(path, resolved_path);
        if (stat(path, &sb) != 0) {
            perror("stat");
            die("Failed to stat resolved path");
        }
        if (!S_ISDIR(sb.st_mode)) {
            fprintf(stderr, "Error: '%s' is not a directory\n", path);
            exit(1);
        }
        return;
    }

    // handle simple relative paths (like a dir in $PWD)
    if (getcwd(expanded_path, sizeof(expanded_path)) == NULL) {
        perror("getcwd() error");
        die("Failed to get current working directory");
    }

    if (stat(path, &sb) != 0) {
        perror("stat");
        die("Failed to stat final path");
    }
    if (!S_ISDIR(sb.st_mode)) {
        fprintf(stderr, "Error: '%s' is not a directory\n", path);
        exit(1);
    }
}
