#include <linux/limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "db.h"
#include "listify.h"
#include "util.h"

#define LS_COMMAND "ls -vp --group-directories-first"

void help(char *this) {
    printf("Usage: %s [-h] [-r <path>] [-a] [-c] [<path>]\n", this);
    printf("\t-h, --help\tShow this help message\n");
    printf("\t-r, --revert <path>\tRevert the last rename operation in <path>\n");
    printf("\t-a\t\tInclude hidden files in the listing\n");
    printf("\t-c\t\tConfirm each rename operation (not yet implemented)\n");
    printf("\t<path>\t\tThe directory to perform the bulk rename on (default: .)\n");
}

void revert(char *file) {
    printf("Reverting changes in: %s\n", file);
    printf("This feature is not yet implemented.\n");
}

// compare string lists
struct Address *get_changes(const char *file_name, struct string_list initial,
                            struct string_list final) {
    if (initial.size != final.size) {
        fprintf(stderr, "Error: Size of initial and final lists do not match.\n");
        return NULL;
    }

    struct Address *addr = (struct Address *)malloc(sizeof(struct Address));
    if (addr == NULL) {
        perror("Failed to allocate memory for Address");
        return NULL;
    }

    strncpy(addr->file_name, file_name, MAX_DATA - 1);
    addr->file_name[MAX_DATA - 1] = '\0'; // Ensure null termination
    addr->total_changes = 0;

    for (int i = 0; i < initial.size; i++) {
        if (strcmp(initial.data[i], final.data[i]) != 0) {
            if (addr->total_changes < MAX_ROWS) {
                strncpy(addr->changed_files[addr->total_changes].file_before, initial.data[i],
                        MAX_DATA - 1);
                addr->changed_files[addr->total_changes].file_before[MAX_DATA - 1] = '\0';

                strncpy(addr->changed_files[addr->total_changes].file_after, final.data[i],
                        MAX_DATA - 1);
                addr->changed_files[addr->total_changes].file_after[MAX_DATA - 1] = '\0';

                addr->total_changes++;
            } else {
                fprintf(stderr, "Error: Too many changes detected.\n");
                free(addr);
                return NULL;
            }
        }
    }
    return addr;
}

void bulk_rename(char *path, int all_flag) {
    // create a temp file
    char tmp_filename[] = "/tmp/rn-XXXXXXXX";
    int fd = mkstemp(tmp_filename);
    if (fd == -1) {
        perror("Could not create temporary file");
        return;
    }
    close(fd);

    // construct the 'ls' command
    char command[256];
    if (all_flag) {
        snprintf(command, sizeof(command), "%s -A \"%s\"", LS_COMMAND, path);
    } else {
        snprintf(command, sizeof(command), "%s \"%s\"", LS_COMMAND, path);
    }
    write_command_to_file(tmp_filename, command);

    // read initial state of the dir
    struct string_list *initial_state = read_file_to_list(tmp_filename);
    if (!initial_state) {
        remove(tmp_filename);
        fprintf(stderr, "Error: Failed to read initial state from temporary file.\n");
        return;
    }

    // get the user's preferred editor
    char *editor = getenv("EDITOR");
    if (editor == NULL) {
        remove(tmp_filename);
        free_string_list(initial_state);
        fprintf(stderr, "Error: $EDITOR environment variable not set.\n");
        return;
    }

    // open the tem file in the editor
    char editor_command[256];
    snprintf(editor_command, sizeof(editor_command), "%s \"%s\"", editor, tmp_filename);
    if (system(editor_command) != 0) {
        remove(tmp_filename);
        free_string_list(initial_state);
        fprintf(stderr, "Error: Failed to open editor.\n");
        return;
    }

    // read the final state of the directory after editing
    struct string_list *final_state = read_file_to_list(tmp_filename);
    remove(tmp_filename);
    if (!final_state) {
        free_string_list(initial_state);
        fprintf(stderr, "Error: Failed to read final state from temporary file.\n");
        return;
    }

    // identify the changes made by the user
    struct Address *addr = get_changes(path, *initial_state, *final_state);
    if (!addr) {
        free_string_list(initial_state);
        free_string_list(final_state);
        fprintf(stderr, "Error: Failed to get changes.\n");
        return;
    }

    // initialize db_file_path
    char db_file_path[PATH_MAX];
    const char *home_dir = getenv("HOME");
    if (home_dir == NULL) {
        die("$HOME environment variable not set");
    }
    snprintf(db_file_path, PATH_MAX, "%s/.local/state/rn-hist.db", home_dir);
    struct Connection *conn = db_open(db_file_path);

    // store the changes in the database
    if (conn) {
        int id = db_search(conn, path);
        db_set(conn, id, addr);
        db_write(conn);
        db_close(conn);
    } else {
        fprintf(stderr, "WARNING: Could not open database to store changes.\n");
    }

    // perform the rename operations
    for (int i = 0; i < addr->total_changes; i++) {
        char oldName[PATH_MAX];
        char newName[PATH_MAX];

        snprintf(oldName, PATH_MAX, "%s/%s", path, addr->changed_files[i].file_before);
        snprintf(newName, PATH_MAX, "%s/%s", path, addr->changed_files[i].file_after);

        if (rename(oldName, newName) == 0) {
            printf("Renamed: %s -> %s\n", oldName, newName);
        } else {
            perror("Error renaming file");
        }
    }

    // free dynamically allocated memory
    free_string_list(initial_state);
    free_string_list(final_state);
    free(addr);
}

int main(int argc, char *argv[]) {
    char path[PATH_MAX] = ".";
    int all_flag = 0;
    int confirm_flag = 0;

    if (argc > 1) {
        if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
            help(argv[0]);
            return 0;
        } else if (strcmp(argv[1], "-r") == 0 || strcmp(argv[1], "--revert") == 0) {
            if (argc == 3) copy_str(path, argv[2], PATH_MAX);
            get_dir_path(path);
            revert(path);
            return 0;
        } else {
            if (strcmp(argv[1], "-a") == 0 || strcmp(argv[1], "-ac") == 0 ||
                strcmp(argv[1], "-ca") == 0) {
                all_flag = 1;
            }
            if (strcmp(argv[1], "-c") == 0 || strcmp(argv[1], "-ac") == 0 ||
                strcmp(argv[1], "-ca") == 0) {
                confirm_flag = 1;
            }
        }
    }

    if (argc == 2) copy_str(path, argv[1], PATH_MAX);
    if (argc == 3) copy_str(path, argv[2], PATH_MAX);
    get_dir_path(path);

    printf("bulk rename\n");
    printf("path: %s\n", path);
    printf("a:%i, c:%i\n", all_flag, confirm_flag);

    bulk_rename(path, all_flag);

    return 0;
}
