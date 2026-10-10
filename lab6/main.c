#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <stdint.h>
#include <math.h>
#include <float.h>
#include <stdarg.h>

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

typedef Status (*RealFunction)(double, double, double *);

Status bisect(RealFunction function, double left, double right, double eps, double *result) {
    if (function == NULL || result == NULL || !isfinite(left) || !isfinite(right) ||
        left >= right) {
        return INVALID;
    }

    Status status = valid_epsilon(eps);

    if (status != OK) {
        return status;
    }

    double a, b;

    status = function(left, eps, &a);
    if (status == OK) {
        status = function(right, eps, &b);
    }

    if (status != OK) {
        return status;
    }

    if (!isfinite(a) || !isfinite(b)) {
        return OVERFLOW;
    }

    if (a == 0) {
        *result = left;

        return OK;
    }

    if (b == 0) {
        *result = right;

        return OK;
    }

    if ((a > 0 && b > 0) || (a < 0 && b < 0)) {
        return INVALID;
    }

    for (int i = 0; i < 1000; i++) {
        double middle = left / 2 + right / 2;

        if (right / 2 - left / 2 <= eps) {
            *result = middle;

            return OK;
        }

        if (middle == left || middle == right) {
            return NO_CONVERGENCE;
        }

        double value;

        status = function(middle, eps, &value);
        if (status != OK) {
            return status;
        }

        if (!isfinite(value)) {
            return OVERFLOW;
        }

        if (value == 0) {
            *result = middle;

            return OK;
        }

        if ((a > 0 && value > 0) || (a < 0 && value < 0)) {
            left = middle;
            a = value;
        } else {
            right = middle;
        }
    }

    return NO_CONVERGENCE;
}

typedef struct {
    double x, y;
} Point;

Status convex_polygon(double eps, int *result, int count, ...) {
    if (result == NULL || count < 3 || valid_epsilon(eps) != OK) {
        return INVALID;
    }

    if ((size_t)count > SIZE_MAX / sizeof(Point)) {
        return OVERFLOW;
    }

    Point *points = malloc(count * sizeof(Point));

    if (points == NULL) {
        return MEMORY_ERROR;
    }

    Status status = OK;
    va_list args;

    va_start(args, count);

    for (int i = 0; i < count; i++) {
        points[i].x = va_arg(args, double);
        points[i].y = va_arg(args, double);
        if (!isfinite(points[i].x) || !isfinite(points[i].y)) {
            status = INVALID;
        }
    }

    va_end(args);

    int convex = 1, direction = 0;

    for (int i = 0; i < count && status == OK; i++) {
        for (int j = i + 1; j < count; j++) {
            if (fabs(points[i].x - points[j].x) <= eps && fabs(points[i].y - points[j].y) <= eps) {
                convex = 0;
            }
        }

        Point a = points[i];
        Point b = points[(i + 1) % count];
        int edge_direction = 0;

        for (int j = 0; j < count; j++) {
            double cross = (b.x - a.x) * (points[j].y - a.y) - (b.y - a.y) * (points[j].x - a.x);

            if (!isfinite(cross)) {
                status = OVERFLOW;
                break;
            }

            if (fabs(cross) <= eps) {
                continue;
            }

            int sign = cross > eps ? 1 : -1;

            if (edge_direction != 0 && edge_direction != sign) {
                convex = 0;
            }

            edge_direction = sign;
        }

        if (direction != 0 && edge_direction != 0 && direction != edge_direction) {
            convex = 0;
        }

        if (edge_direction != 0) {
            direction = edge_direction;
        }
    }

    if (status == OK) {
        *result = convex && direction != 0;
    }

    free(points);

    return status;
}

