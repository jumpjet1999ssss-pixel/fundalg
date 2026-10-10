#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <time.h>

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

Status fill_array(int *array, int size, int a, int b) {
    if (array == NULL || size <= 0 || a > b) {
        return INVALID;
    }

    long long range = (long long)b - a + 1;

    for (int i = 0; i < size; i++) {
        double random = rand() / ((double)RAND_MAX + 1);

        array[i] = (int)(a + (long long)(random * range));
    }

    return OK;
}

Status swap_min_max(int *array, int size) {
    if (array == NULL || size <= 0) {
        return INVALID;
    }

    int min = 0, max = 0;

    for (int i = 1; i < size; i++) {
        if (array[i] < array[min]) {
            min = i;
        }

        if (array[i] > array[max]) {
            max = i;
        }
    }

    int temp = array[min];

    array[min] = array[max];
    array[max] = temp;

    return OK;
}

Status print_array(const int *array, int size) {
    if (array == NULL || size <= 0) {
        return INVALID;
    }

    for (int i = 0; i < size; i++) {
        if (printf("%d%c", array[i], i == size - 1 ? '\n' : ' ') < 0) {
            return FILE_ERROR;
        }
    }

    return OK;
}

int main(int argc, char **argv) {
    if (argc != 3) {
        return report(INVALID);
    }

    long long a, b;
    Status status = parse_integer(argv[1], &a);

    if (status == OK) {
        status = parse_integer(argv[2], &b);
    }

    if (status != OK) {
        return report(status);
    }

    if (a < INT_MIN || a > INT_MAX || b < INT_MIN || b > INT_MAX || a > b) {
        return report(INVALID);
    }

    srand((unsigned)time(NULL));

    int array[20];

    status = fill_array(array, 20, (int)a, (int)b);
    if (status == OK) {
        printf("Before: ");
        status = print_array(array, 20);
    }

    if (status == OK) {
        status = swap_min_max(array, 20);
    }

    if (status == OK) {
        printf("After: ");
        status = print_array(array, 20);
    }

    if (status == OK && fflush(stdout) == EOF) {
        status = FILE_ERROR;
    }

    return report(status);
}
