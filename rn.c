#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "db.h"
#include "listify.h"
#include "util.h"

#define DB_FILE "./.rn-hist"
#define LS_COMMAND "ls -vp --group-directories-first"

void help(char *this) {
    printf("Usage: %s [-h]\n", this);
    printf("\t-h  Show this help message\n");
}

void revert(char *file) {
    printf("revert\n");
    printf("path: %s\n", file);
}

struct Address *get_changes(const char *file_name, struct string_list initial, struct string_list final) {
    if (initial.size != final.size) die("size doesn't match");

    struct Address *addr = (struct Address *)malloc(sizeof(struct Address));
    if (addr == NULL) {
        perror("Failed to allocate memory for Address");
        return NULL;
    }

    strncpy(addr->file_name, file_name, MAX_DATA - 1);
    addr->file_name[MAX_DATA - 1] = '\0'; // ensure null termination
    addr->total_changes = 0;

    for (int i = 0; i < initial.size; i++) {
        if (strcmp(initial.data[i], final.data[i]) != 0) {
            if (addr->total_changes < MAX_ROWS) {
                strncpy(addr->changed_files[addr->total_changes].file_before, initial.data[i], MAX_DATA - 1);
                addr->changed_files[addr->total_changes].file_before[MAX_DATA - 1] = '\0';

                strncpy(addr->changed_files[addr->total_changes].file_after, final.data[i], MAX_DATA - 1);
                addr->changed_files[addr->total_changes].file_after[MAX_DATA - 1] = '\0';

                addr->total_changes++;
            } else {
                die("too many changes");
            }
        }
    }
    return addr;
}

void bulk_rename(char *path, int all_flag) {
    // make tmp
    char tmp_filename[] = "/tmp/rn-XXXXXXXX";
    int fd = mkstemp(tmp_filename);
    if (fd == -1) die("could not make temp file");
    close(fd);

    // write ls to tmp file
    char command[256];
    if (all_flag) {
        sprintf(command, "%s -A %s", LS_COMMAND, path);
    } else {
        sprintf(command, "%s %s", LS_COMMAND, path);
    }
    write_command_to_file(tmp_filename, command);

    // initial state
    struct string_list *initial_state = read_file_to_list(tmp_filename);
    if (!initial_state) {
        remove(tmp_filename);
        die("failed to read temp file");
    }

    // open in editor
    char *editor = getenv("EDITOR");
    if (editor == NULL) {
        remove(tmp_filename);
        die("$EDITOR not set.");
    }

    char editor_command[256];
    sprintf(editor_command, "%s %s", editor, tmp_filename);
    system(editor_command);

    // final state
    struct string_list *final_state = read_file_to_list(tmp_filename);
    remove(tmp_filename);
    if (!final_state) {
        free(initial_state->data);
        free(initial_state);
        die("Failed to read final state");
    }

    // get changes
    struct Address *addr = get_changes(path, *initial_state, *final_state);
    if (!addr) {
        free(initial_state->data);
        free(initial_state);
        free(final_state->data);
        free(final_state);
        die("Failed to get changes");
    }

    // store changes in db
    struct Connection *conn = db_open(DB_FILE);
    if (conn) {
        int id = db_search(conn, path);
        db_set(conn, id, addr);
        db_write(conn);
        db_close(conn);
    } else {
        fprintf(stderr, "WARNING: Could not open database to store changes.\n");
    }

    for (int i = 0; i < addr->total_changes; i++) {
        printf("%s -> %s\n", addr->changed_files[i].file_before, addr->changed_files[i].file_after);
    }

    // free the dynamically allocated memory
    free(initial_state->data);
    free(initial_state);
    free(final_state->data);
    free(final_state);
    free(addr);
}

int main(int argc, char *argv[]) {
    char *path = getenv("PWD");
    int all_flag = 0;
    int confirm_flag = 0;

    if (argc > 1) {
        if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
            help(argv[0]);
            return 0;
        } else if (strcmp(argv[1], "-r") == 0 || strcmp(argv[1], "--revert") == 0) {
            if (argc == 3) path = argv[2];
            expand_path(path);
            revert(path);
            return 0;
        } else {
            if (strcmp(argv[1], "-a") == 0 || strcmp(argv[1], "-ac") == 0 || strcmp(argv[1], "-ca") == 0) {
                all_flag = 1;
            }
            if (strcmp(argv[1], "-c") == 0 || strcmp(argv[1], "-ac") == 0 || strcmp(argv[1], "-ca") == 0) {
                confirm_flag = 1;
            }

            if (argc == 3) path = argv[2];
            if (argc == 2) path = argv[1];
            expand_path(path);
        }
    }

    printf("bulk rename\n");
    printf("path: %s\n", path);
    printf("a:%i, c:%i\n", all_flag, confirm_flag);

    bulk_rename(path, all_flag);

    return 0;
}
