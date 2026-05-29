#ifndef SPATIAL_SVG_H
#define SPATIAL_SVG_H

#include "point2d.h"

int spatial_svg_write_nearest(const char *path, const Point2D *points,
                              size_t count, Point2D query,
                              size_t nearest_index);
int spatial_svg_write_range(const char *path, const Point2D *points,
                            size_t count, Point2D query,
                            const size_t *range_indices,
                            size_t range_count);

#endif
