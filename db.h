#define MAX_DATA 256
#define MAX_ROWS 128

struct Changes {
    char file_before[MAX_DATA];
    char file_after[MAX_DATA];
};

struct Address {
    int total_changes;
    char file_name[MAX_DATA];
    struct Changes changed_files[MAX_ROWS];
};

struct Database {
    int total;
    struct Address rows[MAX_ROWS];
};

struct Connection {
    FILE *file;
    struct Database *db;
};

void print_address(struct Address *addr);
struct Connection *db_open(const char *filename);
void db_load(struct Connection *conn);
void db_write(struct Connection *conn);
void db_get(struct Connection *conn, int id);
void db_set(struct Connection *conn, int id, struct Address *changed_files);
void db_close(struct Connection *conn);
int db_search(struct Connection *conn, const char *search_string);
