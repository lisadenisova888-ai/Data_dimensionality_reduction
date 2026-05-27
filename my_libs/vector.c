#include "vector.h"

#include <math.h>
#include <stdio.h>

double vector_dot(const double *left, const double *right, size_t size)
{
    double result = 0.0;

    if (left == NULL || right == NULL) {
        return 0.0;
    }

    for (size_t i = 0; i<size; i++) {
        result += left[i] * right[i];
    }

    return result;
}

double vector_norm(const double *vector, size_t size)
{
    return sqrt(vector_dot(vector, vector, size));
}

int vector_normalize(double *vector, size_t size)
{
    double norm;
    if (vector == 0 || size == 0) {
        return 0;
    }

    norm = vector_norm(vector, size);
    if (norm == 0.0) {
        return 0;
    }

    for (size_t i = 0; i<size; i++) {
        vector[i] /= norm;
    }

    return 1;
}

void vector_print(const double *vector, size_t size)
{
    size_t index;

    if (vector == NULL) {
        return;
    }
    for (index = 0; index<size; index++) {
        printf("%.2f%c", vector[index], ((index + 1) == size) ? '\n' : ' ');
    }
}
