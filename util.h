#define OUTPUT_SIZE 10240

void die(const char *message);
char *copy_str(char *dest, const char *src, size_t size);
char *trim(const char *str);
void expand_path(char *path);
void write_command_to_file(char *filename, char *command);
