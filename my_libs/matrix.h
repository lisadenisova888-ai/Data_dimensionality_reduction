#ifndef MATRIX_H
#define MATRIX_H

#include <stddef.h>

typedef struct {
    size_t rows;
    size_t cols;
    double *values;
} Matrix;

Matrix matrix_create(size_t rows, size_t cols);
void matrix_destroy(Matrix *matrix);
int matrix_is_valid(const Matrix *matrix);
int matrix_set(Matrix *matrix, size_t row, size_t col, double value);
int matrix_get(const Matrix *matrix, size_t row, size_t col, double *value);
int matrix_fill(Matrix *matrix, const double *values, size_t count);
Matrix matrix_transpose(const Matrix *matrix);
Matrix matrix_multiply(const Matrix *left, const Matrix *right);
int matrix_vector_multiply(const Matrix *matrix, const double *vector,
                           size_t vector_size, double *result);
void matrix_print(const Matrix *matrix);

#endif