Status polynomial(double *result, double x, int degree, ...) {
    if (!result || !isfinite(x) || degree < 0) {
        return INVALID;
    }

    va_list args;

    va_start(args, degree);

    double value = va_arg(args, double);
    Status status = isfinite(value) ? OK : INVALID;

    for (int i = 0; i < degree && status == OK; i++) {
        double coefficient = va_arg(args, double);

        if (!isfinite(coefficient)) {
            status = INVALID;
            break;
        }

        value = value * x + coefficient;
        if (!isfinite(value)) {
            status = OVERFLOW;
        }
    }

    va_end(args);
    if (status == OK) {
        *result = value;
    }

    return status;
}

Status kaprekar_number(const char *text, int base, int *result) {
    if (!text || !result || !*text || base < 2 || base > 36) {
        return INVALID;
    }

    unsigned long long number = 0;

    for (const char *p = text; *p; ++p) {
        int digit;

        if (digit_value((unsigned char)*p, &digit) != OK || digit >= base) {
            return INVALID;
        }

        if (number > (ULLONG_MAX - (unsigned)digit) / (unsigned)base) {
            return OVERFLOW;
        }

        number = number * (unsigned)base + (unsigned)digit;
    }

    *result = number == 1;
    if (!number || number == 1) {
        return OK;
    }

    if (number > ULLONG_MAX / number) {
        return OVERFLOW;
    }

    unsigned long long square = number * number;

    for (unsigned long long divisor = (unsigned)base; divisor <= square;) {
        unsigned long long left = square / divisor, right = square % divisor;
        if (right && left <= number && right == number - left) {
            *result = 1;

            return OK;
        }

        if (divisor > ULLONG_MAX / (unsigned)base) {
            break;
        }

        divisor *= (unsigned)base;
    }

    return OK;
}

Status find_kaprekar(int base, const char ***result, size_t *found, int count, ...) {
    if (!result || !found || count < 1 || base < 2 || base > 36) {
        return INVALID;
    }

    *result = NULL;
    *found = 0;
    if ((size_t)count > SIZE_MAX / sizeof(char *)) {
        return OVERFLOW;
    }

    const char **values = malloc((size_t)count * sizeof(*values));

    if (!values) {
        return MEMORY_ERROR;
    }

    va_list args;

    va_start(args, count);

    Status status = OK;

    for (int i = 0; i < count; i++) {
        const char *text = va_arg(args, const char *);
        int is_kaprekar;

        status = kaprekar_number(text, base, &is_kaprekar);
        if (status != OK) {
            break;
        }

        if (is_kaprekar) {
            values[(*found)++] = text;
        }
    }

    va_end(args);
    if (status != OK) {
        free(values);
        *found = 0;

        return status;
    }

    *result = values;

    return OK;
}

Status geometric_mean(double *result, double eps, int count, ...) {
    if (!result || count < 1 || valid_epsilon(eps) != OK) {
        return INVALID;
    }

    double sum = 0;
    int zeros = 0;
    Status status = OK;
    va_list args;

    va_start(args, count);

    for (int i = 0; i < count; i++) {
        double value = va_arg(args, double);

        if (!isfinite(value) || value < 0) {
            status = INVALID;
            break;
        }

        if (value == 0) {
            zeros = 1;
        } else {
            sum += log(value) / count;
        }
    }

    va_end(args);
    if (status != OK) {
        return status;
    }

    *result = zeros ? 0 : exp(sum);

    return isfinite(*result) ? OK : OVERFLOW;
}

Status unsigned_power(double x, unsigned long long power, double *result) {
    if (!result || !isfinite(x)) {
        return INVALID;
    }

    if (!power) {
        *result = 1;

        return OK;
    }

    double half;
    Status status = unsigned_power(x, power / 2, &half);

    if (status != OK) {
        return status;
    }

    double value = half * half;

    if (power % 2) {
        value *= x;
    }

    if (!isfinite(value)) {
        return OVERFLOW;
    }

    if (value == 0 && x != 0) {
        return OVERFLOW;
    }

    *result = value;

    return OK;
}

