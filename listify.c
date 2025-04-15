#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "listify.h"

struct string_list *create_string_list() {
    struct string_list *list = (struct string_list *)malloc(sizeof(struct string_list));

    if (list == NULL) {
        perror("Failed to allocate memory for struct string_list");
        return NULL;
    }
    list->capacity = 10;
    list->size = 0;
    list->data = (char **)malloc(sizeof(char *) * list->capacity);

    if (list->data == NULL) {
        perror("Failed to allocate memory for struct string_list data");
        free(list);
        return NULL;
    }
    return list;
}

int add_string(struct string_list *list, const char *str) {
    if (list == NULL || str == NULL) return -1;

    // resize if full
    if (list->size == list->capacity) {
        list->capacity *= 2;
        char **newData = (char **)realloc(list->data, sizeof(char *) * list->capacity);
        if (newData == NULL) {
            perror("Failed to reallocate memory for struct string_list");
            return -1;
        }
        list->data = newData;
    }

    // allocate memory for the new string and copy it
    list->data[list->size] = (char *)malloc(strlen(str) + 1);
    if (list->data[list->size] == NULL) {
        perror("Failed to allocate memory for string");
        return -1;
    }
    strcpy(list->data[list->size], str);
    list->size++;
    return 0;
}

struct string_list *read_file_to_list(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        perror("Failed to open file");
        return NULL;
    }

    struct string_list *list = create_string_list();
    if (list == NULL) {
        fclose(file);
        return NULL;
    }

    char line[MAX_LINE_LENGTH];
    while (fgets(line, sizeof(line), file) != NULL) {
        // remove trailing newline character
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') {
            line[len - 1] = '\0';
        }
        if (add_string(list, line) != 0) {
            // error adding string, free the list and return NULL
            free_string_list(list);
            fclose(file);
            return NULL;
        }
    }

    fclose(file);
    return list;
}

void free_string_list(struct string_list *list) {
    if (list != NULL) {
        if (list->data != NULL) {
            for (int i = 0; i < list->size; i++) {
                free(list->data[i]);
            }
            free(list->data);
        }
        free(list);
    }
}
