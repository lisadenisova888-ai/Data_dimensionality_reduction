#include "point2d.h"

//записывает точки из PCA-проекции в массив Point2D
int point2d_from_matrix(const Matrix *projection, Point2D *points,
                        size_t count)
{
    if (!matrix_is_valid(projection) || projection->cols < 2
        || points == NULL || count < projection->rows) {
        return 0;
    }

    for (size_t row = 0; row < projection->rows; row++) {
        points[row].x = projection->values[row * projection->cols];
        points[row].y = projection->values[row * projection->cols + 1];
        points[row].index = row;
    }
    return 1;
}
