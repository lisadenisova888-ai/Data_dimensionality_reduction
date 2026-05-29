#include "svg.h"

#include <stdio.h>

#define SVG_SINGLE_WIDTH 900.0
#define SVG_SINGLE_HEIGHT 680.0
#define SVG_DASHBOARD_WIDTH 1600.0
#define SVG_DASHBOARD_HEIGHT 2050.0
#define SVG_PANEL_WIDTH 690.0
#define SVG_PANEL_HEIGHT 520.0
#define SVG_MARGIN 70.0
#define SVG_POINT_RADIUS 4.0
#define SVG_CARD_RADIUS 18.0

//цвет точки по исходному классу Wine
static const char *class_color(int label)
{
    if (label == 1) {
        return "#e4576b";
    }
    if (label == 2) {
        return "#2878b5";
    }
    if (label == 3) {
        return "#2a9d8f";
    }
    return "#6b7280";
}

//цвет точки по номеру кластера
static const char *cluster_color(int cluster)
{
    static const char *colors[] = {
        "#e4576b", "#2878b5", "#2a9d8f", "#f4a261",
        "#7c3aed", "#ca8a04", "#0891b2", "#db2777"
    };

    if (cluster < 0) {
        return "#9ca3af";
    }
    return colors[(size_t)cluster % (sizeof(colors) / sizeof(colors[0]))];
}

//перевод координаты из диапазона данных в диапазон SVG
static double scale_value(double value, double min_value, double max_value,
                          double start, double end)
{
    if (max_value == min_value) {
        return (start + end) / 2.0;
    }
    return start + (value - min_value) * (end - start) /
        (max_value - min_value);
}

//находит границы PCA-проекции
static void find_bounds(const Matrix *points, double *min_x, double *max_x,
                        double *min_y, double *max_y)
{
    *min_x = points->values[0];
    *max_x = points->values[0];
    *min_y = points->values[1];
    *max_y = points->values[1];

    for (size_t row = 1; row < points->rows; row++) {
        double x = points->values[row * points->cols];
        double y = points->values[row * points->cols + 1];

        if (x < *min_x) {
            *min_x = x;
        }
        if (x > *max_x) {
            *max_x = x;
        }
        if (y < *min_y) {
            *min_y = y;
        }
        if (y > *max_y) {
            *max_y = y;
        }
    }
}

//добавляет небольшой отступ к границам, чтобы точки не прилипали к осям
static void pad_bounds(double *min_x, double *max_x, double *min_y,
                       double *max_y)
{
    double width = *max_x - *min_x;
    double height = *max_y - *min_y;

    if (width == 0.0) {
        width = 1.0;
    }
    if (height == 0.0) {
        height = 1.0;
    }
    *min_x -= width * 0.08;
    *max_x += width * 0.08;
    *min_y -= height * 0.08;
    *max_y += height * 0.08;
}

//расширяет границы графика с учётом центроидов k-means
static void include_centroid_bounds(const Matrix *centroids, double *min_x,
                                    double *max_x, double *min_y,
                                    double *max_y)
{
    for (size_t row = 0; row < centroids->rows; row++) {
        double x = centroids->values[row * centroids->cols];
        double y = centroids->values[row * centroids->cols + 1];

        if (x < *min_x) {
            *min_x = x;
        }
        if (x > *max_x) {
            *max_x = x;
        }
        if (y < *min_y) {
            *min_y = y;
        }
        if (y > *max_y) {
            *max_y = y;
        }
    }
}

//проверяет, находится ли индекс точки в массиве результатов range query
static int contains_index(const size_t *indices, size_t count, size_t index)
{
    for (size_t current = 0; current < count; current++) {
        if (indices[current] == index) {
            return 1;
        }
    }
    return 0;
}

