#include <ctype.h>
#include <errno.h>
#include <limits.h>
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

void get_dir_path(char dir[PATH_MAX]) {
    char resolved_path[PATH_MAX];
    const char *home_dir;

    if (dir[0] == '~') {
        home_dir = getenv("HOME");
        if (home_dir == NULL) die("$HOME environment variable not set.");

        copy_str(resolved_path, home_dir, PATH_MAX);
    }

    if (realpath(dir, resolved_path) != NULL) {
        strncpy(dir, resolved_path, PATH_MAX - 1);
        dir[PATH_MAX - 1] = '\0'; // ensure null termination
    } else {
        die("realpath");
    }

    struct stat path_stat;

    if (stat(dir, &path_stat) != 0) {
        fprintf(stderr, "Failed to stat path: %s: %s\n", dir, strerror(errno));
        exit(1);
    }
    if (!S_ISDIR(path_stat.st_mode)) {
        fprintf(stderr, "%s is not a directory\n", dir);
        exit(1);
    }
}

int safe_rename(const char *oldName, const char *newName) {
    struct stat buffer;

    // check if the new name already exists
    if (stat(newName, &buffer) == 0) {
        fprintf(stderr, "Error: '%s' already exists. Renaming prevented to avoid overwrite.\n", newName);
        return 1;
    } else {
        // new name does not exist, proceed with renaming
        if (rename(oldName, newName) != 0) {
            perror("Error renaming file");
            return 2;
        } else {
            return 0;
        }
    }
}