Status fast_power(double x, long long power, double eps, double *result) {
    if (!result || !isfinite(x) || valid_epsilon(eps) != OK) {
        return INVALID;
    }

    if (fabs(x) <= eps && power <= 0) {
        return INVALID;
    }

    unsigned long long n;
    magnitude(power, &n);
    if (power < 0) {
        x = 1 / x;
    }

    return unsigned_power(x, n, result);
}

Status equation_one(double x, double eps, double *result) {
    if (!result || !isfinite(x) || valid_epsilon(eps) != OK) {
        return INVALID;
    }

    *result = x * x - 2;

    return isfinite(*result) ? OK : OVERFLOW;
}

Status equation_two(double x, double eps, double *result) {
    if (!result || !isfinite(x) || valid_epsilon(eps) != OK) {
        return INVALID;
    }

    *result = cos(x) - x;

    return OK;
}

Status equation_three(double x, double eps, double *result) {
    if (!result || !isfinite(x) || valid_epsilon(eps) != OK) {
        return INVALID;
    }

    *result = x * x * x - x - 2;

    return isfinite(*result) ? OK : OVERFLOW;
}

int main(int argc, char **argv) {
    if (argc > 2) {
        return report(INVALID);
    }

    double eps = 1e-9;
    Status status = argc == 2 ? parse_real(argv[1], &eps) : OK;

    if (status == OK) {
        status = valid_epsilon(eps);
    }

    if (status != OK) {
        return report(status);
    }

    int convex;

    status = convex_polygon(eps, &convex, 4, 0.0, 0.0, 2.0, 0.0, 2.0, 2.0, 0.0, 2.0);
    if (status != OK) {
        return report(status);
    }

    printf("Convex square: %s\n", convex ? "yes" : "no");
    status = convex_polygon(eps, &convex, 4, 0.0, 0.0, 2.0, 0.0, 0.5, 0.5, 0.0, 2.0);
    if (status != OK) {
        return report(status);
    }

    printf("Concave polygon is convex: %s\n", convex ? "yes" : "no");

    double value;

    status = polynomial(&value, 2, 3, 1.0, -2.0, 3.0, -4.0);
    if (status != OK) {
        return report(status);
    }

    printf("Polynomial: %.15g\n", value);

    const char **numbers;
    size_t count;

    status = find_kaprekar(10, &numbers, &count, 7, "0", "1", "9", "45", "55", "99", "100");
    if (status != OK) {
        return report(status);
    }

    printf("Kaprekar base 10:");

    for (size_t i = 0; i < count; i++) {
        printf(" %s", numbers[i]);
    }

    putchar('\n');
    free(numbers);
    status = find_kaprekar(16, &numbers, &count, 4, "1", "F", "FF", "10");
    if (status != OK) {
        return report(status);
    }

    printf("Kaprekar base 16:");

    for (size_t i = 0; i < count; i++) {
        printf(" %s", numbers[i]);
    }

    putchar('\n');
    free(numbers);
    status = geometric_mean(&value, eps, 3, 2.0, 8.0, 4.0);
    if (status != OK) {
        return report(status);
    }

    printf("Geometric mean: %.15g\n", value);
    status = fast_power(2, -10, eps, &value);
    if (status != OK) {
        return report(status);
    }

    printf("2^(-10): %.15g\n", value);

    RealFunction functions[] = {equation_one, equation_two, equation_three};
    const double bounds[4][2] = {{1, 2}, {0, 1}, {1, 2}, {-2, -1}};

    for (int i = 0; i < 4; i++) {
        double accuracy = i == 3 ? eps * 10 : eps;

        status = bisect(functions[i == 3 ? 0 : i], bounds[i][0], bounds[i][1], accuracy, &value);
        if (status != OK) {
            return report(status);
        }

        printf("Root [%g, %g], eps=%g: %.15g\n", bounds[i][0], bounds[i][1], accuracy, value);
    }

    return report(fflush(stdout) == EOF ? FILE_ERROR : OK);
}