//записывает начало SVG-файла с общими стилями
static void write_svg_start(FILE *file, double width, double height,
                            const char *title)
{
    fprintf(file, "<svg xmlns=\"http://www.w3.org/2000/svg\" "
                  "width=\"%.0f\" height=\"%.0f\" "
                  "viewBox=\"0 0 %.0f %.0f\">\n",
                  width, height, width, height);
    fprintf(file, "<defs>\n");
    fprintf(file, "<filter id=\"shadow\" x=\"-20%%\" y=\"-20%%\" "
                  "width=\"140%%\" height=\"140%%\">\n");
    fprintf(file, "<feDropShadow dx=\"0\" dy=\"8\" stdDeviation=\"10\" "
                  "flood-color=\"#0f172a\" flood-opacity=\"0.12\" />\n");
    fprintf(file, "</filter>\n");
    fprintf(file, "<style>\n");
    fprintf(file, "text{font-family:Arial,Helvetica,sans-serif;}\n");
    fprintf(file, ".title{font-size:34px;font-weight:700;fill:#111827;}\n");
    fprintf(file, ".subtitle{font-size:16px;fill:#6b7280;}\n");
    fprintf(file, ".panel-title{font-size:20px;font-weight:700;fill:#111827;}\n");
    fprintf(file, ".panel-note{font-size:13px;fill:#6b7280;}\n");
    fprintf(file, ".axis{stroke:#374151;stroke-width:1.4;}\n");
    fprintf(file, ".grid{stroke:#e5e7eb;stroke-width:1;}\n");
    fprintf(file, ".label{font-size:12px;fill:#4b5563;}\n");
    fprintf(file, ".legend{font-size:12px;fill:#374151;}\n");
    fprintf(file, "</style>\n");
    fprintf(file, "</defs>\n");
    fprintf(file, "<rect width=\"100%%\" height=\"100%%\" fill=\"#f3f4f6\" />\n");
    fprintf(file, "<text x=\"70\" y=\"58\" class=\"title\">%s</text>\n", title);
    fprintf(file, "<text x=\"70\" y=\"88\" class=\"subtitle\">Wine dataset: PCA projection, clustering and spatial queries</text>\n");
}

//рисует карточку панели
static void write_panel_card(FILE *file, double x, double y, double width,
                             double height, const char *title,
                             const char *note)
{
    fprintf(file, "<rect x=\"%.1f\" y=\"%.1f\" width=\"%.1f\" "
                  "height=\"%.1f\" rx=\"%.1f\" fill=\"#ffffff\" "
                  "filter=\"url(#shadow)\" />\n",
                  x, y, width, height, SVG_CARD_RADIUS);
    fprintf(file, "<text x=\"%.1f\" y=\"%.1f\" class=\"panel-title\">%s</text>\n",
                  x + 26.0, y + 38.0, title);
    fprintf(file, "<text x=\"%.1f\" y=\"%.1f\" class=\"panel-note\">%s</text>\n",
                  x + 26.0, y + 60.0, note);
}

//рисует оси, сетку и подписи PC1/PC2 внутри панели
static void write_axes(FILE *file, double left, double top, double width,
                       double height)
{
    double right = left + width;
    double bottom = top + height;

    for (int step = 1; step < 4; step++) {
        double x = left + width * (double)step / 4.0;
        double y = top + height * (double)step / 4.0;

        fprintf(file, "<line x1=\"%.1f\" y1=\"%.1f\" x2=\"%.1f\" "
                      "y2=\"%.1f\" class=\"grid\" />\n",
                      x, top, x, bottom);
        fprintf(file, "<line x1=\"%.1f\" y1=\"%.1f\" x2=\"%.1f\" "
                      "y2=\"%.1f\" class=\"grid\" />\n",
                      left, y, right, y);
    }
    fprintf(file, "<line x1=\"%.1f\" y1=\"%.1f\" x2=\"%.1f\" "
                  "y2=\"%.1f\" class=\"axis\" />\n",
                  left, bottom, right, bottom);
    fprintf(file, "<line x1=\"%.1f\" y1=\"%.1f\" x2=\"%.1f\" "
                  "y2=\"%.1f\" class=\"axis\" />\n",
                  left, top, left, bottom);
    fprintf(file, "<text x=\"%.1f\" y=\"%.1f\" class=\"label\">PC1</text>\n",
                  right - 22.0, bottom + 28.0);
    fprintf(file, "<text x=\"%.1f\" y=\"%.1f\" class=\"label\">PC2</text>\n",
                  left - 42.0, top + 12.0);
}

//переводит координату X точки в координату SVG
static double point_x(const Matrix *points, size_t row, double min_x,
                      double max_x, double left, double width)
{
    return scale_value(points->values[row * points->cols], min_x, max_x,
                       left, left + width);
}

//переводит координату Y точки в координату SVG
static double point_y(const Matrix *points, size_t row, double min_y,
                      double max_y, double top, double height)
{
    return scale_value(points->values[row * points->cols + 1], min_y, max_y,
                       top + height, top);
}

