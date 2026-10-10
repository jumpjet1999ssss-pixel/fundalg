#include <stdio.h>
#include <math.h>
#include <float.h>

typedef enum {
    OK,
    INVALID,
    OVERFLOW,
    PRECISION_ERROR
} Status;

Status parse_epsilon(const char *text, long double *result) {
    if (text == NULL || result == NULL || *text == '\0') {
        return INVALID;
    }

    int i = 0;
    int digits = 0;
    long double value = 0;

    if (text[i] == '+') {
        i++;
    }

    while (text[i] >= '0' && text[i] <= '9') {
        value = value * 10 + text[i] - '0';
        if (!isfinite(value)) {
            return OVERFLOW;
        }
        digits++;
        i++;
    }

    if (text[i] == '.') {
        i++;
        long double place = 0.1L;

        while (text[i] >= '0' && text[i] <= '9') {
            value += (text[i] - '0') * place;
            place /= 10;
            digits++;
            i++;
        }
    }

    if (digits == 0) {
        return INVALID;
    }

    int exponent = 0;
    int negative = 0;

    if (text[i] == 'e' || text[i] == 'E') {
        i++;
        if (text[i] == '+' || text[i] == '-') {
            negative = text[i] == '-';
            i++;
        }

        if (text[i] < '0' || text[i] > '9') {
            return INVALID;
        }

        while (text[i] >= '0' && text[i] <= '9') {
            if (exponent > 10000) {
                return OVERFLOW;
            }
            exponent = exponent * 10 + text[i] - '0';
            i++;
        }
    }

    if (text[i] != '\0' || value <= 0) {
        return INVALID;
    }

    for (int j = 0; j < exponent; j++) {
        value = negative ? value / 10 : value * 10;
        if (!isfinite(value) || value == 0) {
            return OVERFLOW;
        }
    }

    if (value < 64 * LDBL_EPSILON) {
        return PRECISION_ERROR;
    }

    *result = value;
    return OK;
}

Status e_limit(long double eps, long double *result) {
    if (result == NULL || !isfinite(eps) || eps <= 0) {
        return INVALID;
    }

    long double n = 1;
    while (4 / n > eps / 2) {
        n *= 2;
    }

    if (1 / n < LDBL_EPSILON) {
        return PRECISION_ERROR;
    }

    *result = powl(1 + 1 / n, n);
    return OK;
}

Status pi_limit(long double eps, long double *result) {
    if (result == NULL || !isfinite(eps) || eps <= 0) {
        return INVALID;
    }

    long double value = 4;

    for (int n = 1; n < 10000000; n++) {
        long double k = n;
        if (1 / (k * k) < eps / 4) {
            *result = value * (1 - 1 / (4 * k));
            return OK;
        }
        value *= 4 * k * (k + 1) / ((2 * k + 1) * (2 * k + 1));
    }

    return PRECISION_ERROR;
}

Status ln_limit(long double eps, long double *result) {
    if (result == NULL || !isfinite(eps) || eps <= 0) {
        return INVALID;
    }

    long double n = 1;
    while (1 / (n * n) > eps / 4) {
        n *= 2;
    }

    if (16 * n * LDBL_EPSILON > eps) {
        return PRECISION_ERROR;
    }

    long double first = n * (powl(2, 1 / n) - 1);
    long double second = 2 * n * (powl(2, 1 / (2 * n)) - 1);
    *result = 2 * second - first;

    return OK;
}

Status sqrt_limit(long double eps, long double *result) {
    if (result == NULL || !isfinite(eps) || eps <= 0) {
        return INVALID;
    }

    long double x = -0.5L;

    for (int n = 0; n < 1000; n++) {
        x = x - x * x / 2 + 1;
        if (fabsl(x * x - 2) < eps / 2) {
            *result = x;
            return OK;
        }
    }

    return PRECISION_ERROR;
}

