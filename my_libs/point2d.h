#ifndef POINT2D_H
#define POINT2D_H

#include "matrix.h"

#include <stddef.h>

typedef struct {
    double x;     //координата точки по PC1
    double y;     //координата точки по PC2
    size_t index; //номер исходной строки в матрице проекции
} Point2D;

//записывает точки из PCA-проекции в массив Point2D
int point2d_from_matrix(const Matrix *projection, Point2D *points,
                        size_t count);

#endif
