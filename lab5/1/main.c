#include <stdio.h>
#include <float.h>
#include <math.h>

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

Status parse_real(const char *text, double *result) {
    if (text == NULL || result == NULL || text[0] == '\0') {
        return INVALID;
    }

    size_t i = 0;
    int negative = 0;

    if (text[i] == '-' || text[i] == '+') {
        negative = text[i] == '-';
        i++;
    }

    long double value = 0;
    int has_digits = 0;
    int nonzero = 0;

    while (text[i] >= '0' && text[i] <= '9') {
        int digit = text[i] - '0';
        value = value * 10 + digit;
        has_digits = 1;
        if (digit != 0) {
            nonzero = 1;
        }
        if (!isfinite(value)) {
            return OVERFLOW;
        }
        i++;
    }

    if (text[i] == '.') {
        i++;
        long double place = 0.1L;

        while (text[i] >= '0' && text[i] <= '9') {
            int digit = text[i] - '0';
            value += digit * place;
            place /= 10;
            has_digits = 1;
            if (digit != 0) {
                nonzero = 1;
            }
            i++;
        }
    }

    if (!has_digits) {
        return INVALID;
    }

    int exponent = 0;
    int exponent_negative = 0;

    if (text[i] == 'e' || text[i] == 'E') {
        i++;
        if (text[i] == '-' || text[i] == '+') {
            exponent_negative = text[i] == '-';
            i++;
        }

        if (text[i] < '0' || text[i] > '9') {
            return INVALID;
        }

        while (text[i] >= '0' && text[i] <= '9') {
            if (exponent < 10000) {
                exponent = exponent * 10 + text[i] - '0';
                if (exponent > 10000) {
                    exponent = 10000;
                }
            }
            i++;
        }
    }

    if (text[i] != '\0') {
        return INVALID;
    }

    if (!nonzero) {
        *result = negative ? -0.0 : 0.0;
        return OK;
    }

    for (int j = 0; j < exponent; j++) {
        if (exponent_negative) {
            value /= 10;
        } else {
            value *= 10;
        }
        if (!isfinite(value) || value == 0) {
            return OVERFLOW;
        }
    }

    if (value > DBL_MAX || value == 0) {
        return OVERFLOW;
    }

    double number = (double)value;

    if (number == 0) {
        return OVERFLOW;
    }

    *result = negative ? -number : number;

    return OK;
}

Status valid_epsilon(double eps) {
    if (!isfinite(eps) || eps <= 0) {
        return INVALID;
    }

    if (eps < 1e-14) {
        return NO_CONVERGENCE;
    }

    return OK;
}

Status series_sum(char kind, double x, double eps, double *result) {
    if (!result || (kind < 'a' || kind > 'd') || !isfinite(x)) {
        return INVALID;
    }

    Status status = valid_epsilon(eps);

    if (status != OK) {
        return status;
    }

    double abs_x = fabs(x);

    if ((kind == 'c' && abs_x >= 1) || (kind == 'd' && abs_x > 1)) {
        return INVALID;
    }

    double square = x * x;

    if (!isfinite(square)) {
        return OVERFLOW;
    }

    double term = kind == 'd' ? -square / 2 : 1;
    double sum = term;

    for (int n = kind == 'd' ? 1 : 0; n < 10000000; n++) {
        double k = (double)n;
        double ratio;

        switch (kind) {
            case 'a':
                ratio = x / (k + 1);
                break;
            case 'b':
                ratio = -square / ((2 * k + 1) * (2 * k + 2));
                break;
            case 'c':
                ratio = 27 * square * (k + 1) * (k + 1) * (k + 1) /
                        ((3 * k + 1) * (3 * k + 2) * (3 * k + 3));
                break;
            default:
                ratio = -square * (2 * k + 1) / (2 * k + 2);
                break;
        }

        double next = term * ratio;

        if (!isfinite(next)) {
            return OVERFLOW;
        }

        if (kind == 'd') {
            double following = -next * square * (2 * k + 3) / (2 * k + 4);
            double error = (fabs(next) - fabs(following)) / 2;

            if (error <= eps / 4) {
                *result = sum + next / 2;

                return OK;
            }
        }

        double bound = fabs(next);

        if (kind == 'a' && x >= 0) {
            double r = abs_x / (k + 2);

            if (r >= 1) {
                bound = DBL_MAX;
            } else {
                bound /= 1 - r;
            }
        } else if (kind == 'c') {
            double r = 27 * square * ((k + 2) * (k + 2) * (k + 2)) /
                       ((3 * k + 4) * (3 * k + 5) * (3 * k + 6));
            if (r >= 1) {
                bound = DBL_MAX;
            } else {
                bound /= 1 - r;
            }
        } else if (fabs(ratio) > 1) {
            bound = DBL_MAX;
        }

        if (bound <= eps / 4) {
            *result = sum;

            return OK;
        }

        double updated = sum + next;

        if (!isfinite(updated)) {
            return OVERFLOW;
        }

        if (fabs(updated) * DBL_EPSILON > eps / 8) {
            return NO_CONVERGENCE;
        }

        sum = updated;
        term = next;
    }

    return NO_CONVERGENCE;
}

int main(int argc, char **argv) {
    if (argc != 3) {
        return report(INVALID);
    }

    double eps, x;
    Status status = parse_real(argv[1], &eps);

    if (status == OK) {
        status = valid_epsilon(eps);
    }

    if (status == OK) {
        status = parse_real(argv[2], &x);
    }

    if (status != OK) {
        return report(status);
    }

    int failed = 0;

    for (char kind = 'a'; kind <= 'd'; kind++) {
        double value;

        status = series_sum(kind, x, eps, &value);
        if (status == OK) {
            printf("%c: %.15g\n", kind, value);
        } else {
            printf("%c: calculation failed\n", kind);
            fflush(stdout);
            report(status);
            failed = 1;
        }
    }

    if (fflush(stdout) == EOF) {
        return report(FILE_ERROR);
    }

    return failed;
}
