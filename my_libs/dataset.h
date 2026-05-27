#ifndef DATASET_H
#define DATASET_H

#include "matrix.h"

typedef struct {
    Matrix features;
    int *labels;
} Dataset;

int dataset_load_wine(const char *path, Dataset *dataset);
void dataset_destroy(Dataset *dataset);
int dataset_standardize(Matrix *dataset);

#endif