//рисует один круг-точку с title-подсказкой
static void write_point(FILE *file, double x, double y, double radius,
                        const char *color, const char *title)
{
    fprintf(file, "<circle cx=\"%.2f\" cy=\"%.2f\" r=\"%.1f\" "
                  "fill=\"%s\" opacity=\"0.84\">",
                  x, y, radius, color);
    fprintf(file, "<title>%s</title></circle>\n", title);
}

//рисует элемент легенды
static void write_legend_item(FILE *file, double x, double y,
                              const char *color, const char *text,
                              int square)
{
    if (square) {
        fprintf(file, "<rect x=\"%.1f\" y=\"%.1f\" width=\"11\" "
                      "height=\"11\" fill=\"%s\" stroke=\"#111827\" "
                      "stroke-width=\"1.5\" />\n", x, y - 9.0, color);
    } else {
        fprintf(file, "<circle cx=\"%.1f\" cy=\"%.1f\" r=\"5\" "
                      "fill=\"%s\" />\n", x + 5.5, y - 4.0, color);
    }
    fprintf(file, "<text x=\"%.1f\" y=\"%.1f\" class=\"legend\">%s</text>\n",
                  x + 20.0, y, text);
}

//рисует PCA-панель с исходными классами
static void write_pca_panel(FILE *file, const Matrix *points,
                            const int *labels, double card_x,
                            double card_y, double min_x, double max_x,
                            double min_y, double max_y)
{
    double plot_x = card_x + 62.0;
    double plot_y = card_y + 92.0;
    double plot_w = SVG_PANEL_WIDTH - 118.0;
    double plot_h = SVG_PANEL_HEIGHT - 170.0;

    write_panel_card(file, card_x, card_y, SVG_PANEL_WIDTH, SVG_PANEL_HEIGHT,
                     "1. PCA projection",
                     "Points are colored by the original Wine class");
    write_axes(file, plot_x, plot_y, plot_w, plot_h);

    for (size_t row = 0; row < points->rows; row++) {
        char title[80];
        snprintf(title, sizeof(title), "Wine %lu, class %d",
                 (unsigned long)(row + 1), labels[row]);
        write_point(file, point_x(points, row, min_x, max_x, plot_x, plot_w),
                    point_y(points, row, min_y, max_y, plot_y, plot_h),
                    SVG_POINT_RADIUS, class_color(labels[row]), title);
    }

    write_legend_item(file, card_x + 76.0, card_y + SVG_PANEL_HEIGHT - 42.0,
                      class_color(1), "Class 1", 0);
    write_legend_item(file, card_x + 176.0, card_y + SVG_PANEL_HEIGHT - 42.0,
                      class_color(2), "Class 2", 0);
    write_legend_item(file, card_x + 276.0, card_y + SVG_PANEL_HEIGHT - 42.0,
                      class_color(3), "Class 3", 0);
}

//рисует k-means-панель
static void write_kmeans_panel(FILE *file, const Matrix *points,
                               const int *clusters,
                               const Matrix *centroids,
                               size_t cluster_count, double card_x,
                               double card_y, double min_x, double max_x,
                               double min_y, double max_y)
{
    double plot_x = card_x + 62.0;
    double plot_y = card_y + 92.0;
    double plot_w = SVG_PANEL_WIDTH - 118.0;
    double plot_h = SVG_PANEL_HEIGHT - 170.0;

    write_panel_card(file, card_x, card_y, SVG_PANEL_WIDTH, SVG_PANEL_HEIGHT,
                     "2. K-means clusters",
                     "Circles are points, squares are centroids");
    write_axes(file, plot_x, plot_y, plot_w, plot_h);

    for (size_t row = 0; row < points->rows; row++) {
        char title[80];
        snprintf(title, sizeof(title), "Wine %lu, cluster %d",
                 (unsigned long)(row + 1), clusters[row] + 1);
        write_point(file, point_x(points, row, min_x, max_x, plot_x, plot_w),
                    point_y(points, row, min_y, max_y, plot_y, plot_h),
                    SVG_POINT_RADIUS, cluster_color(clusters[row]), title);
    }

    for (size_t cluster = 0; cluster < centroids->rows; cluster++) {
        double x = scale_value(centroids->values[cluster * centroids->cols],
                               min_x, max_x, plot_x, plot_x + plot_w);
        double y = scale_value(centroids->values[cluster * centroids->cols + 1],
                               min_y, max_y, plot_y + plot_h, plot_y);
        fprintf(file, "<rect x=\"%.2f\" y=\"%.2f\" width=\"15\" "
                      "height=\"15\" fill=\"%s\" stroke=\"#111827\" "
                      "stroke-width=\"2\"><title>Centroid %lu</title></rect>\n",
                      x - 7.5, y - 7.5, cluster_color((int)cluster),
                      (unsigned long)(cluster + 1));
    }

    for (size_t cluster = 0; cluster < cluster_count; cluster++) {
        char text[32];
        snprintf(text, sizeof(text), "Cluster %lu",
                 (unsigned long)(cluster + 1));
        write_legend_item(file, card_x + 76.0 + 112.0 * (double)cluster,
                          card_y + SVG_PANEL_HEIGHT - 42.0,
                          cluster_color((int)cluster), text, 0);
    }
    write_legend_item(file, card_x + 430.0, card_y + SVG_PANEL_HEIGHT - 42.0,
                      "#ffffff", "Centroid", 1);
}

