#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct pair {
    char *key;
    char *value;
    struct pair *next;
} pair_t;

pair_t *head = NULL;

int run(char command[], char output[], int output_size);
char *getLine(char *str, int index);
char *trim(char *str);
void addPair(char *key, char *value);
void freePair();

int main() {
    // make tmp
    char t_filename[] = "/tmp/rn-XXXXXXXX";
    int fd = mkstemp(t_filename);
    if (fd == -1) {
        perror("mkstemp");
        return 1;
    }
    close(fd);

    // open
    FILE *t_file = fopen(t_filename, "w");
    if (t_file == NULL) {
        printf("Could not open file\n");
        return 1;
    }

    // get output for ls
    char ls[] = "ls -Av --group-directories-first";
    int ls_s = 10240;
    char ls_out[ls_s];
    run(ls, ls_out, ls_s);

    // write
    fprintf(t_file, "%s", ls_out);
    fclose(t_file);

    // open in editor
    char *editor = getenv("EDITOR");
    if (editor == NULL) {
        printf("$EDITOR not set.\n");
        return 1;
    }
    char editor_command[256];
    sprintf(editor_command, "%s %s", editor, t_filename);
    system(editor_command);

    // re-open tmp file
    FILE *t_file_after = fopen(t_filename, "r");
    if (t_file_after == NULL) {
        printf("Error: could not open file\n");
        return 1;
    }

    // read
    char t_buffer_after[256];
    int index = 0;
    while (fgets(t_buffer_after, sizeof(t_buffer_after), t_file_after) !=
           NULL) {
        char *t_buffer_before = getLine(ls_out, index);
        char *before = trim(t_buffer_before);
        char *after = trim(t_buffer_after);
        if (after[0] == '\0') {
            printf("Error: cannot be renamed to '', operation failed.\n");
            return 1;
        }
        if (strcmp(before, after) != 0) {
            addPair(before, after);
        }
        index++;
    }

    if (head != NULL) {
        pair_t *current = head;
        while (current != NULL) {
            printf("%s -> %s\n", current->key, current->value);
            // rename
            // if fail exit
            current = current->next;
        }
        return 0;
    } else {
        printf("Nothing to do\n");
    }

    freePair();
    remove(t_filename);
    return 0;
}

int run(char command[], char output[], int output_size) {
    FILE *pipe;
    pipe = popen(command, "r");

    if (!pipe) {
        perror("popen");
        return 1;
    }
    // read the output
    fread(output, 1, output_size, pipe);
    pclose(pipe);

    return 0;
}

char *getLine(char *str, int index) {
    int lineCount = 0;
    int startIdx = 0;

    // find the start index of the desired line
    for (int i = 0; str[i] != '\0'; i++) {
        if (str[i] == '\n') {
            lineCount++;
            if (lineCount == index) {
                startIdx = i + 1;
                break;
            }
        }
    }

    if (lineCount < index) {
        return NULL;
    }

    // find the end index of the desired line
    int endIdx = startIdx;
    while (str[endIdx] != '\0' && str[endIdx] != '\n') {
        endIdx++;
    }

    // allocate memory for the desired line
    char *line = (char *)malloc((endIdx - startIdx + 1) * sizeof(char));

    // copy the desired line into the allocated memory
    strncpy(line, str + startIdx, endIdx - startIdx);
    line[endIdx - startIdx] = '\0';

    return line;
}

char *trim(char *str) {
    int len = strlen(str);
    int start = 0;
    int end = len - 1;

    // find the first non-whitespace character
    while (start <= end && isspace(str[start])) {
        start++;
    }
    // find the last non-whitespace character
    while (start <= end && isspace(str[end])) {
        end--;
    }
    // shift the string to remove whitespace
    memmove(str, str + start, end - start + 1);

    str[end - start + 1] = '\0';
    return str;
}

void addPair(char *key, char *value) {
    pair_t *newPair = (pair_t *)malloc(sizeof(pair_t));
    if (newPair == NULL) {
        fprintf(stderr, "Memory allocation failed (pair)\n");
        return;
    }

    // allocate memory for the key
    newPair->key = strdup(key);
    if (newPair->key == NULL) {
        fprintf(stderr, "Memory allocation failed (key)\n");
        free(newPair);
        return;
    }
    // allocate memory for the value
    newPair->value = strdup(value);
    if (newPair->value == NULL) {
        fprintf(stderr, "Memory allocation failed (value)\n");
        free(newPair->key);
        free(newPair);
        return;
    }

    newPair->next = head;
    head = newPair;
}

void freePair() {
    pair_t *current = head;
    while (current != NULL) {
        pair_t *temp = current;
        current = current->next;
        free(temp->key);
        free(temp->value);
        free(temp);
    }
    head = NULL;
}