Status gamma_limit(long double eps, long double *result) {
    if (result == NULL || !isfinite(eps) || eps <= 0) {
        return INVALID;
    }

    long double sum = 0;

    for (int n = 1; n < 10000000; n++) {
        long double k = n;
        sum += 1 / k;
        if (1 / (12 * k * k) < eps / 4) {
            *result = sum - logl(k) - 1 / (2 * k);
            return OK;
        }
    }

    return PRECISION_ERROR;
}

Status e_series(long double eps, long double *result) {
    if (result == NULL || !isfinite(eps) || eps <= 0) {
        return INVALID;
    }

    long double sum = 1;
    long double term = 1;

    for (int n = 1; n < 1000; n++) {
        term /= n;
        sum += term;
        if (2 * term / (n + 1) < eps / 4) {
            *result = sum;
            return OK;
        }
    }

    return PRECISION_ERROR;
}

Status pi_series(long double eps, long double *result) {
    if (result == NULL || !isfinite(eps) || eps <= 0) {
        return INVALID;
    }

    long double a = 1.0L / 5;
    long double b = 1.0L / 239;
    long double sum = 0;
    int sign = 1;

    for (int n = 0; n < 1000; n++) {
        sum += sign * (16 * a - 4 * b) / (2 * n + 1);
        a /= 25;
        b /= 239 * 239;
        sign = -sign;
        if ((16 * a + 4 * b) / (2 * n + 3) < eps / 4) {
            *result = sum;
            return OK;
        }
    }

    return PRECISION_ERROR;
}

Status ln_series(long double eps, long double *result) {
    if (result == NULL || !isfinite(eps) || eps <= 0) {
        return INVALID;
    }

    long double term = 1.0L / 3;
    long double sum = 0;

    for (int n = 0; n < 1000; n++) {
        sum += 2 * term / (2 * n + 1);
        term /= 9;
        if (3 * term / (2 * n + 3) < eps / 4) {
            *result = sum;
            return OK;
        }
    }

    return PRECISION_ERROR;
}

Status sqrt_series(long double eps, long double *result) {
    if (result == NULL || !isfinite(eps) || eps <= 0) {
        return INVALID;
    }

    long double product = 1;
    long double power = 0.25L;

    for (int k = 2; k < 1000; k++) {
        product *= powl(2, power);
        if (2 * power < eps / 4) {
            *result = product;
            return OK;
        }
        power /= 2;
    }

    return PRECISION_ERROR;
}

Status gamma_series(long double eps, long double *result) {
    if (result == NULL || !isfinite(eps) || eps <= 0) {
        return INVALID;
    }

    long double sum = 0;

    for (int n = 1; n < 10000000; n++) {
        long double k = n;
        sum += 1 / k - log1pl(1 / k);
        if (1 / (12 * k * k) < eps / 4) {
            *result = sum + log1pl(1 / k) - 1 / (2 * k);
            return OK;
        }
    }

    return PRECISION_ERROR;
}

typedef Status (*Equation)(long double, long double *);

Status equation_value(int number, long double x, long double eps, long double *result) {
    if (result == NULL || !isfinite(x) || !isfinite(eps) || eps <= 0) {
        return INVALID;
    }

    switch (number) {
        case 0:
            if (x <= 0) {
                return INVALID;
            }
            *result = logl(x) - 1;
            break;
        case 1:
            *result = cosl(x / 2);
            break;
        case 2:
            *result = expl(x) - 2;
            break;
        case 3:
            *result = x * x - 2;
            break;
        default:
            return INVALID;
    }

    return isfinite(*result) ? OK : OVERFLOW;
}

