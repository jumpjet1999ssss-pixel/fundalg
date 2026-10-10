#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdint.h>
#include <ctype.h>

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

Status parse_integer(const char *text, long long *result) {
    if (text == NULL || result == NULL || text[0] == '\0') {
        return INVALID;
    }

    int negative = 0;
    size_t i = 0;

    if (text[i] == '-' || text[i] == '+') {
        negative = text[i] == '-';
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
        if (text[i] < '0' || text[i] > '9') {
            return INVALID;
        }

        int digit = text[i] - '0';

        if (value > (limit - digit) / 10) {
            return OVERFLOW;
        }

        value = value * 10 + digit;
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

Status magnitude(long long value, unsigned long long *result) {
    if (!result) {
        return INVALID;
    }

    *result = value < 0 ? (unsigned long long)(-(value + 1)) + 1 : (unsigned long long)value;

    return OK;
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

Status add_numbers(long long a, long long b, long long *result) {
    if (result == NULL) {
        return INVALID;
    }

    if ((b > 0 && a > LLONG_MAX - b) || (b < 0 && a < LLONG_MIN - b)) {
        return OVERFLOW;
    }

    *result = a + b;

    return OK;
}

Status read_numbers(int base, long long *maximum, long long *sum) {
    if (maximum == NULL || sum == NULL || base < 2 || base > 36) {
        return INVALID;
    }

    *sum = 0;

    int count = 0;
    unsigned long long max_abs = 0;

    while (1) {
        char *text;
        Status status = read_token(stdin, &text);

        if (status != OK) {
            return status;
        }

        if (strcmp(text, "Stop") == 0) {
            free(text);

            return count == 0 ? INVALID : OK;
        }

        long long value;
        status = parse_base(text, base, 1, &value);
        free(text);
        if (status != OK) {
            return status;
        }

        status = add_numbers(*sum, value, sum);
        if (status != OK) {
            return status;
        }

        unsigned long long abs_value;
        magnitude(value, &abs_value);
        if (count == 0 || abs_value > max_abs) {
            *maximum = value;
            max_abs = abs_value;
        }

        count = 1;
    }
}

Status print_number(const char *name, long long number, int base) {
    if (name == NULL || base < 2 || base > 36) {
        return INVALID;
    }

    int bases[5] = {base, 9, 18, 27, 36};

    if (printf("%s:\n", name) < 0) {
        return FILE_ERROR;
    }

    for (int i = 0; i < 5; i++) {
        char text[70];
        Status status = format_base(number, bases[i], text, sizeof(text));

        if (status != OK) {
            return status;
        }

        if (printf("base %d: %s\n", bases[i], text) < 0) {
            return FILE_ERROR;
        }
    }

    return OK;
}

int main(int argc, char **argv) {
    (void)argv;
    if (argc != 1) {
        return report(INVALID);
    }

    char *text;
    Status status = read_token(stdin, &text);

    if (status != OK) {
        return report(status);
    }

    long long base;
    status = parse_integer(text, &base);
    free(text);
    if (status != OK) {
        return report(status);
    }

    if (base < 2 || base > 36) {
        return report(INVALID);
    }

    long long maximum, sum;
    status = read_numbers((int)base, &maximum, &sum);
    if (status == OK) {
        status = print_number("Maximum by absolute value", maximum, (int)base);
    }

    if (status == OK) {
        status = print_number("Sum", sum, (int)base);
    }

    if (status == OK && fflush(stdout) == EOF) {
        status = FILE_ERROR;
    }

    return report(status);
}
