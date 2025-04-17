#include <stdio.h>

#include "db.h"

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <database_file>\n", argv[0]);
        return 1;
    }

    const char *db_filename = argv[1];
    struct Connection *conn = db_open(db_filename);
    if (!conn) {
        fprintf(stderr, "Error: Could not open or create database file '%s'\n", db_filename);
        return 1;
    }

    printf("Contents of the database '%s':\n", db_filename);

    if (conn->db->total == 0) {
        printf("Database is empty.\n");
    } else {
        for (int i = 1; i <= conn->db->total; i++) {
            printf("\n--- Entry %d ---\n", i);
            print_address(&conn->db->rows[i]);
        }
    }

    db_close(conn);
    return 0;
}