Status solve_equation(int number, long double left, long double right,
                      long double eps, long double *result) {
    if (result == NULL || !isfinite(left) || !isfinite(right) || left >= right ||
        !isfinite(eps) || eps <= 0) {
        return INVALID;
    }

    long double a, b;
    Status status = equation_value(number, left, eps, &a);
    if (status == OK) {
        status = equation_value(number, right, eps, &b);
    }
    if (status != OK) {
        return status;
    }
    if (a * b > 0) {
        return INVALID;
    }

    for (int n = 0; n < 1000; n++) {
        long double middle = (left + right) / 2;
        if ((right - left) / 2 <= eps / 4) {
            *result = middle;
            return OK;
        }
        if (middle == left || middle == right) {
            return PRECISION_ERROR;
        }

        long double value;
        status = equation_value(number, middle, eps, &value);
        if (status != OK) {
            return status;
        }

        if ((a < 0 && value < 0) || (a > 0 && value > 0)) {
            left = middle;
            a = value;
        } else {
            right = middle;
        }
    }

    return PRECISION_ERROR;
}

Status e_equation(long double eps, long double *result) {
    return solve_equation(0, 2, 3, eps, result);
}

Status pi_equation(long double eps, long double *result) {
    return solve_equation(1, 3, 4, eps, result);
}

Status ln_equation(long double eps, long double *result) {
    return solve_equation(2, 0, 1, eps, result);
}

Status sqrt_equation(long double eps, long double *result) {
    return solve_equation(3, 1, 2, eps, result);
}

Status gamma_equation(long double eps, long double *result) {
    if (result == NULL || !isfinite(eps) || eps <= 0) {
        return INVALID;
    }

    long double end = 16;
    while ((end + 1) * expl(-end) > eps / 16) {
        end *= 2;
    }

    long double upper = powl(end, 0.25L);
    long double previous = 0;
    long double integral = 0;
    int converged = 0;

    for (int n = 16; n <= 4194304; n *= 2) {
        long double step = upper / n;
        long double sum = 0;

        for (int i = 0; i < n; i++) {
            long double u = (i + 0.5L) * step;
            sum += 16 * u * u * u * logl(u) * expl(-u * u * u * u);
        }

        integral = sum * step;
        if (n >= 64 && fabsl(integral - previous) < eps / 16) {
            converged = 1;
            break;
        }
        previous = integral;
    }

    if (!converged) {
        return PRECISION_ERROR;
    }

    long double left = 0;
    long double right = 1;
    long double target = expl(integral);

    for (int n = 0; n < 1000; n++) {
        long double middle = (left + right) / 2;
        if ((right - left) / 2 <= eps / 4) {
            *result = middle;
            return OK;
        }
        if (middle == left || middle == right) {
            return PRECISION_ERROR;
        }

        if (expl(-middle) > target) {
            left = middle;
        } else {
            right = middle;
        }
    }

    return PRECISION_ERROR;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <epsilon>\n", argv[0]);
        return 1;
    }

    long double eps;
    Status status = parse_epsilon(argv[1], &eps);
    if (status != OK) {
        fprintf(stderr, "Invalid epsilon or unsupported precision\n");
        return 1;
    }

    const char *names[] = {"e", "pi", "ln2", "sqrt2", "gamma"};
    Equation limits[] = {e_limit, pi_limit, ln_limit, sqrt_limit, gamma_limit};
    Equation series[] = {e_series, pi_series, ln_series, sqrt_series, gamma_series};
    Equation equations[] = {e_equation, pi_equation, ln_equation, sqrt_equation, gamma_equation};

    for (int i = 0; i < 5; i++) {
        long double a, b, c;
        status = limits[i](eps, &a);
        if (status == OK) {
            status = series[i](eps, &b);
        }
        if (status == OK) {
            status = equations[i](eps, &c);
        }
        if (status != OK) {
            fprintf(stderr, "%s: calculation failed (%d)\n", names[i], status);
            return 1;
        }

        printf("%s: limit=%.18Lg series=%.18Lg equation=%.18Lg\n", names[i], a, b, c);
    }

    if (fflush(stdout) == EOF || ferror(stdout)) {
        fprintf(stderr, "Output error\n");
        return 1;
    }

    return 0;
}