//рисует DBSCAN-панель
static void write_dbscan_panel(FILE *file, const Matrix *points,
                               const DBSCANResult *dbscan, double card_x,
                               double card_y, double min_x, double max_x,
                               double min_y, double max_y)
{
    double plot_x = card_x + 62.0;
    double plot_y = card_y + 92.0;
    double plot_w = SVG_PANEL_WIDTH - 118.0;
    double plot_h = SVG_PANEL_HEIGHT - 170.0;

    write_panel_card(file, card_x, card_y, SVG_PANEL_WIDTH, SVG_PANEL_HEIGHT,
                     "3. DBSCAN clusters",
                     "Noise is shown in gray, dense groups are colored");
    write_axes(file, plot_x, plot_y, plot_w, plot_h);

    for (size_t row = 0; row < points->rows; row++) {
        char title[90];
        const char *color = cluster_color(dbscan->labels[row]);

        if (dbscan->labels[row] < 0) {
            snprintf(title, sizeof(title), "Wine %lu, noise",
                     (unsigned long)(row + 1));
        } else {
            snprintf(title, sizeof(title), "Wine %lu, DBSCAN cluster %d",
                     (unsigned long)(row + 1), dbscan->labels[row] + 1);
        }
        write_point(file, point_x(points, row, min_x, max_x, plot_x, plot_w),
                    point_y(points, row, min_y, max_y, plot_y, plot_h),
                    SVG_POINT_RADIUS, color, title);
    }

    for (size_t cluster = 0; cluster < dbscan->cluster_count && cluster < 4;
         cluster++) {
        char text[32];
        snprintf(text, sizeof(text), "Cluster %lu",
                 (unsigned long)(cluster + 1));
        write_legend_item(file, card_x + 76.0 + 112.0 * (double)cluster,
                          card_y + SVG_PANEL_HEIGHT - 42.0,
                          cluster_color((int)cluster), text, 0);
    }
    write_legend_item(file, card_x + 520.0, card_y + SVG_PANEL_HEIGHT - 42.0,
                      cluster_color(-1), "Noise", 0);
}

