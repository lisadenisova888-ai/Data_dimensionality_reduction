#include "dataset.h"

#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define DATASET_LINE_SIZE 4096
#define WINE_FEATURE_COUNT 13

//Проверка на отсутствие символов после 13 признака в строке 
static int only_spaces_left(const char *cursor)
{
    while (*cursor != '\0') {
        if (!isspace((unsigned char)*cursor)) {
            return 0;
        }
        cursor++;
    }
    return 1;
}

//сдвигаем позицию до нахождение разделителя - запятой
static int read_comma(char **cursor)
{
    while (isspace((unsigned char)**cursor)) {
        (*cursor)++;
    }
    if (**cursor != ',') {
        return 0;
    }
    (*cursor)++;
    return 1;
}
//запись одной строки
static int parse_wine_row(char *line, int *label, double *features)
{
    char *cursor = line; // присваиваем начало строки
    char *end; // место окончания числа
    int parsed_label; // метка класса

    if (*cursor < '1' || *cursor > '3' || cursor[1] != ',') {
        return 0;
    }
    parsed_label = *cursor - '0'; // превращаем символ класса в целое число
    cursor += 2; // пропускаем цифру класса и запятую

    for (size_t column = 0; column<WINE_FEATURE_COUNT; column++) {
     
        features[column] = strtod(cursor, &end); //считываем double и в end записываем адрес конца числа
        if (cursor == end|| !isfinite(features[column])) {
            return 0;
        }
        cursor = end;
        if (column + 1 < WINE_FEATURE_COUNT && !read_comma(&cursor)) { 
            return 0;
        }
    }

    if (!only_spaces_left(cursor)) {
        return 0;
    }
    *label = parsed_label;
    return 1;
}
//считаем число строк матрицы
static int count_rows(FILE *input, size_t *rows)
{
    char line[DATASET_LINE_SIZE]; 

    *rows = 0;
    while (fgets(line, sizeof(line), input) != NULL) {
        (*rows)++;
    }
    return *rows > 0;
}
//Считываем датасет
int dataset_load_wine(const char *path, Dataset *dataset) //path - путь к файлу, в массив структур dataset записываем полученную матрицу
{
    FILE *input;
    char line[DATASET_LINE_SIZE]; // будет хранить строку файла
    double features[WINE_FEATURE_COUNT]; //13 параметров с каждой строки

    Dataset loaded = {{0, 0, NULL}, NULL}; //результат

    if (path == NULL || dataset == NULL) { //проверяем входные данные
        return 0;
    }

    input = fopen(path, "r");
    if (input == NULL) {
        return 0;
    }


    size_t rows;
    if (!count_rows(input, &rows)) { //считываем строки матрицы и если их 0 завершаем работу
        fclose(input);
        return 0;
    }

    loaded.features = matrix_create(rows, WINE_FEATURE_COUNT); // создаём матрицу подходящего размера
    loaded.labels = malloc(rows * sizeof(int)); //выделяем место под классы
    if (!matrix_is_valid(&loaded.features) || loaded.labels == NULL) { //проверяем полученную матрицу
        fclose(input);
        dataset_destroy(&loaded);
        return 0;
    }

    rewind(input); //возвращаем файловый указатель в начало
    for (size_t row = 0; row<rows; row++) {
        if (fgets(line, sizeof(line), input) == NULL
            || !parse_wine_row(line, &loaded.labels[row], features)) { //записываем классы и строку в features
            fclose(input);
            dataset_destroy(&loaded); 
            return 0;
        }
        for (size_t column = 0; column < WINE_FEATURE_COUNT; column++) {
            matrix_set(&loaded.features, row, column, features[column]); // записываем в текущую строку матрицы  
        }                                                                // поштучно каждый из 13 параметров
    }
    fclose(input);

    *dataset = loaded; //присваем dataset значение структуры loaded
    return 1;
}
//устроой дестрой порядок это отстой круши ломай тряси башкою пустой
void dataset_destroy(Dataset *dataset)
{
    if (dataset == NULL) {
        return;
    }
    matrix_destroy(&dataset->features);
    free(dataset->labels);
    dataset->labels = NULL;
}
//Делает матрицу пропорциональной
int dataset_standardize(Matrix *dataset)
{

    if (!matrix_is_valid(dataset) || dataset->rows < 2) {
        return 0;
    }

    for (size_t column = 0; column < dataset->cols; column++) {//стандартизируем по колонкам
        double mean = 0.0; //среднее значение
        double variance = 0.0; //дисперсия(средний квадрат отклонения каждого значения от среднего)
        double standard_deviation;//стандартное отклонение

        for (size_t row = 0; row < dataset->rows; row++) {
            mean += dataset->values[row * dataset->cols + column];
        }
        mean /= (double)dataset->rows;

        for (size_t row = 0; row < dataset->rows; row++) {
            double difference =
                dataset->values[row * dataset->cols + column] - mean;//значение в наборе - среднее
            variance += difference * difference;
        }
        variance /= (double)dataset->rows;
        standard_deviation = sqrt(variance);

        for (size_t row = 0; row < dataset->rows; row++) {
            double *value =
                &dataset->values[row * dataset->cols + column];
            *value = (standard_deviation == 0.0)
                ? 0.0 : (*value - mean) / standard_deviation;
        }
    }
    return 1;
}
