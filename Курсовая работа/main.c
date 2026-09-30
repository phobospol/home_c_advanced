/* 
 * Файл: main.c
 * Назначение: Точка входа в программу. Управляет логикой работы:
 *             - Обрабатывает аргументы командной строки
 *             - Запускает тесты (если запрошены)
 *             - Вычисляет площадь фигуры, ограниченной тремя кривыми
 *             - Выводит результаты на английском языке
 */

/* 
 * Макрос _USE_MATH_DEFINES должен быть определен ДО подключения <math.h>,
 * чтобы сделать доступными математические константы (например, M_PI)
 * в строгом стандарте C99.
 */
#define _USE_MATH_DEFINES

/* Подключение стандартной библиотеки ввода-вывода (printf, fprintf) */
#include <stdio.h>

/* Подключение стандартной библиотеки общего назначения (malloc, free, exit) */
#include <stdlib.h>

/* Подключение библиотеки для работы со строками (strcmp) */
#include <string.h>

/* Подключение математической библиотеки (sin, cos, fabs, M_PI и др.) */
#include <math.h>

/* Подключение библиотеки для работы с локалями (setlocale) */
#include <locale.h>

/* Подключение пользовательских заголовков с объявлениями функций */
#include "functions.h"   /* Объявления функций f1, f2, f3 и их производных */
#include "root.h"        /* Объявление функции root() для поиска корней */
#include "integral.h"    /* Объявление функции integral() для вычисления интегралов */

/* ====================== НАСТРОЙКИ И ПЕРЕМЕННЫЕ ====================== */

/* Точность (epsilon) для поиска корней уравнений (по умолчанию 10^-6) */
static double eps1 = 1e-6;

/* Точность для вычисления интегралов (по умолчанию 10^-6) */
static double eps2 = 1e-6;

/* Флаги управления выводом и режимами работы (изначально выключены) */
static int print_roots       = 0; /* Флаг: печатать ли координаты точек пересечения */
static int print_iterations  = 0; /* Флаг: печатать ли количество итераций */
static int test_root_flag    = 0; /* Флаг: запускать ли тест функции поиска корня */
static int test_integral_flag= 0; /* Флаг: запускать ли тест функции интегрирования */

/* ====================== ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ ====================== */

/* 
 * Вспомогательная функция для вычисления разности f1(x) - f3(x).
 * Нужна для нахождения площади между первой и третьей кривой.
 * Возвращает: значение разности функций в точке x.
 */
static double area_f1_minus_f3(double x) { return f1(x) - f3(x); }

/* 
 * Вспомогательная функция для вычисления разности f1(x) - f2(x).
 * Нужна для нахождения площади между первой и второй кривой.
 * Возвращает: значение разности функций в точке x.
 */
static double area_f1_minus_f2(double x) { return f1(x) - f2(x); }

/* ====================== ФУНКЦИИ ТЕСТИРОВАНИЯ ====================== */

/* 
 * Тестовая функция: f(x) = x^2. Используется для проверки root().
 * Ожидается, что корень уравнения x^2 = 4 будет равен 2.
 */
static double test_f_sqrt(double x)  { (void)x; return x * x; }

/* 
 * Тестовая функция: g(x) = 4. Используется как правая часть уравнения.
 * Уравнение: x^2 = 4.
 */
static double test_g_sqrt(double x)  { (void)x; return 4.0; }

/* 
 * Производная тестовой функции f(x) = x^2 -> f'(x) = 2x.
 * Необходима для методов поиска корня, использующих производные.
 */
static double test_df_sqrt(double x)  { return 2.0 * x; }

/* 
 * Производная тестовой функции g(x) = 4 -> g'(x) = 0.
 */
static double test_dg_sqrt(double x) { (void)x; return 0.0; }

/* 
 * Тестовая функция: sin(x). Используется для проверки integral().
 * Интеграл от sin(x) на отрезке [0, PI] должен быть равен 2.
 */
static double test_sin(double x) { return sin(x); }

/* 
 * Функция запуска теста поиска корня.
 * Логика: решает уравнение x^2 = 4 на интервале [0, 5] и сравнивает результат с 2.0.
 */