//рисует панель поиска ближайшего соседа
static void write_nearest_panel(FILE *file, const Matrix *points,
                                size_t query_index, size_t nearest_index,
                                double card_x, double card_y,
                                double min_x, double max_x, double min_y,
                                double max_y)
{
    double plot_x = card_x + 62.0;
    double plot_y = card_y + 92.0;
    double plot_w = SVG_PANEL_WIDTH - 118.0;
    double plot_h = SVG_PANEL_HEIGHT - 170.0;
    double query_x;
    double query_y;
    double nearest_x;
    double nearest_y;

    write_panel_card(file, card_x, card_y, SVG_PANEL_WIDTH, SVG_PANEL_HEIGHT,
                     "4. Nearest neighbor query",
                     "Orange point is the query, red point is the answer");
    write_axes(file, plot_x, plot_y, plot_w, plot_h);

    for (size_t row = 0; row < points->rows; row++) {
        const char *color = "#9ca3af";
        double radius = 3.2;
        char title[80];

        if (row == nearest_index) {
            color = "#e4576b";
            radius = 6.0;
        }
        if (row == query_index) {
            color = "#f4a261";
            radius = 7.0;
        }
        snprintf(title, sizeof(title), "Wine %lu", (unsigned long)(row + 1));
        write_point(file, point_x(points, row, min_x, max_x, plot_x, plot_w),
                    point_y(points, row, min_y, max_y, plot_y, plot_h),
                    radius, color, title);
    }

    query_x = point_x(points, query_index, min_x, max_x, plot_x, plot_w);
    query_y = point_y(points, query_index, min_y, max_y, plot_y, plot_h);
    nearest_x = point_x(points, nearest_index, min_x, max_x, plot_x, plot_w);
    nearest_y = point_y(points, nearest_index, min_y, max_y, plot_y, plot_h);
    fprintf(file, "<line x1=\"%.2f\" y1=\"%.2f\" x2=\"%.2f\" "
                  "y2=\"%.2f\" stroke=\"#e4576b\" stroke-width=\"2.4\" "
                  "stroke-dasharray=\"7 6\" />\n",
                  query_x, query_y, nearest_x, nearest_y);
    write_legend_item(file, card_x + 76.0, card_y + SVG_PANEL_HEIGHT - 42.0,
                      "#f4a261", "Query", 0);
    write_legend_item(file, card_x + 176.0, card_y + SVG_PANEL_HEIGHT - 42.0,
                      "#e4576b", "Nearest", 0);
    write_legend_item(file, card_x + 300.0, card_y + SVG_PANEL_HEIGHT - 42.0,
                      "#9ca3af", "Other points", 0);
}

//рисует панель range query
static void write_range_panel(FILE *file, const Matrix *points,
                              size_t query_index,
                              const size_t *range_indices,
                              size_t range_count, double range_radius,
                              double card_x, double card_y,
                              double min_x, double max_x, double min_y,
                              double max_y)
{
    double plot_x = card_x + 62.0;
    double plot_y = card_y + 92.0;
    double plot_w = SVG_PANEL_WIDTH - 118.0;
    double plot_h = SVG_PANEL_HEIGHT - 170.0;
    char note[100];

    snprintf(note, sizeof(note), "Green points are inside radius %.2f",
             range_radius);
    write_panel_card(file, card_x, card_y, SVG_PANEL_WIDTH, SVG_PANEL_HEIGHT,
                     "5. Range query", note);
    write_axes(file, plot_x, plot_y, plot_w, plot_h);

    for (size_t row = 0; row < points->rows; row++) {
        int selected = contains_index(range_indices, range_count, row);
        const char *color = selected ? "#2a9d8f" : "#cbd5e1";
        double radius = selected ? 5.0 : 3.0;
        char title[80];

        if (row == query_index) {
            color = "#f4a261";
            radius = 7.0;
        }
        snprintf(title, sizeof(title), "Wine %lu", (unsigned long)(row + 1));
        write_point(file, point_x(points, row, min_x, max_x, plot_x, plot_w),
                    point_y(points, row, min_y, max_y, plot_y, plot_h),
                    radius, color, title);
    }

    write_legend_item(file, card_x + 76.0, card_y + SVG_PANEL_HEIGHT - 42.0,
                      "#f4a261", "Query", 0);
    write_legend_item(file, card_x + 176.0, card_y + SVG_PANEL_HEIGHT - 42.0,
                      "#2a9d8f", "Inside range", 0);
    write_legend_item(file, card_x + 330.0, card_y + SVG_PANEL_HEIGHT - 42.0,
                      "#cbd5e1", "Outside", 0);
}

//сохраняет PCA-проекцию отдельным SVG-файлом
int svg_write_pca_projection(const char *path, const Matrix *points,
                             const int *labels)
{
    FILE *file;
    double min_x;
    double max_x;
    double min_y;
    double max_y;

    if (path == NULL || !matrix_is_valid(points) || points->cols < 2
        || labels == NULL) {
        return 0;
    }

    find_bounds(points, &min_x, &max_x, &min_y, &max_y);
    pad_bounds(&min_x, &max_x, &min_y, &max_y);

    file = fopen(path, "w");
    if (file == NULL) {
        return 0;
    }

    write_svg_start(file, SVG_SINGLE_WIDTH, SVG_SINGLE_HEIGHT,
                    "Wine PCA projection");
    write_pca_panel(file, points, labels, 105.0, 120.0, min_x, max_x,
                    min_y, max_y);
    fprintf(file, "</svg>\n");
    fclose(file);
    return 1;
}

