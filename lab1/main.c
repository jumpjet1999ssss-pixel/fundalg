#include <stdio.h>
#include <limits.h>

typedef enum {
    STATUS_OK,
    STATUS_INVALID_ARGUMENT,
    STATUS_INVALID_NUMBER,
    STATUS_INVALID_FLAG,
    STATUS_OVERFLOW
} Status;

Status parse_number(const char *str, long *result) {
    if (str == NULL || result == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    if (*str == '\0') {
        return STATUS_INVALID_NUMBER;
    }

    int negative = 0;
    int i = 0;

    if (str[i] == '-') {
        negative = 1;
        i++;
    } else if (str[i] == '+') {
        i++;
    }

    if (str[i] == '\0') {
        return STATUS_INVALID_NUMBER;
    }

    unsigned long value = 0;
    unsigned long limit;

    if (negative) {
        limit = (unsigned long)LONG_MAX + 1UL;
    } else {
        limit = LONG_MAX;
    }

    while (str[i] != '\0') {
        if (str[i] < '0' || str[i] > '9') {
            return STATUS_INVALID_NUMBER;
        }

        unsigned long digit = str[i] - '0';

        if (value > (limit - digit) / 10) {
            return STATUS_OVERFLOW;
        }

        value = value * 10 + digit;
        i++;
    }

    if (negative) {
        if (value == (unsigned long)LONG_MAX + 1UL) {
            *result = LONG_MIN;
        } else {
            *result = -(long)value;
        }
    } else {
        *result = (long)value;
    }

    return STATUS_OK;
}

Status validate_flag(const char *flag) {
    if (flag == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    if (flag[0] != '-' && flag[0] != '/') {
        return STATUS_INVALID_FLAG;
    }

    if (flag[1] == '\0' || flag[2] != '\0') {
        return STATUS_INVALID_FLAG;
    }

    switch (flag[1]) {
        case 'h':
        case 'p':
        case 's':
        case 'e':
        case 'a':
        case 'f':
            return STATUS_OK;
        default:
            return STATUS_INVALID_FLAG;
    }
}

Status find_multiples(long x, int result[], int *count) {
    if (result == NULL || count == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    if (x == 0) {
        return STATUS_INVALID_NUMBER;
    }

    unsigned long step;

    if (x < 0) {
        step = (unsigned long)(-(x + 1)) + 1UL;
    } else {
        step = (unsigned long)x;
    }

    *count = 0;

    if (step > 100) {
        return STATUS_OK;
    }

    for (unsigned long i = step; i <= 100; i += step) {
        result[*count] = (int)i;
        (*count)++;
    }

    return STATUS_OK;
}

Status is_prime(long x, int *result) {
    if (result == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    if (x <= 1) {
        return STATUS_INVALID_NUMBER;
    }

    *result = 1;

    for (long i = 2; i <= x / i; i++) {
        if (x % i == 0) {
            *result = 0;
            return STATUS_OK;
        }
    }

    return STATUS_OK;
}

Status get_hex_digits(long x, int digits[], int *count) {
    if (digits == NULL || count == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    if (x < 0) {
        return STATUS_INVALID_NUMBER;
    }

    *count = 0;

    if (x == 0) {
        digits[0] = 0;
        *count = 1;
        return STATUS_OK;
    }

    while (x > 0) {
        digits[*count] = (int)(x % 16);
        (*count)++;
        x /= 16;
    }

    return STATUS_OK;
}

Status hex_digit(int value, char *result) {
    if (result == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    if (value < 0 || value > 15) {
        return STATUS_INVALID_NUMBER;
    }

    if (value < 10) {
        *result = (char)('0' + value);
    } else {
        *result = (char)('A' + value - 10);
    }

    return STATUS_OK;
}

Status calculate_power_table(long x, unsigned long long table[10][10]) {
    if (table == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    if (x < 1 || x > 10) {
        return STATUS_INVALID_NUMBER;
    }

    for (int base = 1; base <= 10; base++) {
        unsigned long long value = 1;

        for (int power = 1; power <= x; power++) {
            value *= (unsigned long long)base;
            table[base - 1][power - 1] = value;
        }
    }

    return STATUS_OK;
}

Status calculate_sum(long x, unsigned long long *result) {
    if (result == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    if (x < 1) {
        return STATUS_INVALID_NUMBER;
    }

    *result = 0;

    for (unsigned long i = 1; i <= (unsigned long)x; i++) {
        if (*result > ULLONG_MAX - i) {
            return STATUS_OVERFLOW;
        }

        *result += i;
    }

    return STATUS_OK;
}

Status calculate_factorial(long x, unsigned long long *result) {
    if (result == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    if (x < 0) {
        return STATUS_INVALID_NUMBER;
    }

    *result = 1;

    for (long i = 2; i <= x; i++) {
        if (*result > ULLONG_MAX / (unsigned long long)i) {
            return STATUS_OVERFLOW;
        }

        *result *= (unsigned long long)i;
    }

    return STATUS_OK;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <number> <flag>\n", argv[0]);
        return 1;
    }

    long x;
    Status status = parse_number(argv[1], &x);

    if (status == STATUS_INVALID_NUMBER) {
        printf("Invalid number.\n");
        return 1;
    }

    if (status == STATUS_OVERFLOW) {
        printf("Number is too large.\n");
        return 1;
    }

    if (status != STATUS_OK) {
        printf("Error.\n");
        return 1;
    }

    status = validate_flag(argv[2]);

    if (status != STATUS_OK) {
        printf("Invalid flag.\n");
        return 1;
    }

    char flag = argv[2][1];

    switch (flag) {
        case 'h': {
            int numbers[100];
            int count;

            status = find_multiples(x, numbers, &count);

            if (status != STATUS_OK) {
                printf("x cannot be zero.\n");
                break;
            }

            if (count == 0) {
                printf("There are no multiples.\n");
                break;
            }

            for (int i = 0; i < count; i++) {
                printf("%d ", numbers[i]);
            }

            printf("\n");
            break;
        }

        case 'p': {
            int prime;

            status = is_prime(x, &prime);

            if (status != STATUS_OK) {
                printf("x must be greater than 1.\n");
                break;
            }

            if (prime) {
                printf("%ld is prime.\n", x);
            } else {
                printf("%ld is composite.\n", x);
            }

            break;
        }

        case 's': {
            int digits[sizeof(long) * 2];
            int count;

            status = get_hex_digits(x, digits, &count);

            if (status != STATUS_OK) {
                printf("x must be non-negative.\n");
                break;
            }

            for (int i = count - 1; i >= 0; i--) {
                char digit;

                status = hex_digit(digits[i], &digit);

                if (status != STATUS_OK) {
                    printf("Error.\n");
                    return 1;
                }

                printf("%c", digit);

                if (i != 0) {
                    printf(" ");
                }
            }

            printf("\n");
            break;
        }

        case 'e': {
            unsigned long long table[10][10];

            status = calculate_power_table(x, table);

            if (status != STATUS_OK) {
                printf("For -e, x must be between 1 and 10.\n");
                break;
            }

            for (int base = 1; base <= 10; base++) {
                printf("%d: ", base);

                for (int power = 1; power <= x; power++) {
                    printf("%llu ", table[base - 1][power - 1]);
                }

                printf("\n");
            }

            break;
        }

        case 'a': {
            unsigned long long sum;

            status = calculate_sum(x, &sum);

            if (status == STATUS_INVALID_NUMBER) {
                printf("x must be a natural number.\n");
                break;
            }

            if (status == STATUS_OVERFLOW) {
                printf("Overflow while calculating sum.\n");
                break;
            }

            printf("Sum = %llu\n", sum);
            break;
        }

        case 'f': {
            unsigned long long factorial;

            status = calculate_factorial(x, &factorial);

            if (status == STATUS_INVALID_NUMBER) {
                printf("Factorial is not defined for negative numbers.\n");
                break;
            }

            if (status == STATUS_OVERFLOW) {
                printf("Factorial is too large.\n");
                break;
            }

            printf("%ld! = %llu\n", x, factorial);
            break;
        }
    }

    return 0;
}