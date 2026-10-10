#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <errno.h>
#include <limits.h>

typedef enum {
    STATUS_OK,
    STATUS_INVALID_ARGUMENT,
    STATUS_INVALID_NUMBER,
    STATUS_INVALID_FLAG,
    STATUS_OVERFLOW
} Status;

Status parse_long(const char *str, long *result) {
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

Status parse_double(const char *str, double *result) {
    if (str == NULL || result == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    if (*str == '\0') {
        return STATUS_INVALID_NUMBER;
    }

    char *end;

    double value = strtod(str, &end);

    if (end == str || *end != '\0') {
        return STATUS_INVALID_NUMBER;
    }

    *result = value;

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

    if (flag[1] != 'q' &&
        flag[1] != 'm' &&
        flag[1] != 't') {
        return STATUS_INVALID_FLAG;
    }

    return STATUS_OK;
}

Status solve_quadratic(
    double a,
    double b,
    double c,
    double eps,
    double *x1,
    double *x2,
    int *root_count
) {
    if (x1 == NULL || x2 == NULL || root_count == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    if (eps <= 0.0) {
        return STATUS_INVALID_NUMBER;
    }

    if (fabs(a) < eps) {
        if (fabs(b) < eps) {
            *root_count = 0;
            return STATUS_OK;
        }

        *x1 = -c / b;
        *root_count = 1;

        return STATUS_OK;
    }

    double discriminant = b * b - 4.0 * a * c;

    if (fabs(discriminant) < eps) {
        *x1 = -b / (2.0 * a);
        *root_count = 1;

        return STATUS_OK;
    }

    if (discriminant < 0.0) {
        *root_count = 0;
        return STATUS_OK;
    }

    double sqrt_d = sqrt(discriminant);

    *x1 = (-b + sqrt_d) / (2.0 * a);
    *x2 = (-b - sqrt_d) / (2.0 * a);
    *root_count = 2;

    return STATUS_OK;
}

Status is_multiple(long first, long second, int *result) {
    if (result == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    if (first == 0 || second == 0) {
        return STATUS_INVALID_NUMBER;
    }

    *result = first % second == 0;

    return STATUS_OK;
}

Status is_right_triangle(
    double a,
    double b,
    double c,
    double eps,
    int *result
) {
    if (result == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    if (eps <= 0.0 || a <= 0.0 || b <= 0.0 || c <= 0.0) {
        return STATUS_INVALID_NUMBER;
    }

    double max = a;
    double side1 = b;
    double side2 = c;

    if (b > max) {
        max = b;
        side1 = a;
        side2 = c;
    }

    if (c > max) {
        max = c;
        side1 = a;
        side2 = b;
    }

    double left = max * max;
    double right = side1 * side1 + side2 * side2;

    *result = fabs(left - right) < eps;

    return STATUS_OK;
}

int same_double(double a, double b, double eps) {
    return fabs(a - b) < eps;
}

int same_permutation(
    double first[3],
    double second[3],
    double eps
) {
    return same_double(first[0], second[0], eps) &&
           same_double(first[1], second[1], eps) &&
           same_double(first[2], second[2], eps);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Not enough arguments.\n");
        return 1;
    }

    Status status = validate_flag(argv[1]);

    if (status != STATUS_OK) {
        printf("Invalid flag.\n");
        return 1;
    }

    char flag = argv[1][1];

    switch (flag) {
        case 'q': {
            if (argc != 6) {
                printf("Usage: %s -q <epsilon> <a> <b> <c>\n", argv[0]);
                return 1;
            }

            double eps;
            double values[3];

            if (parse_double(argv[2], &eps) != STATUS_OK ||
                parse_double(argv[3], &values[0]) != STATUS_OK ||
                parse_double(argv[4], &values[1]) != STATUS_OK ||
                parse_double(argv[5], &values[2]) != STATUS_OK) {
                printf("Invalid number.\n");
                return 1;
            }

            if (eps <= 0.0) {
                printf("Epsilon must be positive.\n");
                return 1;
            }

            int permutations[6][3] = {
                {0, 1, 2},
                {0, 2, 1},
                {1, 0, 2},
                {1, 2, 0},
                {2, 0, 1},
                {2, 1, 0}
            };

            double used[6][3];
            int used_count = 0;

            for (int i = 0; i < 6; i++) {
                double current[3] = {
                    values[permutations[i][0]],
                    values[permutations[i][1]],
                    values[permutations[i][2]]
                };

                int duplicate = 0;

                for (int j = 0; j < used_count; j++) {
                    if (same_permutation(current, used[j], eps)) {
                        duplicate = 1;
                        break;
                    }
                }

                if (duplicate) {
                    continue;
                }

                used[used_count][0] = current[0];
                used[used_count][1] = current[1];
                used[used_count][2] = current[2];
                used_count++;

                double x1 = 0.0;
                double x2 = 0.0;
                int root_count = 0;

                status = solve_quadratic(
                    current[0],
                    current[1],
                    current[2],
                    eps,
                    &x1,
                    &x2,
                    &root_count
                );

                if (status != STATUS_OK) {
                    printf("Calculation error.\n");
                    return 1;
                }

                printf(
                    "a = %.6g, b = %.6g, c = %.6g: ",
                    current[0],
                    current[1],
                    current[2]
                );

                if (root_count == 0) {
                    printf("no real roots\n");
                } else if (root_count == 1) {
                    printf("x = %.10g\n", x1);
                } else {
                    printf("x1 = %.10g, x2 = %.10g\n", x1, x2);
                }
            }

            break;
        }

        case 'm': {
            if (argc != 4) {
                printf("Usage: %s -m <first> <second>\n", argv[0]);
                return 1;
            }

            long first;
            long second;

            status = parse_long(argv[2], &first);

            if (status != STATUS_OK) {
                printf("Invalid first number.\n");
                return 1;
            }

            status = parse_long(argv[3], &second);

            if (status != STATUS_OK) {
                printf("Invalid second number.\n");
                return 1;
            }

            int result;

            status = is_multiple(first, second, &result);

            if (status != STATUS_OK) {
                printf("Numbers must be non-zero.\n");
                return 1;
            }

            if (result) {
                printf("%ld is divisible by %ld.\n", first, second);
            } else {
                printf("%ld is not divisible by %ld.\n", first, second);
            }

            break;
        }

        case 't': {
            if (argc != 6) {
                printf("Usage: %s -t <epsilon> <a> <b> <c>\n", argv[0]);
                return 1;
            }

            double eps;
            double a;
            double b;
            double c;

            if (parse_double(argv[2], &eps) != STATUS_OK ||
                parse_double(argv[3], &a) != STATUS_OK ||
                parse_double(argv[4], &b) != STATUS_OK ||
                parse_double(argv[5], &c) != STATUS_OK) {
                printf("Invalid number.\n");
                return 1;
            }

            int result;

            status = is_right_triangle(a, b, c, eps, &result);

            if (status != STATUS_OK) {
                printf("Epsilon and sides must be positive.\n");
                return 1;
            }

            if (result) {
                printf("The sides form a right triangle.\n");
            } else {
                printf("The sides do not form a right triangle.\n");
            }

            break;
        }
    }

    return 0;
}