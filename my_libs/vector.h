#ifndef VECTOR_H
#define VECTOR_H

#include <stddef.h>

double vector_dot(const double *left, const double *right, size_t size);
double vector_norm(const double *vector, size_t size);
int vector_normalize(double *vector, size_t size);
void vector_print(const double *vector, size_t size);

#endif
