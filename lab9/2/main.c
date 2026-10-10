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

Status fill_array(int *array, int size) {
    if (array == NULL || size < 10 || size > 10000) {
        return INVALID;
    }

    for (int i = 0; i < size; i++) {
        array[i] = rand() % 2001 - 1000;
    }

    return OK;
}

Status build_array(const int *a, int size_a, const int *b, int size_b, int *c) {
    if (a == NULL || b == NULL || c == NULL || size_a < 1 || size_b < 1) {
        return INVALID;
    }

    for (int i = 0; i < size_a; i++) {
        int nearest = 0;

        for (int j = 1; j < size_b; j++) {
            long long distance = (long long)a[i] - b[j];
            long long best = (long long)a[i] - b[nearest];
            if (distance < 0) {
                distance = -distance;
            }

            if (best < 0) {
                best = -best;
            }

            if (distance < best) {
                nearest = j;
            }
        }

        long long sum = (long long)a[i] + b[nearest];
        if (sum < INT_MIN || sum > INT_MAX) {
            return OVERFLOW;
        }

        c[i] = (int)sum;
    }

    return OK;
}

Status print_array(const int *array, int size) {
    if (array == NULL || size < 1) {
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
    (void)argv;
    if (argc != 1) {
        return report(INVALID);
    }

    srand((unsigned)time(NULL));

    int size_a = rand() % 9991 + 10;
    int size_b = rand() % 9991 + 10;
    int *a = malloc(size_a * sizeof(int));
    int *b = malloc(size_b * sizeof(int));
    int *c = malloc(size_a * sizeof(int));

    if (a == NULL || b == NULL || c == NULL) {
        free(a);
        free(b);
        free(c);

        return report(MEMORY_ERROR);
    }

    Status status = fill_array(a, size_a);

    if (status == OK) {
        status = fill_array(b, size_b);
    }

    if (status == OK) {
        status = build_array(a, size_a, b, size_b, c);
    }

    if (status == OK) {
        printf("A (%d): ", size_a);
        status = print_array(a, size_a);
    }

    if (status == OK) {
        printf("B (%d): ", size_b);
        status = print_array(b, size_b);
    }

    if (status == OK) {
        printf("C (%d): ", size_a);
        status = print_array(c, size_a);
    }

    free(a);
    free(b);
    free(c);
    if (status == OK && fflush(stdout) == EOF) {
        status = FILE_ERROR;
    }

    return report(status);
}
