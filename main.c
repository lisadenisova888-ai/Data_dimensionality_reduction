#include "my_libs/dataset.h"

#include <stdio.h>

static void print_first_rows(const Matrix *matrix, size_t limit)
{
    for (size_t row = 0; row<limit && row<matrix->rows; row++) {
        for (size_t column = 0; column < matrix->cols; column++) {
            printf("%.2f%c", matrix->values[row * matrix->cols + column],
                   (column + 1 == matrix->cols) ? '\n' : ' ');
        }
    }
}

int main(int argc, char *argv[])
{
    const char *path = (argc > 1) ? argv[1] : "data/wine/wine.data";
    Dataset dataset = {{0, 0, NULL}, NULL};
    size_t class_count[3] = {0, 0, 0};

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
    printf("First 3 feature rows before standardization:\n");
    print_first_rows(&dataset.features, 3);

    if (!dataset_standardize(&dataset.features)) {
        printf("Cannot standardize dataset\n");
        dataset_destroy(&dataset);
        return 1;
    }

    printf("\nFirst 3 standardized feature rows:\n");
    print_first_rows(&dataset.features, 3);
    printf("\nLabels are stored separately and are not PCA features.\n");
    dataset_destroy(&dataset);
    return 0;
}
