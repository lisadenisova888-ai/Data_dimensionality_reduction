#include "my_libs/brute_force.h"
#include "my_libs/dataset.h"
#include "my_libs/dbscan.h"
#include "my_libs/kdtree.h"
#include "my_libs/kmeans.h"
#include "my_libs/pca.h"
#include "my_libs/svg.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define KMEANS_CLUSTERS 3
#define KMEANS_MAX_ITERATIONS 100
#define DBSCAN_RADIUS 0.75
#define DBSCAN_MIN_POINTS 4
#define SPATIAL_REPEATS 2000
#define SPATIAL_RADIUS 1.0

//выводит первые строки PCA-проекции для проверки результата
static void print_projection(const PCAResult *pca, const int *labels,
                             size_t limit)
{
    for (size_t row = 0; row < limit && row < pca->projection.rows; row++) {
        printf("Wine %lu: PC1=%7.3f PC2=%7.3f class=%d\n",
               (unsigned long)(row + 1),
               pca->projection.values[row * pca->projection.cols],
               pca->projection.values[row * pca->projection.cols + 1],
               labels[row]);
    }
}

//выводит количество точек в каждом кластере k-means
static void print_kmeans_cluster_counts(const KMeansResult *kmeans,
                                        size_t rows)
{
    for (size_t cluster = 0; cluster < kmeans->clusters; cluster++) {
        size_t count = 0;

        for (size_t row = 0; row < rows; row++) {
            if (kmeans->labels[row] == (int)cluster) {
                count++;
            }
        }
        printf("Cluster %lu: %lu points\n",
               (unsigned long)(cluster + 1), (unsigned long)count);
    }
}

//выводит количество точек в каждом кластере DBSCAN
static void print_dbscan_cluster_counts(const DBSCANResult *dbscan,
                                        size_t rows)
{
    for (size_t cluster = 0; cluster < dbscan->cluster_count; cluster++) {
        size_t count = 0;

        for (size_t row = 0; row < rows; row++) {
            if (dbscan->labels[row] == (int)cluster) {
                count++;
            }
        }
        printf("Cluster %lu: %lu points\n",
               (unsigned long)(cluster + 1), (unsigned long)count);
    }
    printf("Noise: %lu points\n", (unsigned long)dbscan->noise_count);
}

//сравнивает два набора индексов без учёта порядка элементов
static int same_index_sets(const size_t *left, size_t left_count,
                           const size_t *right, size_t right_count)
{
    if (left_count != right_count) {
        return 0;
    }

    for (size_t i = 0; i < left_count; i++) {
        int found = 0;

        for (size_t j = 0; j < right_count; j++) {
            if (left[i] == right[j]) {
                found = 1;
                break;
            }
        }
        if (!found) {
            return 0;
        }
    }
    return 1;
}

//печатает результаты сравнения пространственных запросов
static void print_spatial_report(size_t brute_nearest, size_t tree_nearest,
                                 size_t brute_count, size_t tree_count,
                                 const size_t *brute_range,
                                 const size_t *tree_range,
                                 double brute_nearest_time,
                                 double tree_nearest_time,
                                 double brute_range_time,
                                 double tree_range_time)
{
    printf("\nSpatial queries:\n");
    printf("Nearest brute force: Wine %lu\n",
           (unsigned long)(brute_nearest + 1));
    printf("Nearest k-d tree:    Wine %lu\n",
           (unsigned long)(tree_nearest + 1));
    printf("Nearest equal: %s\n",
           (brute_nearest == tree_nearest) ? "yes" : "no");
    printf("Nearest time for %lu repeats: brute=%.6f sec, tree=%.6f sec\n",
           (unsigned long)SPATIAL_REPEATS, brute_nearest_time,
           tree_nearest_time);
    printf("Range radius %.2f: brute=%lu, tree=%lu, equal=%s\n",
           SPATIAL_RADIUS, (unsigned long)brute_count,
           (unsigned long)tree_count,
           same_index_sets(brute_range, brute_count, tree_range, tree_count)
               ? "yes" : "no");
    printf("Range time for %lu repeats: brute=%.6f sec, tree=%.6f sec\n",
           (unsigned long)SPATIAL_REPEATS, brute_range_time,
           tree_range_time);
}

