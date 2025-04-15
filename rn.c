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

// struct Address get_changes(struct string_list initial, struct string_list final) {
void get_changes(struct string_list initial, struct string_list final) {
    if (initial.size != final.size) die("size does't match");

    for (int i = 1; i < initial.size; i++) {
        printf("%s -> %s\n", initial.data[i], final.data[i]);
    }
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

    // open in editor
    char *editor = getenv("EDITOR");
    if (editor == NULL) die("$EDITOR not set.");

    char editor_command[256];
    sprintf(editor_command, "%s %s", editor, tmp_filename);
    system(editor_command);

    // final state
    struct string_list *final_state = read_file_to_list(tmp_filename);
    remove(tmp_filename);

    get_changes(*initial_state, *final_state);
}

int main(int argc, char *argv[]) {
    char *path = ".";
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
            for (int i = 0; argv[1][i] != '\0'; i++) {
                if (argv[1][i] == 'a') {
                    all_flag = 1;
                } else if (argv[1][i] == 'c') {
                    confirm_flag = 1;
                } else if (argv[1][i] == '-') {
                } else {
                    fprintf(stderr, "Error: Unknown option '%c'\n", argv[1][i]);
                    return 1;
                }
            }

            if (argc > 2) path = argv[2];
            expand_path(path);
        }
    }

    printf("bulk rename\n");
    printf("path: %s\n", path);
    printf("a:%i, c:%i\n", all_flag, confirm_flag);

    bulk_rename(path, all_flag);

    return 0;
}
