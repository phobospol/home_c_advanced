#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

/*
Польская запись
Необходимо вычислить выражение написанное в обратной польской записи. На вход подается строка состоящая из целых, неотрицательных чисел и арифметических символов. В ответ единственное целое число - результат.
*/

#define MAX_LINE 10000 // Максимальная длина всей входной строки
#define MAX_TOKENS 1000 // Максимальное количество токенов
#define MAX_TOKEN_LEN 20 // Максимальная длина одного токена

long stack[MAX_TOKENS]; // Массив для хранения операндов и промежуточных результатов
int sp = 0; // индекс вершины стека это количество элементов сейчас в стеке

// Функции работы со стеком
void push(long v) // Кладёт значение v на вершину стека: сначала записываем по индексу sp, потом увеличиваем sp.
{
    stack[sp++] = v;
}

long pop() // Забирает верхний элемент: сначала уменьшаем sp на 1, потом возвращаем элемент по этому индексу.
{
    if (sp == 0) // Проверка sp == 0 выводит ошибку "недостаточно операндов"
    {
        fprintf(stderr, "Error: insufficient operands\n");
        exit(EXIT_FAILURE);
    }
    return stack[--sp];
}

int main(void)
{
    char line[MAX_LINE]; // Буфер, куда fgets прочитает всю строку целиком
    if (fgets(line, sizeof(line), stdin) == NULL) // Читаем строку из стандартного ввода.
    {
        return 0;  // пустой ввод
    }

    size_t len = strlen(line); // длина прочитанной строки
    if (len > 0 && line[len - 1] == '\n') // Убираем возможный '\n' в конце заменяя на \0
    {
        line[len - 1] = '\0';
        len--;
    }
    // Разбиение строки на токены
    char *token = strtok(line, " \t\r\n"); // Разбивает строку line на токены, используя разделители — пробел, табуляцию, \r, \n
    while (token != NULL) // Цикл по всем токенам. Пока strtok находит следующий токен
    {
        //Проверяем, является ли токен числом:
        if (isdigit((unsigned char)token[0])) //
        {
            long num = strtol(token, NULL, 10); // Если это число — переводим строку в число (strtol, основание 10) и кладём в стек
            push(num);
        }
        // Иначе считаем, что это оператор (+ - * /).
        else
        {
            if (sp < 2) // Перед применением оператора нужно минимум два операнда в стеке. Если их меньше — ошибка "недостаточно операндов для оператора"
            {
                fprintf(stderr, "Error: insufficient operands for the operator %s\n", token);
                return EXIT_FAILURE;
            }
            // Извлекаем два верхних элемента. Сначала b (второй операнд, тот, что был положен позже), потом a (первый операнд)
            long b = pop();
            long a = pop();
            long res = 0;
            // Выполняем операцию в зависимости от символа оператора. Для деления проверяем, не делим ли на ноль.default ловит любые неожиданные символы.
            switch (token[0])
            {
            case '+':
                res = a + b;
                break;
            case '-':
                res = a - b;
                break;
            case '*':
                res = a * b;
                break;
            case '/':
                if (b == 0)
                {
                    fprintf(stderr, "Error: division by zero\n");
                    return EXIT_FAILURE;
                }
                res = a / b;
                break;
            default:
                fprintf(stderr, "Unknown operator: %s\n", token);
                return EXIT_FAILURE;
            }
            push(res); // Результат операции кладём обратно в стек
        }
        token = strtok(NULL, " \t\r\n"); // Следующий вызов strtok(NULL, ...) продолжает разбиение строки и возвращает следующий токен. Когда токены кончаются, вернёт NULL
    }
    // в стеке должен остаться ровно один элемент — результат. Если элементов больше — значит, в выражении лишние числа. Если меньше (0) — значит, не хватило операндов
    if (sp != 1)
    {
        fprintf(stderr, "Error: there is not exactly one result left in the stack\n");
        return EXIT_FAILURE;
    }

    printf("%ld\n", stack[0]); // Выводим единственный оставшийся элемент
    return 0;
}

