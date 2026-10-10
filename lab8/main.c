#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
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

Status digit_value(unsigned char c, int *digit) {
    if (!digit) {
        return INVALID;
    }

    if (c >= '0' && c <= '9') {
        *digit = c - '0';
    } else if (c >= 'A' && c <= 'Z') {
        *digit = c - 'A' + 10;
    } else if (c >= 'a' && c <= 'z') {
        *digit = c - 'a' + 10;
    } else {
        return INVALID;
    }

    return OK;
}

Status parse_base(const char *text, int base, int uppercase, long long *result) {
    if (text == NULL || result == NULL || text[0] == '\0' || base < 2 || base > 36) {
        return INVALID;
    }

    int negative = text[0] == '-';
    int i = 0;

    if (text[0] == '-' || text[0] == '+') {
        i++;
    }

    if (text[i] == '\0') {
        return INVALID;
    }

    unsigned long long value = 0;
    unsigned long long limit = LLONG_MAX;
    if (negative) {
        limit++;
    }

    while (text[i] != '\0') {
        if (uppercase && text[i] >= 'a' && text[i] <= 'z') {
            return INVALID;
        }

        int digit;
        Status status = digit_value((unsigned char)text[i], &digit);

        if (status != OK || digit >= base) {
            return INVALID;
        }

        if (value > (limit - digit) / base) {
            return OVERFLOW;
        }

        value = value * base + digit;
        i++;
    }

    if (!negative) {
        *result = (long long)value;
    } else if (value == (unsigned long long)LLONG_MAX + 1) {
        *result = LLONG_MIN;
    } else {
        *result = -(long long)value;
    }

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

Status minimal_base(const char *text, int *base, long long *value, char **normalized) {
    if (!text || !base || !value || !normalized || !*text) {
        return INVALID;
    }

    *normalized = NULL;

    const char *p = text;
    int negative = *p == '-';

    if (*p == '+' || *p == '-') {
        ++p;
    }

    if (!*p) {
        return INVALID;
    }

    int maximum = 1;

    for (const char *q = p; *q; ++q) {
        int digit;

        if (digit_value((unsigned char)*q, &digit) != OK) {
            return INVALID;
        }

        if (digit > maximum) {
            maximum = digit;
        }
    }

    *base = maximum + 1;

    Status status = parse_base(text, *base, 0, value);

    if (status != OK) {
        return status;
    }

    while (*p == '0' && p[1]) {
        ++p;
    }

    size_t length = strlen(p), sign = negative && *value != 0;

    if (length > SIZE_MAX - sign - 1) {
        return OVERFLOW;
    }

    char *result = malloc(length + sign + 1);

    if (!result) {
        return MEMORY_ERROR;
    }

    if (sign) {
        result[0] = '-';
    }

    for (size_t i = 0; i < length; i++) {
        result[sign + i] = p[i] >= 'a' && p[i] <= 'z' ? (char)(p[i] - 'a' + 'A') : p[i];
    }

    result[sign + length] = '\0';
    *normalized = result;

    return OK;
}

Status convert_file(FILE *input, FILE *output) {
    if (!input || !output) {
        return INVALID;
    }

    for (;;) {
        char *token, *normalized = NULL;
        Status status = read_token(input, &token);

        if (status == END_OF_INPUT) {
            return OK;
        }

        if (status != OK) {
            return status;
        }

        int base;
        long long value;
        status = minimal_base(token, &base, &value, &normalized);
        free(token);
        if (status == OK && fprintf(output, "%s %d %lld\n", normalized, base, value) < 0) {
            status = FILE_ERROR;
        }

        free(normalized);
        if (status != OK) {
            return status;
        }
    }
}

int main(int argc, char **argv) {
    if (argc != 3) {
        return report(INVALID);
    }

    Status status = distinct_files(argv[1], argv[2]);
    FILE *input = NULL, *output = NULL;

    if (status == OK) {
        input = fopen(argv[1], "rb");
        output = fopen(argv[2], "wb");
        if (!input || !output) {
            status = FILE_ERROR;
        }
    }

    if (status == OK) {
        status = convert_file(input, output);
    }

    if (input && fclose(input) != 0) {
        status = FILE_ERROR;
    }

    if (output && fclose(output) != 0) {
        status = FILE_ERROR;
    }

    return report(status);
}