static void test_root(void)
{
    /* Вывод заголовка теста */
    printf("=== Root Function Test ===\n");
    /* Описание тестируемого уравнения */
    printf("Equation: x^2 = 4 on interval [0, 5]\n");
    /* Ожидаемый правильный ответ */
    printf("Expected root: 2.0\n");
    
    /* Вызов функции поиска корня. 
       Параметры: функции, интервал, точность, производные.
       Результат сохраняется в переменную r. */
    double r = root(test_f_sqrt, test_g_sqrt, 0.0, 5.0, 1e-8,
                    test_df_sqrt, test_dg_sqrt);
    
    /* Вывод полученного результата с высокой точностью */
    printf("  Result:  %.10f\n", r);
    /* Вывод количества итераций, затраченных на поиск (глобальная переменная из root.c) */
    printf("  Iterations: %d\n", root_iterations);
    /* Вычисление и вывод ошибки (разницы между результатом и эталоном) */
    printf("  Error: %.2e\n\n", fabs(r - 2.0));
}

/* 
 * Функция запуска теста вычисления интеграла.
 * Логика: проверяет интеграл от sin(x) и от x^2.
 */
static void test_integral(void)
{
    /* Вывод заголовка теста */
    printf("=== Integral Function Test ===\n");
    
    /* --- Тест 1: Интеграл от sin(x) на [0, PI] --- */
    printf("Test 1: Integral of sin(x) on [0, PI]\n");
    printf("  Expected result: 2.0\n");
    
    /* Вычисление интеграла. M_PI - это число Пи. */
    double I = integral(test_sin, 0.0, M_PI, 1e-8);
    
    /* Вывод результата и ошибки */
    printf("  Result:  %.10f\n", I);
    printf("  Error: %.2e\n\n", fabs(I - 2.0));

    /* --- Тест 2: Интеграл от x^2 на [0, 1] --- */
    printf("Test 2: Integral of x^2 on [0, 1]\n");
    printf("  Expected result: 0.3333333333...\n");
    
    /* Повторное использование переменной I для нового расчета */
    I = integral(test_f_sqrt, 0.0, 1.0, 1e-8);
    
    /* Вывод результата и ошибки (1/3 - ожидаемое значение) */
    printf("  Result:  %.10f\n", I);
    printf("  Error: %.2e\n\n", fabs(I - 1.0 / 3.0));
}

/* ====================== ОСНОВНАЯ ЛОГИКА ВЫЧИСЛЕНИЙ ====================== */

/* 
 * Основная функция вычисления площади фигуры, ограниченной тремя кривыми.
 * Алгоритм:
 * 1. Найти точки пересечения кривых (корни уравнений f_i = f_j).
 * 2. Проинтегрировать разности функций на полученных отрезках.
 * 3. Сложить площади.
 */
static void compute_area(void)
{
    /* --- Шаг 1: Поиск точек пересечения --- */
    
    /* Находим точку пересечения f3 и f1 на интервале [0.5, 1.0] */
    double x1 = root(f3, f1, 0.5, 1.0, eps1, df3, df1);
    /* Сохраняем количество итераций для возможного вывода */
    int it1 = root_iterations;

    /* Находим точку пересечения f3 и f2 на интервале [3.0, 3.5] */
    double x2 = root(f3, f2, 3.0, 3.5, eps1, df3, df2);
    int it2 = root_iterations;

    /* Находим точку пересечения f2 и f1 на интервале [3.5, 4.5] */
    double x3 = root(f2, f1, 3.5, 4.5, eps1, df2, df1);
    int it3 = root_iterations;


 /* Описание задачи программы */
    printf("=== This program calculates the area bounded by the curves ===\n");
    printf("  f1(x) = 0.6x + 3\n");
    printf("  f2(x) = (x - 2)^3 - 1\n");
    printf("  f3(x) = 3 / x\n\n");


    /* Вывод найденных координат точек пересечения */
    printf("=== Curve Intersection Points ===\n");
    printf(" Intersaction f1 and f3:  x = %.10f  y = (f1=%.6f, f3=%.6f)\n", x1, f1(x1), f3(x1));
    printf(" Intersaction f2 and f3:  x = %.10f  y = (f2=%.6f, f3=%.6f)\n", x2, f2(x2), f3(x2));
    printf(" Intersaction f1 and f2:  x = %.10f  y = (f1=%.6f, f2=%.6f)\n", x3, f1(x3), f2(x3));

    /* Если пользователь запросил вывод количества итераций, показываем их */
    if (print_iterations) {
        printf("\n=== Root Finding Iterations ===\n");
        printf(" Intersaction f1 and f3:  %d iterations\n", it1);
        printf(" Intersaction f2 and f3:  %d iterations\n", it2);
        printf(" Intersaction f1 and f2:  %d iterations\n", it3);
    }

    /* --- Шаг 2: Вычисление площадей на отрезках --- */
    
    /* Площадь между f1 и f3 на отрезке [x1, x2] */
    double area1 = integral(area_f1_minus_f3, x1, x2, eps2);
    
    /* Площадь между f1 и f2 на отрезке [x2, x3] */
    double area2 = integral(area_f1_minus_f2, x2, x3, eps2);
    
    /* Общая площадь */
    double total = area1 + area2;

    /* Вывод результатов вычислений */
    printf("\n=== Calculated Area ===\n");
    printf("  Segment 1 [%.6f, %.6f]:  %.10f\n", x1, x2, area1);
    printf("  Segment 2 [%.6f, %.6f]:  %.10f\n", x2, x3, area2);
    printf("  Total Area:              %.10f\n", total);
}

