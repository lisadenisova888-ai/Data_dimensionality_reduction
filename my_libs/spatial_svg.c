#include "spatial_svg.h"

#include <stdio.h>

#define SVG_WIDTH 800.0
#define SVG_HEIGHT 600.0
#define SVG_MARGIN 60.0

static void bounds(const Point2D *points, size_t count, double *min_x,
                   double *max_x, double *min_y, double *max_y)
{
    *min_x = points[0].x;
    *max_x = points[0].x;
    *min_y = points[0].y;
    *max_y = points[0].y;

    for (size_t index = 1; index < count; index++) {
        if (points[index].x < *min_x) {
            *min_x = points[index].x;
        }
        if (points[index].x > *max_x) {
            *max_x = points[index].x;
        }
        if (points[index].y < *min_y) {
            *min_y = points[index].y;
        }
        if (points[index].y > *max_y) {
            *max_y = points[index].y;
        }
    }
}

static double scale(double value, double min_value, double max_value,
                    double start, double end)
{
    if (min_value == max_value) {
        return (start + end) / 2.0;
    }
    return start + (value - min_value) * (end - start) /
        (max_value - min_value);
}

static int contains_index(const size_t *indices, size_t count, size_t index)
{
    for (size_t current = 0; current < count; current++) {
        if (indices[current] == index) {
            return 1;
        }
    }
    return 0;
}

static void write_start(FILE *file, const char *title)
{
    fprintf(file, "<svg xmlns=\"http://www.w3.org/2000/svg\" "
                  "width=\"%.0f\" height=\"%.0f\" "
                  "viewBox=\"0 0 %.0f %.0f\">\n",
                  SVG_WIDTH, SVG_HEIGHT, SVG_WIDTH, SVG_HEIGHT);
    fprintf(file, "<rect width=\"100%%\" height=\"100%%\" fill=\"white\" />\n");
    fprintf(file, "<text x=\"%.0f\" y=\"35\" font-size=\"20\" "
                  "font-family=\"Arial\">%s</text>\n", SVG_MARGIN, title);
    fprintf(file, "<rect x=\"%.0f\" y=\"%.0f\" width=\"%.0f\" "
                  "height=\"%.0f\" fill=\"#fafafa\" stroke=\"#dddddd\" />\n",
                  SVG_MARGIN, SVG_MARGIN, SVG_WIDTH - 2.0 * SVG_MARGIN,
                  SVG_HEIGHT - 2.0 * SVG_MARGIN);
    fprintf(file, "<line x1=\"%.0f\" y1=\"%.0f\" x2=\"%.0f\" y2=\"%.0f\" "
                  "stroke=\"#333\" />\n",
                  SVG_MARGIN, SVG_HEIGHT - SVG_MARGIN,
                  SVG_WIDTH - SVG_MARGIN, SVG_HEIGHT - SVG_MARGIN);
    fprintf(file, "<line x1=\"%.0f\" y1=\"%.0f\" x2=\"%.0f\" y2=\"%.0f\" "
                  "stroke=\"#333\" />\n",
                  SVG_MARGIN, SVG_MARGIN,
                  SVG_MARGIN, SVG_HEIGHT - SVG_MARGIN);
    fprintf(file, "<text x=\"%.0f\" y=\"%.0f\" font-size=\"13\" "
                  "font-family=\"Arial\">PC1</text>\n",
                  SVG_WIDTH - SVG_MARGIN - 25.0, SVG_HEIGHT - 25.0);
    fprintf(file, "<text x=\"20\" y=\"%.0f\" font-size=\"13\" "
                  "font-family=\"Arial\">PC2</text>\n", SVG_MARGIN - 20.0);
}

static double svg_x(Point2D point, double min_x, double max_x)
{
    return scale(point.x, min_x, max_x, SVG_MARGIN, SVG_WIDTH - SVG_MARGIN);
}

static double svg_y(Point2D point, double min_y, double max_y)
{
    return scale(point.y, min_y, max_y, SVG_HEIGHT - SVG_MARGIN, SVG_MARGIN);
}

static void write_nearest_legend(FILE *file)
{
    const double x = SVG_WIDTH - 190.0;
    const double y = 70.0;

    fprintf(file, "<text x=\"%.0f\" y=\"%.0f\" font-size=\"14\" "
                  "font-family=\"Arial\">Nearest query</text>\n", x, y);
    fprintf(file, "<circle cx=\"%.0f\" cy=\"%.0f\" r=\"5\" "
                  "fill=\"#999999\" opacity=\"0.85\" />\n", x + 8.0,
                  y + 21.0);
    fprintf(file, "<text x=\"%.0f\" y=\"%.0f\" font-size=\"13\" "
                  "font-family=\"Arial\">Other points</text>\n",
                  x + 22.0, y + 25.0);
    fprintf(file, "<circle cx=\"%.0f\" cy=\"%.0f\" r=\"6\" "
                  "fill=\"#d1495b\" opacity=\"0.9\" />\n", x + 8.0,
                  y + 46.0);
    fprintf(file, "<text x=\"%.0f\" y=\"%.0f\" font-size=\"13\" "
                  "font-family=\"Arial\">Nearest</text>\n",
                  x + 22.0, y + 50.0);
    fprintf(file, "<circle cx=\"%.0f\" cy=\"%.0f\" r=\"7\" "
                  "fill=\"#f4a261\" stroke=\"#111\" stroke-width=\"2\" />\n",
                  x + 8.0, y + 71.0);
    fprintf(file, "<text x=\"%.0f\" y=\"%.0f\" font-size=\"13\" "
                  "font-family=\"Arial\">Query</text>\n",
                  x + 22.0, y + 75.0);
}

