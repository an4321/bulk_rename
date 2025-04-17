#include <limits.h>

#define OUTPUT_SIZE 10240

void die(const char *message);
char *copy_str(char *dest, const char *src, size_t size);
char *trim(const char *str);
void write_command_to_file(char *filename, char *command);
void get_dir_path(char dir[PATH_MAX]);
int safe_rename(const char *oldName, const char *newName);
