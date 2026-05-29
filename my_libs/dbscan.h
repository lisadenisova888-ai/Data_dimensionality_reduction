#ifndef DBSCAN_H
#define DBSCAN_H

#include "kdtree.h"
#include "matrix.h"

#include <stddef.h>

#define DBSCAN_NOISE (-1)
#define DBSCAN_UNVISITED (-2)

typedef struct {
    int *labels;          //номер кластера для каждой точки или DBSCAN_NOISE
    size_t cluster_count; //количество найденных кластеров
    size_t noise_count;   //количество шумовых точек
    size_t min_points;    //минимальное число соседей для основной точки
    double radius;        //радиус поиска соседей
} DBSCANResult;

//кластеризация PCA-проекции алгоритмом DBSCAN через range query KD-дерева
int dbscan_fit(const Matrix *points, const KDNode *tree, double radius,
               size_t min_points, DBSCANResult *result);

//освобождение памяти результата DBSCAN
void dbscan_destroy(DBSCANResult *result);

#endif