static void write_range_legend(FILE *file)
{
    const double x = SVG_WIDTH - 190.0;
    const double y = 70.0;

    fprintf(file, "<text x=\"%.0f\" y=\"%.0f\" font-size=\"14\" "
                  "font-family=\"Arial\">Range query</text>\n", x, y);
    fprintf(file, "<circle cx=\"%.0f\" cy=\"%.0f\" r=\"5\" "
                  "fill=\"#bbbbbb\" opacity=\"0.85\" />\n", x + 8.0,
                  y + 21.0);
    fprintf(file, "<text x=\"%.0f\" y=\"%.0f\" font-size=\"13\" "
                  "font-family=\"Arial\">Outside</text>\n",
                  x + 22.0, y + 25.0);
    fprintf(file, "<circle cx=\"%.0f\" cy=\"%.0f\" r=\"6\" "
                  "fill=\"#2a9d8f\" opacity=\"0.9\" />\n", x + 8.0,
                  y + 46.0);
    fprintf(file, "<text x=\"%.0f\" y=\"%.0f\" font-size=\"13\" "
                  "font-family=\"Arial\">Inside range</text>\n",
                  x + 22.0, y + 50.0);
    fprintf(file, "<circle cx=\"%.0f\" cy=\"%.0f\" r=\"7\" "
                  "fill=\"#f4a261\" stroke=\"#111\" stroke-width=\"2\" />\n",
                  x + 8.0, y + 71.0);
    fprintf(file, "<text x=\"%.0f\" y=\"%.0f\" font-size=\"13\" "
                  "font-family=\"Arial\">Query</text>\n",
                  x + 22.0, y + 75.0);
}

static void svg_point(FILE *file, Point2D point, double min_x, double max_x,
                      double min_y, double max_y, const char *color,
                      double radius)
{
    double x = svg_x(point, min_x, max_x);
    double y = svg_y(point, min_y, max_y);

    fprintf(file, "<circle cx=\"%.2f\" cy=\"%.2f\" r=\"%.1f\" "
                  "fill=\"%s\" opacity=\"0.82\" />\n", x, y, radius, color);
}

static int find_point(const Point2D *points, size_t count, size_t point_index,
                      Point2D *result)
{
    for (size_t index = 0; index < count; index++) {
        if (points[index].index == point_index) {
            *result = points[index];
            return 1;
        }
    }
    return 0;
}

int spatial_svg_write_nearest(const char *path, const Point2D *points,
                              size_t count, Point2D query,
                              size_t nearest_index)
{
    FILE *file;
    double min_x;
    double max_x;
    double min_y;
    double max_y;
    Point2D nearest;

    if (path == 0 || points == 0 || count == 0) {
        return 0;
    }

    bounds(points, count, &min_x, &max_x, &min_y, &max_y);
    file = fopen(path, "w");
    if (file == 0) {
        return 0;
    }

    write_start(file, "Nearest neighbor query");
    for (size_t index = 0; index < count; index++) {
        if (points[index].index != nearest_index) {
            svg_point(file, points[index], min_x, max_x, min_y, max_y,
                      "#999999", 3.0);
        }
    }

    if (find_point(points, count, nearest_index, &nearest)) {
        fprintf(file, "<line x1=\"%.2f\" y1=\"%.2f\" x2=\"%.2f\" "
                      "y2=\"%.2f\" stroke=\"#d1495b\" "
                      "stroke-width=\"2\" stroke-dasharray=\"6 5\" />\n",
                      svg_x(query, min_x, max_x), svg_y(query, min_y, max_y),
                      svg_x(nearest, min_x, max_x),
                      svg_y(nearest, min_y, max_y));
        fprintf(file, "<circle cx=\"%.2f\" cy=\"%.2f\" r=\"6.5\" "
                      "fill=\"#d1495b\" stroke=\"#111\" "
                      "stroke-width=\"1.5\" />\n",
                      svg_x(nearest, min_x, max_x),
                      svg_y(nearest, min_y, max_y));
    }
    fprintf(file, "<circle cx=\"%.2f\" cy=\"%.2f\" r=\"7.0\" "
                  "fill=\"#f4a261\" stroke=\"#111\" "
                  "stroke-width=\"2\" />\n",
                  svg_x(query, min_x, max_x), svg_y(query, min_y, max_y));
    write_nearest_legend(file);
    fprintf(file, "</svg>\n");
    fclose(file);
    return 1;
}

int spatial_svg_write_range(const char *path, const Point2D *points,
                            size_t count, Point2D query,
                            const size_t *range_indices,
                            size_t range_count)
{
    FILE *file;
    double min_x;
    double max_x;
    double min_y;
    double max_y;

    if (path == 0 || points == 0 || range_indices == 0 || count == 0) {
        return 0;
    }

    bounds(points, count, &min_x, &max_x, &min_y, &max_y);
    file = fopen(path, "w");
    if (file == 0) {
        return 0;
    }

    write_start(file, "Range query result");
    for (size_t index = 0; index < count; index++) {
        int selected = contains_index(range_indices, range_count,
                                      points[index].index);
        svg_point(file, points[index], min_x, max_x, min_y, max_y,
                  selected ? "#2a9d8f" : "#bbbbbb", selected ? 5.0 : 3.0);
    }
    fprintf(file, "<circle cx=\"%.2f\" cy=\"%.2f\" r=\"7.0\" "
                  "fill=\"#f4a261\" stroke=\"#111\" "
                  "stroke-width=\"2\" />\n",
                  svg_x(query, min_x, max_x), svg_y(query, min_y, max_y));
    write_range_legend(file);
    fprintf(file, "</svg>\n");
    fclose(file);
    return 1;
}
