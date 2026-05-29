#include "dbscan.h"

#include <stdlib.h>

//проверяет, есть ли индекс точки в списке соседей
static int contains_index(const size_t *indices, size_t count, size_t index)
{
    for (size_t current = 0; current < count; current++) {
        if (indices[current] == index) {
            return 1;
        }
    }
    return 0;
}

//добавляет в очередь новые точки, которых там ещё нет
static size_t append_neighbors(size_t *queue, size_t queue_size,
                               size_t capacity, const size_t *neighbors,
                               size_t neighbor_count)
{
    for (size_t index = 0; index < neighbor_count; index++) {
        if (queue_size < capacity
            && !contains_index(queue, queue_size, neighbors[index])) {
            queue[queue_size] = neighbors[index];
            queue_size++;
        }
    }
    return queue_size;
}

//расширяет один кластер от найденной основной точки
static int expand_cluster(const Matrix *points, const KDNode *tree,
                          double radius, size_t min_points,
                          DBSCANResult *result, size_t start,
                          int cluster_id, size_t *neighbors,
                          size_t *queue)
{
    size_t queue_size;
    size_t queue_position = 0;

    queue_size = kdtree_range_query(tree, points, start, radius, neighbors,
                                    points->rows);
    if (queue_size < min_points) {
        result->labels[start] = DBSCAN_NOISE;
        return 1;
    }

    result->labels[start] = cluster_id;
    for (size_t index = 0; index < queue_size; index++) {
        queue[index] = neighbors[index];
    }

    while (queue_position < queue_size) {
        size_t point = queue[queue_position];
        size_t neighbor_count;

        queue_position++;
        if (result->labels[point] == DBSCAN_NOISE) {
            result->labels[point] = cluster_id;
        }
        if (result->labels[point] != DBSCAN_UNVISITED) {
            continue;
        }

        result->labels[point] = cluster_id;
        neighbor_count = kdtree_range_query(tree, points, point, radius,
                                            neighbors, points->rows);
        if (neighbor_count >= min_points) {
            queue_size = append_neighbors(queue, queue_size, points->rows,
                                          neighbors, neighbor_count);
        }
    }
    return 1;
}

//кластеризация PCA-проекции алгоритмом DBSCAN через range query KD-дерева
int dbscan_fit(const Matrix *points, const KDNode *tree, double radius,
               size_t min_points, DBSCANResult *result)
{
    size_t *neighbors;
    size_t *queue;
    int cluster_id = 0;

    if (!matrix_is_valid(points) || points->cols < 2 || tree == NULL
        || radius <= 0.0 || min_points == 0 || result == NULL) {
        return 0;
    }

    result->labels = malloc(points->rows * sizeof(int));
    result->cluster_count = 0;
    result->noise_count = 0;
    result->min_points = min_points;
    result->radius = radius;
    if (result->labels == NULL) {
        dbscan_destroy(result);
        return 0;
    }

    neighbors = malloc(points->rows * sizeof(size_t));
    queue = malloc(points->rows * sizeof(size_t));
    if (neighbors == NULL || queue == NULL) {
        free(neighbors);
        free(queue);
        dbscan_destroy(result);
        return 0;
    }

    for (size_t row = 0; row < points->rows; row++) {
        result->labels[row] = DBSCAN_UNVISITED;
    }

    for (size_t row = 0; row < points->rows; row++) {
        if (result->labels[row] != DBSCAN_UNVISITED) {
            continue;
        }
        if (!expand_cluster(points, tree, radius, min_points, result, row,
                            cluster_id, neighbors, queue)) {
            free(neighbors);
            free(queue);
            dbscan_destroy(result);
            return 0;
        }
        if (result->labels[row] == cluster_id) {
            cluster_id++;
        }
    }

    result->cluster_count = (size_t)cluster_id;
    for (size_t row = 0; row < points->rows; row++) {
        if (result->labels[row] == DBSCAN_NOISE) {
            result->noise_count++;
        }
    }

    free(neighbors);
    free(queue);
    return 1;
}

//освобождение памяти результата DBSCAN
void dbscan_destroy(DBSCANResult *result)
{
    if (result == NULL) {
        return;
    }
    free(result->labels);
    result->labels = NULL;
    result->cluster_count = 0;
    result->noise_count = 0;
    result->min_points = 0;
    result->radius = 0.0;
}
