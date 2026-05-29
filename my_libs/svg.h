#ifndef SVG_H
#define SVG_H

#include "dbscan.h"
#include "matrix.h"

#include <stddef.h>

int svg_write_pca_projection(const char *path, const Matrix *points,
                             const int *labels);
int svg_write_kmeans_projection(const char *path, const Matrix *points,
                                const int *clusters, const Matrix *centroids,
                                size_t cluster_count);
int svg_write_dbscan_projection(const char *path, const Matrix *points,
                                const DBSCANResult *dbscan);

//сохраняет все основные графики проекта на одном SVG-листе
int svg_write_project_dashboard(const char *path, const Matrix *points,
                                const int *class_labels,
                                const int *kmeans_labels,
                                const Matrix *kmeans_centroids,
                                size_t kmeans_cluster_count,
                                const DBSCANResult *dbscan,
                                size_t query_index,
                                size_t nearest_index,
                                const size_t *range_indices,
                                size_t range_count,
                                double range_radius);

#endif
