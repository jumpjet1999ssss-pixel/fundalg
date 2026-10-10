#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
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

Status parse_flag(const char *text, const char *allowed, char *flag) {
    if (!text || !allowed || !flag || strlen(text) != 2 || (text[0] != '-' && text[0] != '/') ||
        !strchr(allowed, text[1])) {
        return INVALID;
    }

    *flag = text[1];

    return OK;
}

Status magnitude(long long value, unsigned long long *result) {
    if (!result) {
        return INVALID;
    }

    *result = value < 0 ? (unsigned long long)(-(value + 1)) + 1 : (unsigned long long)value;

    return OK;
}

Status format_base(long long value, int base, char *buffer, size_t size) {
    if (buffer == NULL || base < 2 || base > 36) {
        return INVALID;
    }

    char reversed[70];
    int count = 0;
    unsigned long long number;
    magnitude(value, &number);

    do {
        int digit = number % base;

        if (digit < 10) {
            reversed[count] = '0' + digit;
        } else {
            reversed[count] = 'A' + digit - 10;
        }

        count++;
        number /= base;
    } while (number != 0);
    if (value < 0) {
        reversed[count++] = '-';
    }

    if (size <= (size_t)count) {
        return INVALID;
    }

    for (int i = 0; i < count; i++) {
        buffer[i] = reversed[count - i - 1];
    }

    buffer[count] = '\0';

    return OK;
}

Status read_token(FILE *stream, char **result) {
    if (!stream || !result) {
        return INVALID;
    }

    *result = NULL;

    int c;

    do {
        c = fgetc(stream);
    } while (c != EOF && isspace((unsigned char)c));
    if (c == EOF) {
        return ferror(stream) ? FILE_ERROR : END_OF_INPUT;
    }

    size_t capacity = 32, length = 0;
    char *text = malloc(capacity);

    if (!text) {
        return MEMORY_ERROR;
    }

    do {
        if (length + 1 == capacity) {
            if (capacity > SIZE_MAX / 2) {
                free(text);

                return OVERFLOW;
            }

            char *next = realloc(text, capacity * 2);

            if (!next) {
                free(text);

                return MEMORY_ERROR;
            }

            text = next;
            capacity *= 2;
        }

        text[length++] = (char)c;
        c = fgetc(stream);
    } while (c != EOF && !isspace((unsigned char)c));
    if (ferror(stream)) {
        free(text);

        return FILE_ERROR;
    }

    text[length] = '\0';
    *result = text;

    return OK;
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

Status interleave(FILE *first, FILE *second, FILE *output) {
    if (!first || !second || !output) {
        return INVALID;
    }

    int done[2] = {0, 0}, written = 0;
    FILE *inputs[2] = {first, second};

    while (!done[0] || !done[1]) {
        for (int i = 0; i < 2; i++) {
            if (done[i]) {
                continue;
            }

            char *token;
            Status status = read_token(inputs[i], &token);

            if (status == END_OF_INPUT) {
                done[i] = 1;
                continue;
            }

            if (status != OK) {
                return status;
            }

            if ((written && fputc(' ', output) == EOF) || fputs(token, output) == EOF) {
                status = FILE_ERROR;
            }

            free(token);
            if (status != OK) {
                return status;
            }

            written = 1;
        }
    }

    return OK;
}

Status transform_token(const char *token, size_t position, char **result) {
    if (!token || !result || !position) {
        return INVALID;
    }

    int base = position % 10 == 0 ? 4 : position % 5 == 0 ? 8 : 0;
    int lower = position % 2 == 0;
    size_t length = strlen(token);

    if (length > (SIZE_MAX - 1) / 4) {
        return OVERFLOW;
    }

    char *text = malloc(length * 4 + 1);

    if (!text) {
        return MEMORY_ERROR;
    }

    size_t used = 0;

    for (size_t i = 0; i < length; i++) {
        unsigned char c = (unsigned char)token[i];

        if (lower && c >= 'A' && c <= 'Z') {
            c = (unsigned char)(c - 'A' + 'a');
        }

        if (base) {
            char buffer[16];
            Status status = format_base(c, base, buffer, sizeof(buffer));

            if (status != OK) {
                free(text);

                return status;
            }

            size_t digits = strlen(buffer);

            memcpy(text + used, buffer, digits);
            used += digits;
        } else {
            text[used++] = (char)c;
        }
    }

    text[used] = '\0';
    *result = text;

    return OK;
}

Status transform_file(FILE *input, FILE *output) {
    if (!input || !output) {
        return INVALID;
    }

    size_t position = 1;

    for (;;) {
        char *token, *transformed = NULL;
        Status status = read_token(input, &token);

        if (status == END_OF_INPUT) {
            return OK;
        }

        if (status != OK) {
            return status;
        }

        status = transform_token(token, position, &transformed);
        free(token);
        if (status == OK &&
            ((position > 1 && fputc(' ', output) == EOF) || fputs(transformed, output) == EOF)) {
            status = FILE_ERROR;
        }

        free(transformed);
        if (status != OK) {
            return status;
        }

        if (position == SIZE_MAX) {
            return OVERFLOW;
        }

        ++position;
    }
}

int main(int argc, char **argv) {
    if (argc < 2) {
        return report(INVALID);
    }

    char flag;
    Status status = parse_flag(argv[1], "ra", &flag);

    if (status != OK) {
        return report(status);
    }

    if (argc != (flag == 'r' ? 5 : 4)) {
        return report(INVALID);
    }

    const char *path = argv[argc - 1];

    status = distinct_files(argv[2], path);
    if (status == OK && flag == 'r') {
        status = distinct_files(argv[3], path);
    }

    FILE *first = NULL, *second = NULL, *output = NULL;

    if (status == OK) {
        first = fopen(argv[2], "rb");
        if (flag == 'r') {
            second = fopen(argv[3], "rb");
        }

        output = fopen(path, "wb");
        if (!first || !output || (flag == 'r' && !second)) {
            status = FILE_ERROR;
        }
    }

    if (status == OK) {
        status = flag == 'r' ? interleave(first, second, output) : transform_file(first, output);
    }

    if (first && fclose(first) != 0) {
        status = FILE_ERROR;
    }

    if (second && fclose(second) != 0) {
        status = FILE_ERROR;
    }

    if (output && fclose(output) != 0) {
        status = FILE_ERROR;
    }

    return report(status);
}
