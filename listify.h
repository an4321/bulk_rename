#define MAX_LINE_LENGTH 256

struct string_list {
    char **data;
    int size;
    int capacity;
};

struct string_list *create_string_list();
struct string_list *read_file_to_list(const char *filename);
int add_string(struct string_list *list, const char *str);
void free_string_list(struct string_list *list);

