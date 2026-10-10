#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/stat.h>

typedef enum {
    OK,
    INVALID,
    OVERFLOW,
    MEMORY_ERROR,
    FILE_ERROR,
    NO_CONVERGENCE,
    END_OF_INPUT
} Status;

int report(Status status) {
    const char *message = "Unknown error";

    switch (status) {
        case OK:
            return 0;
        case INVALID:
            message = "Invalid arguments or input";
            break;
        case OVERFLOW:
            message = "Numeric overflow";
            break;
        case MEMORY_ERROR:
            message = "Memory allocation failed";
            break;
        case FILE_ERROR:
            message = "File or stream error";
            break;
        case NO_CONVERGENCE:
            message = "Requested accuracy cannot be reached";
            break;
        case END_OF_INPUT:
            message = "Unexpected end of input";
            break;
    }

    fprintf(stderr, "%s\n", message);

    return 1;
}

Status distinct_files(const char *input, const char *output) {
    if (!input || !output || !*input || !*output) {
        return INVALID;
    }

    if (!strcmp(input, output)) {
        return INVALID;
    }

    struct stat a, b;

    if (stat(input, &a) != 0) {
        return FILE_ERROR;
    }

    if (stat(output, &b) == 0 && a.st_dev == b.st_dev && a.st_ino == b.st_ino) {
        return INVALID;
    }

    return OK;
}

Status transform(FILE *input, FILE *output, char flag) {
    if (!input || !output || (flag != 'd' && flag != 'i' && flag != 's' && flag != 'a')) {
        return INVALID;
    }

    size_t count = 0;
    int c, has_line = 0;

    while ((c = fgetc(input)) != EOF) {
        int letter = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
        int digit = c >= '0' && c <= '9';

        if (flag == 'd') {
            if (!digit && fputc(c, output) == EOF) {
                return FILE_ERROR;
            }
        } else if (flag == 'a') {
            if (digit) {
                if (fputc(c, output) == EOF) {
                    return FILE_ERROR;
                }
            } else if (fprintf(output, "%X", (unsigned)c) < 0) {
                return FILE_ERROR;
            }
        } else if (c == '\n') {
            if (fprintf(output, "%zu\n", count) < 0) {
                return FILE_ERROR;
            }

            count = 0;
            has_line = 0;
        } else {
            has_line = 1;
            if ((flag == 'i' && letter) || (flag == 's' && !letter && !digit && c != ' ')) {
                if (count == SIZE_MAX) {
                    return OVERFLOW;
                }

                ++count;
            }
        }
    }

    if (ferror(input)) {
        return FILE_ERROR;
    }

    if ((flag == 'i' || flag == 's') && has_line && fprintf(output, "%zu\n", count) < 0) {
        return FILE_ERROR;
    }

    return OK;
}

Status output_name(const char *input, char **result) {
    if (!input || !result || !*input) {
        return INVALID;
    }

    const char *name = strrchr(input, '/');

    name = name ? name + 1 : input;
    if (!*name) {
        return INVALID;
    }

    size_t prefix = (size_t)(name - input), length = strlen(input);

    if (length > SIZE_MAX - 5) {
        return OVERFLOW;
    }

    char *path = malloc(length + 5);

    if (!path) {
        return MEMORY_ERROR;
    }

    memcpy(path, input, prefix);
    memcpy(path + prefix, "out_", 4);
    strcpy(path + prefix + 4, name);
    *result = path;

    return OK;
}

int main(int argc, char **argv) {
    if (argc < 3) {
        return report(INVALID);
    }

    size_t length = strlen(argv[1]);

    if ((argv[1][0] != '-' && argv[1][0] != '/') || (length != 2 && length != 3)) {
        return report(INVALID);
    }

    int named = length == 3;

    if ((named && argv[1][1] != 'n') || argc != (named ? 4 : 3)) {
        return report(INVALID);
    }

    char flag = argv[1][named ? 2 : 1];

    if ((flag != 'd' && flag != 'i' && flag != 's' && flag != 'a')) {
        return report(INVALID);
    }

    char *generated = NULL;
    Status status = OK;

    if (!named) {
        status = output_name(argv[2], &generated);
    }

    const char *path = named ? argv[3] : generated;

    if (status == OK) {
        status = distinct_files(argv[2], path);
    }

    FILE *input = NULL, *output = NULL;

    if (status == OK) {
        input = fopen(argv[2], "rb");
        output = fopen(path, "wb");
        if (!input || !output) {
            status = FILE_ERROR;
        }
    }

    if (status == OK) {
        status = transform(input, output, flag);
    }

    if (input && fclose(input) != 0) {
        status = FILE_ERROR;
    }

    if (output && fclose(output) != 0) {
        status = FILE_ERROR;
    }

    free(generated);

    return report(status);
}
