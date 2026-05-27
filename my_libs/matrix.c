#include "matrix.h"

#include <stdio.h>
#include <stdlib.h>
//проверка выхода индексов за границы матрицы
int matrix_contains(const Matrix *matrix, size_t row, size_t col)
{
    return (matrix_is_valid(matrix) && (row < matrix->rows) && (col < matrix->cols));
}
//создание матрицы инициализированной нулями
Matrix matrix_create(size_t rows, size_t cols)
{
    Matrix matrix = {0, 0, NULL};
    

    if (rows == 0 || cols == 0) {
        return matrix;
    }

    size_t element_count = rows * cols;
    
    matrix.values = calloc(element_count, sizeof(double));
    if (matrix.values == NULL) {
        return matrix;
    }

    matrix.rows = rows;
    matrix.cols = cols;
    return matrix;
}
//удаление матрицы
void matrix_destroy(Matrix *matrix)
{
    if (matrix == NULL) {
        return;
    }

    free(matrix->values);
    matrix->values = NULL;
    matrix->rows = 0;
    matrix->cols = 0;
}
//проверка корректности матрицы
int matrix_is_valid(const Matrix *matrix)
{
    return ((matrix != NULL) && (matrix->values != NULL)
        && (matrix->rows > 0) && (matrix->cols > 0));
}
//записывает значение по индексам в матрицу
int matrix_set(Matrix *matrix, size_t row, size_t col, double value)
{
    if (!matrix_contains(matrix, row, col)) {
        return 0;
    }

    matrix->values[row * matrix->cols + col] = value;
    return 1;
}
//записывает по адресу value элемент матрицы по индексам
int matrix_get(const Matrix *matrix, size_t row, size_t col, double *value)
{
    if (value == NULL || !matrix_contains(matrix, row, col)) {
        return 0;
    }

    *value = matrix->values[row * matrix->cols + col];
    return 1;
}
//заполняет матрицу значениями из массива
int matrix_fill(Matrix *matrix, const double *values, size_t count)
{
    if (!matrix_is_valid(matrix) || values == NULL
        || (count != matrix->rows * matrix->cols)) {
        return 0;
    }

    for (size_t i = 0; i<count; i++) {
        matrix->values[i] = values[i];
    }

    return 1;
}
//транспонирование матрицы
Matrix matrix_transpose(const Matrix *matrix)
{
    Matrix result = {0, 0, NULL};

    if (!matrix_is_valid(matrix)) {
        return result;
    }

    result = matrix_create(matrix->cols, matrix->rows);
    if (!matrix_is_valid(&result)) {
        return result;
    }

    for (size_t row = 0; row < matrix->rows; row++) {
        for (size_t col = 0; col < matrix->cols; col++) {

            result.values[col * result.cols + row] =
                matrix->values[row * matrix->cols + col];

        }
    }

    return result;
}
//произведение матриц
Matrix matrix_multiply(const Matrix *left, const Matrix *right)
{
    Matrix result;
    result.cols = 0;
    result.rows = 0;
    result.values = NULL;

    if (!matrix_is_valid(left) || !matrix_is_valid(right)
        || left->cols != right->rows) { //корректность матриц, также для умножения 
        return result;//необходимо равенство числа колонок первой матрицы и строк второй
    }

    result = matrix_create(left->rows, right->cols);
    if (!matrix_is_valid(&result)) {
        return result;
    }

    for (size_t row = 0; row<result.rows; row++) {
        for (size_t col = 0; col<result.cols; col++) {
            double sum = 0.0;

            for (size_t inner = 0; inner < left->cols; inner++) {
                sum +=
                    left->values[row * left->cols + inner]
                    * right->values[inner * right->cols + col]; 
            }

            result.values[row * result.cols + col] = sum;
        }
    }

    return result;
}
//Умножение матрицы на вектор
int matrix_vector_multiply(const Matrix *matrix, const double *vector,
                           size_t vector_size, double *result)
{
    
    if (!matrix_is_valid(matrix) || vector == NULL || result == NULL
        || vector_size != matrix->cols) {
        return 0;
    }

    for (size_t row = 0; row<matrix->rows; row++) {
        result[row] = 0.0;
        for (size_t col = 0; col<matrix->cols; col++) {
            result[row] += matrix->values[row * matrix->cols + col]
                * vector[col];
        }
    }

    return 1;
}
//вывод матрицы
void matrix_print(const Matrix *matrix)
{
   
    if (!matrix_is_valid(matrix)) {
        return;
    }

    for (size_t row = 0; row < matrix->rows; row++) {
        for (size_t col = 0; col < matrix->cols; col++) {
            printf("%.2f%c", matrix->values[row * matrix->cols + col], 
                ((col + 1) == matrix->cols)  ? '\n' : ' ');
        }
    }
}
