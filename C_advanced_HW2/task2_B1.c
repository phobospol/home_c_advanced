#include <stddef.h>

/*
Всего памяти
Описана структура данных для хранения информации об использованной памяти:
typedef struct list {
void *address;
size_t size;
char comment[64];
struct list *next;
} list;
Требуется реализовать только одну функцию, которая анализирует данный список и возвращает сколько всего памяти используется. Адрес хранится в поле address, поле size - соответствующий размер данного блока. Если список пустой, то функция должна возвращать 0.
Прототип функции:
size_t totalMemoryUsage(list *head)
 */

typedef struct list
{
    void *address;
    size_t size;
    char comment[64];
    struct list *next;
} list;

// totalMemoryUsage функция подсчета суммарного объема памяти всех элементов списка
size_t totalMemoryUsage(list *head)
{
    size_t total = 0; // Создаём переменную-накопитель total
    list *current = head; // Создаём указатель current, который будет «ходить» по списку

    // Проходим в цикле по всему списку
    while (current != NULL)
    {
        total += current->size;  // Добавляем размер текущего блока
        current = current->next; // Переходим к следующему элементу списка (блоку памяти)
    }

    return total; // Возвращаем накопленную сумму памяти всех элементов списка
}