/* ====================== СПРАВКА (HELP) ====================== */

/* 
 * Функция вывода справки по использованию программы.
 * Вызывается при передаче аргумента --help.
 */
static void print_help(void)
{

    /* Описание задачи программы */
    printf("This program calculates the area bounded by the curves:\n");
    printf("  f1(x) = 0.6x + 3\n");
    printf("  f2(x) = (x - 2)^3 - 1\n");
    printf("  f3(x) = 3 / x\n\n");

	 /* Базовый синтаксис запуска */
    printf("Usage: ./program [OPTIONS]...\n\n");
    
    /* Список доступных опций командной строки */
    printf("Command line options:\n");
    printf("  --print-roots        Print x-coordinates of intersection points\n");
    printf("  --print-iterations   Print number of iterations for root finding\n");
    printf("  --test-root          Run unit test for the root() function\n");
    printf("  --test-integral      Run unit test for the integral() function\n");
    printf("  --help               Display this help message\n");
}

/* ====================== ТОЧКА ВХОДА (MAIN) ====================== */

/* 
 * Главная функция программы.
 * Аргументы:
 *   argc - количество аргументов командной строки.
 *   argv - массив строк с самими аргументами.
 */
int main(int argc, char **argv)
{
    /* 
     * Установка локали в значение по умолчанию ("") системы.
     * Это нужно для корректного отображения десятичного разделителя
     * (точка вместо запятой в некоторых локалях) при выводе чисел.
     */
    setlocale(LC_ALL, "");

    /* Цикл обработки аргументов командной строки (начиная с индекса 1, т.к. 0 - имя программы) */
    for (int i = 1; i < argc; i++) 
	{
        /* Проверка на флаг --help */
        if (strcmp(argv[i], "--help") == 0) 
		{
            print_help();
            return 0; /* Завершение программы после вывода справки */
        } 
        /* Проверка на флаг --print-roots */
        else if (strcmp(argv[i], "--print-roots") == 0) 
		{
            print_roots = 1; /* Активируем флаг */
        } 
        /* Проверка на флаг --print-iterations */
        else if (strcmp(argv[i], "--print-iterations") == 0) 
		{
            print_iterations = 1; /* Активируем флаг */
        } 
        /* Проверка на флаг --test-root */
        else if (strcmp(argv[i], "--test-root") == 0) 
		{
            test_root_flag = 1; /* Активируем флаг */
        } 
        /* Проверка на флаг --test-integral */
        else if (strcmp(argv[i], "--test-integral") == 0) 
		{
            test_integral_flag = 1; /* Активируем флаг */
        } 
        /* Если аргумент не распознан */
        else {
            /* Вывод сообщения об ошибке в поток ошибок (stderr) */
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            /* Вывод справки для помощи пользователю */
            print_help();
            return 1; /* Возврат кода ошибки */
        }
    }

    /* Если запрошен тест поиска корня, запускаем его */
    if (test_root_flag) 
	{
        test_root();
    }
    
    /* Если запрошен тест интегрирования, запускаем его */
    if (test_integral_flag) 
	{
        test_integral();
    }

    /* Если тесты НЕ запускались, выполняем основной расчет площади */
    if (!test_root_flag && !test_integral_flag) 
	{
        compute_area();
    }

    /* 
     * Особый случай: если запущены тесты И запрошен вывод промежуточных данных
     * (координаты или итерации), то основной расчет тоже выполняется.
     */
    if ((test_root_flag || test_integral_flag) &&
        (print_roots || print_iterations)) 
	{
        compute_area();
    }

    /* Успешное завершение программы */
    return 0;
}
