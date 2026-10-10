#include <stdio.h>
#include <math.h>
#include <float.h>

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

typedef Status (*RealFunction)(double, double, double *);

Status integrate(RealFunction function, double a, double b, double eps, double *result) {
    if (function == NULL || result == NULL || !isfinite(a) || !isfinite(b) || a >= b) {
        return INVALID;
    }

    Status status = valid_epsilon(eps);

    if (status != OK) {
        return status;
    }

    double previous = 0;

    for (int n = 8; n <= 16777216; n *= 2) {
        double step = (b - a) / n;
        double sum = 0;

        for (int i = 0; i < n; i++) {
            double x = a + (i + 0.5) * step;
            double value;

            status = function(x, eps, &value);
            if (status != OK) {
                return status;
            }

            if (!isfinite(value)) {
                return OVERFLOW;
            }

            sum += value;
        }

        double current = sum * step;

        if (!isfinite(current)) {
            return OVERFLOW;
        }

        if (n >= 32 && fabs(current - previous) < eps / 2) {
            *result = current;

            return OK;
        }

        previous = current;
    }

    return NO_CONVERGENCE;
}

Status integrand_a(double x, double eps, double *result) {
    if (!result || !isfinite(x) || x < 0 || x > 1 || valid_epsilon(eps) != OK) {
        return INVALID;
    }

    *result = x == 0 ? 1 : log1p(x) / x;

    return OK;
}

Status integrand_b(double x, double eps, double *result) {
    if (!result || !isfinite(x) || x < 0 || x > 1 || valid_epsilon(eps) != OK) {
        return INVALID;
    }

    *result = exp(-x * x / 2);

    return OK;
}

Status integrand_c(double t, double eps, double *result) {
    if (!result || !isfinite(t) || t < 0 || valid_epsilon(eps) != OK) {
        return INVALID;
    }

    *result = t * exp(-t);

    return OK;
}

Status integrand_d(double t, double eps, double *result) {
    if (!result || !isfinite(t) || t < 0 || t > 1 || valid_epsilon(eps) != OK) {
        return INVALID;
    }

    double x = pow(t, 4);

    *result = t == 0 ? 0 : 4 * t * t * t * exp(x * log(x));

    return OK;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        return report(INVALID);
    }

    double eps;
    Status status = parse_real(argv[1], &eps);

    if (status == OK) {
        status = valid_epsilon(eps);
    }

    if (status != OK) {
        return report(status);
    }

    RealFunction functions[] = {integrand_a, integrand_b, integrand_c, integrand_d};

    for (int i = 0; i < 4; i++) {
        double end = 1, value;

        if (i == 2) {
            end = 8;

            while ((end + 1) * exp(-end) > eps / 4) {
                end *= 2;
            }
        }

        status = integrate(functions[i], 0, end, eps / 4, &value);
        if (status != OK) {
            return report(status);
        }

        printf("%c: %.15g\n", 'a' + i, value);
    }

    return report(fflush(stdout) == EOF ? FILE_ERROR : OK);
}