//сохраняет k-means-проекцию отдельным SVG-файлом
int svg_write_kmeans_projection(const char *path, const Matrix *points,
                                const int *clusters, const Matrix *centroids,
                                size_t cluster_count)
{
    FILE *file;
    double min_x;
    double max_x;
    double min_y;
    double max_y;

    if (path == NULL || !matrix_is_valid(points) || points->cols < 2
        || clusters == NULL || !matrix_is_valid(centroids)
        || centroids->cols < 2 || cluster_count == 0) {
        return 0;
    }

    find_bounds(points, &min_x, &max_x, &min_y, &max_y);
    include_centroid_bounds(centroids, &min_x, &max_x, &min_y, &max_y);
    pad_bounds(&min_x, &max_x, &min_y, &max_y);

    file = fopen(path, "w");
    if (file == NULL) {
        return 0;
    }

    write_svg_start(file, SVG_SINGLE_WIDTH, SVG_SINGLE_HEIGHT,
                    "Wine k-means clusters");
    write_kmeans_panel(file, points, clusters, centroids, cluster_count,
                       105.0, 120.0, min_x, max_x, min_y, max_y);
    fprintf(file, "</svg>\n");
    fclose(file);
    return 1;
}

//сохраняет DBSCAN-проекцию отдельным SVG-файлом
int svg_write_dbscan_projection(const char *path, const Matrix *points,
                                const DBSCANResult *dbscan)
{
    FILE *file;
    double min_x;
    double max_x;
    double min_y;
    double max_y;

    if (path == NULL || !matrix_is_valid(points) || points->cols < 2
        || dbscan == NULL || dbscan->labels == NULL) {
        return 0;
    }

    find_bounds(points, &min_x, &max_x, &min_y, &max_y);
    pad_bounds(&min_x, &max_x, &min_y, &max_y);

    file = fopen(path, "w");
    if (file == NULL) {
        return 0;
    }

    write_svg_start(file, SVG_SINGLE_WIDTH, SVG_SINGLE_HEIGHT,
                    "Wine DBSCAN clusters");
    write_dbscan_panel(file, points, dbscan, 105.0, 120.0, min_x, max_x,
                       min_y, max_y);
    fprintf(file, "</svg>\n");
    fclose(file);
    return 1;
}

//сохраняет все графики проекта на одном SVG-листе
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
                                double range_radius)
{
    FILE *file;
    double min_x;
    double max_x;
    double min_y;
    double max_y;

    if (path == NULL || !matrix_is_valid(points) || points->cols < 2
        || class_labels == NULL || kmeans_labels == NULL
        || !matrix_is_valid(kmeans_centroids) || dbscan == NULL
        || dbscan->labels == NULL || range_indices == NULL
        || query_index >= points->rows || nearest_index >= points->rows) {
        return 0;
    }

    find_bounds(points, &min_x, &max_x, &min_y, &max_y);
    include_centroid_bounds(kmeans_centroids, &min_x, &max_x, &min_y, &max_y);
    pad_bounds(&min_x, &max_x, &min_y, &max_y);

    file = fopen(path, "w");
    if (file == NULL) {
        return 0;
    }

    write_svg_start(file, SVG_DASHBOARD_WIDTH, SVG_DASHBOARD_HEIGHT,
                    "Data dimensionality reduction dashboard");
    write_pca_panel(file, points, class_labels, 70.0, 130.0, min_x, max_x,
                    min_y, max_y);
    write_kmeans_panel(file, points, kmeans_labels, kmeans_centroids,
                       kmeans_cluster_count, 840.0, 130.0,
                       min_x, max_x, min_y, max_y);
    write_dbscan_panel(file, points, dbscan, 70.0, 720.0,
                       min_x, max_x, min_y, max_y);
    write_nearest_panel(file, points, query_index, nearest_index,
                        840.0, 720.0, min_x, max_x, min_y, max_y);
    write_range_panel(file, points, query_index, range_indices, range_count,
                      range_radius, 455.0, 1310.0,
                      min_x, max_x, min_y, max_y);

    fprintf(file, "<text x=\"70\" y=\"1940\" class=\"subtitle\">"
                  "All panels use the same PCA coordinate bounds, so the positions are directly comparable."
                  "</text>\n");
    fprintf(file, "</svg>\n");
    fclose(file);
    return 1;
}
