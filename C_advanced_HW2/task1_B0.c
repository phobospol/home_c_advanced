#include <stdint.h>
#include <stddef.h>

/*
Максимальный блок
Описана структура данных
typedef struct list {
uint64_t address;
size_t size;
char comment[64];
struct list *next;
} list;
Требуется реализовать только одну функцию, которая в данном списке находит адрес блока памяти занимающий больше всего места.
Адрес хранится в поле address, поле size - соответствующий размер данного блока. Если список пустой, то функция должна возвращать 0. Если есть несколько таких блоков, то вернуть адрес любого из них.
Прототип функции:
uint64_t findMaxBlock(list *head)
 */

typedef struct list
{
    uint64_t address;
    size_t size;
    char comment[64];
    struct list *next;
} list;

// findMaxBlock функция нахождения адреса блока памяти занимающего больше всего места
uint64_t findMaxBlock(list *head)
{
    if (head == NULL) // Защита от пустого списка
    {
        return 0;
    }

    list *current = head; // Объявляется указатель current, который будет "бегать" по списку
    list *maxNode = head; // Указатель maxNode, который хранит адрес блока с самым большим размером

    // Цикл обхода списка пока не встретится NULL - конец списка
    while (current != NULL)
    {
        if (current->size > maxNode->size) // Условие сравнения размеров блоков
        {
            maxNode = current; // Обновление максимума
        }
        current = current->next; // Переход к следующему блоку в списке
    }

    return maxNode->address; // По окончании цикла возвращаем поле address структуры — это и есть адрес самого большого блока
}



