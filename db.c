#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "db.h"
#include "util.h"

void print_address(struct Address *addr) {
    printf("FILE: %s\n", addr->file_name);

    for (int i = 0; i < MAX_ROWS; i++) {
        if (addr->changed_files[i].file_before[0] == '\0') {
            return;
        }
        printf("%s -> %s\n", addr->changed_files[i].file_before, addr->changed_files[i].file_after);
    }
}

void db_load(struct Connection *conn) {
    int rc = fread(conn->db, sizeof(struct Database), 1, conn->file);
    if (rc != 1) die("Failed to load database.");
}

struct Connection *db_open(const char *filename) {
    struct Connection *conn = malloc(sizeof(struct Connection));
    if (!conn) die("Memory error");

    conn->db = malloc(sizeof(struct Database));
    if (!conn->db) die("Memory error");

    conn->file = fopen(filename, "r+"); // for read & write
    if (!conn->file) {
        conn->file = fopen(filename, "w+");
        if (!conn->file) die("Failed to open the file");
    } else {
        db_load(conn);
    }
    return conn;
}

void db_close(struct Connection *conn) {
    if (conn) {
        if (conn->file) fclose(conn->file);
        if (conn->db) free(conn->db);
        free(conn);
    }
}

void db_write(struct Connection *conn) {
    rewind(conn->file);

    int rc = fwrite(conn->db, sizeof(struct Database), 1, conn->file);
    if (rc != 1) die("Failed to write database.");

    rc = fflush(conn->file);
    if (rc == -1) die("Cannot flush database.");
}

void db_set(struct Connection *conn, int id, struct Address *changed_files) {
    // set to -1 to add to the end
    if (id == -1) {
        conn->db->total += 1;
        id = conn->db->total;
    }
    struct Address *addr = &conn->db->rows[id];
    *addr = *changed_files;
}

void db_get(struct Connection *conn, int id) {
    struct Address *addr = &conn->db->rows[id];
    print_address(addr);
}

int db_search(struct Connection *conn, const char *search_string) {
    for (int i = 1; i <= conn->db->total; i++) {
        if (strcmp(search_string, conn->db->rows[i].file_name) == 0) {
            return i;
        }
    }
    return -1;
}

int db_delete_item(struct Connection *conn, int id) {
    if (id < 1 || id > conn->db->total) {
        fprintf(stderr, "Error: Invalid ID (%d). ID must be between 1 and %d.\n", id, conn->db->total);
        return -1;
    }

    // shift subsequent elements to overwrite the deleted one
    for (int i = id; i < conn->db->total; i++) {
        conn->db->rows[i] = conn->db->rows[i + 1];
    }

    // decrement the total count
    conn->db->total--;

    // mark the last element as empty
    memset(&conn->db->rows[conn->db->total + 1], 0, sizeof(struct Address));

    db_write(conn);
    return 0;
}