int main(int argc, char *argv[])
{
    const char *path = (argc > 1) ? argv[1] : "data/wine/wine.data";
    Dataset dataset = {{0, 0, NULL}, NULL};
    PCAResult pca = {{0, 0, NULL}, {0, 0, NULL}, {0.0, 0.0}, {0.0, 0.0}};
    KMeansResult kmeans = {{0, 0, NULL}, NULL, 0, 0};
    DBSCANResult dbscan = {NULL, 0, 0, 0, 0.0};
    KDNode *tree = NULL;
    size_t *brute_range = NULL;
    size_t *tree_range = NULL;
    size_t class_count[3] = {0, 0, 0};
    size_t query_index = 0;
    size_t brute_nearest = (size_t)-1;
    size_t tree_nearest = (size_t)-1;
    size_t brute_range_count = 0;
    size_t tree_range_count = 0;
    clock_t start;
    double brute_nearest_time;
    double tree_nearest_time;
    double brute_range_time;
    double tree_range_time;
    double dbscan_time;
    int status = 1;

    if (!dataset_load_wine(path, &dataset)) {
        printf("Cannot read Wine dataset: %s\n", path);
        return 1;
    }

    for (size_t row = 0; row < dataset.features.rows; row++) {
        class_count[dataset.labels[row] - 1]++;
    }

    printf("Wine dataset: %lu wines, %lu features\n",
           (unsigned long)dataset.features.rows,
           (unsigned long)dataset.features.cols);
    printf("Classes: 1=%lu, 2=%lu, 3=%lu\n",
           (unsigned long)class_count[0], (unsigned long)class_count[1],
           (unsigned long)class_count[2]);

    if (!dataset_standardize(&dataset.features)) {
        printf("Cannot standardize dataset\n");
        goto cleanup;
    }

    if (!pca_fit_transform(&dataset.features, &pca)) {
        printf("Cannot calculate PCA projection\n");
        goto cleanup;
    }

    printf("\nPCA explained variance:\n");
    printf("PC1: %.2f%%\n", pca.explained_variance[0] * 100.0);
    printf("PC2: %.2f%%\n", pca.explained_variance[1] * 100.0);
    printf("Together: %.2f%%\n",
           (pca.explained_variance[0] + pca.explained_variance[1]) * 100.0);
    printf("\nFirst 5 projected wines:\n");
    print_projection(&pca, dataset.labels, 5);
    printf("\nLabels are printed for checking, not used in PCA.\n");

    if (!kmeans_fit(&pca.projection, KMEANS_CLUSTERS,
                    KMEANS_MAX_ITERATIONS, &kmeans)) {
        printf("Cannot calculate k-means clusters\n");
        goto cleanup;
    }

    printf("\nK-means clusters:\n");
    printf("Iterations: %lu\n", (unsigned long)kmeans.iterations);
    print_kmeans_cluster_counts(&kmeans, pca.projection.rows);

    tree = kdtree_build(&pca.projection);
    brute_range = malloc(pca.projection.rows * sizeof(size_t));
    tree_range = malloc(pca.projection.rows * sizeof(size_t));
    if (tree == NULL || brute_range == NULL || tree_range == NULL) {
        printf("Cannot prepare spatial query structures\n");
        goto cleanup;
    }

    start = clock();
    for (size_t i = 0; i < SPATIAL_REPEATS; i++) {
        brute_nearest = brute_force_nearest(&pca.projection, query_index,
                                            query_index);
    }
    brute_nearest_time = (double)(clock() - start) / CLOCKS_PER_SEC;

    start = clock();
    for (size_t i = 0; i < SPATIAL_REPEATS; i++) {
        tree_nearest = kdtree_nearest(tree, &pca.projection, query_index,
                                      query_index);
    }
    tree_nearest_time = (double)(clock() - start) / CLOCKS_PER_SEC;

    brute_range_count = brute_force_range_query(&pca.projection, query_index,
                                                SPATIAL_RADIUS, brute_range,
                                                pca.projection.rows);
    tree_range_count = kdtree_range_query(tree, &pca.projection, query_index,
                                          SPATIAL_RADIUS, tree_range,
                                          pca.projection.rows);

    start = clock();
    for (size_t i = 0; i < SPATIAL_REPEATS; i++) {
        brute_force_range_query(&pca.projection, query_index, SPATIAL_RADIUS,
                                brute_range, pca.projection.rows);
    }
    brute_range_time = (double)(clock() - start) / CLOCKS_PER_SEC;

    start = clock();
    for (size_t i = 0; i < SPATIAL_REPEATS; i++) {
        kdtree_range_query(tree, &pca.projection, query_index, SPATIAL_RADIUS,
                           tree_range, pca.projection.rows);
    }
    tree_range_time = (double)(clock() - start) / CLOCKS_PER_SEC;

    print_spatial_report(brute_nearest, tree_nearest, brute_range_count,
                         tree_range_count, brute_range, tree_range,
                         brute_nearest_time, tree_nearest_time,
                         brute_range_time, tree_range_time);

    start = clock();
    if (!dbscan_fit(&pca.projection, tree, DBSCAN_RADIUS, DBSCAN_MIN_POINTS,
                    &dbscan)) {
        printf("Cannot calculate DBSCAN clusters\n");
        goto cleanup;
    }
    dbscan_time = (double)(clock() - start) / CLOCKS_PER_SEC;

    printf("\nDBSCAN clusters:\n");
    printf("Radius: %.2f\n", dbscan.radius);
    printf("Min points: %lu\n", (unsigned long)dbscan.min_points);
    printf("Clusters: %lu\n", (unsigned long)dbscan.cluster_count);
    print_dbscan_cluster_counts(&dbscan, pca.projection.rows);
    printf("Time: %.6f sec\n", dbscan_time);

    if (!svg_write_project_dashboard("results/project_dashboard.svg",
                                     &pca.projection, dataset.labels,
                                     kmeans.labels, &kmeans.centroids,
                                     kmeans.clusters, &dbscan, query_index,
                                     tree_nearest, tree_range,
                                     tree_range_count, SPATIAL_RADIUS)) {
        printf("Cannot save project dashboard SVG\n");
        goto cleanup;
    }

    printf("\nSVG saved: results/project_dashboard.svg\n");
    status = 0;

cleanup:
    dbscan_destroy(&dbscan);
    kdtree_destroy(tree);
    free(brute_range);
    free(tree_range);
    kmeans_destroy(&kmeans);
    pca_destroy(&pca);
    dataset_destroy(&dataset);
    return status;
}
