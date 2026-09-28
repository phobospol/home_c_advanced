#include <stdio.h>
#include <time.h>
#include <stdbool.h>

// Реализация таймаута с использованием clock

// timeout_ms — таймаут в миллисекундах
bool wait_with_timeout(unsigned int timeout_ms) 
{
    clock_t start = clock();
    // Переводим миллисекунды в тики: CLOCKS_PER_SEC — число тиков в секунду
    clock_t deadline = start + ((clock_t)timeout_ms * CLOCKS_PER_SEC) / 1000;

    while (true) 
    {
        // Здесь можно вставить проверку какого-то условия, например, чтение данных,
        // опрос устройства и т.п. Сейчас просто эмулируем полезную работу.

        // Проверка таймаута
        if (clock() >= deadline) 
        {
            return false; // таймаут истёк
        }

        // Если условие выполнено — возвращаем успех
        // Например, если прочитали данные — раскомментируйте:
        // if (data_ready()) return true;

        // Небольшая пауза, чтобы не гонять процессор на 100% (опционально)
        // Можно использовать usleep/nanosleep, либо оставить пустой цикл
    }
}

int main(void) 
{
    unsigned int timeout = 10000; // 10 секунд

    printf("Waiting with timeout %u ms...\n", timeout);
    if (wait_with_timeout(timeout)) {
        printf("The condition fulfilled befor the timeout expiration.\n");
    } else {
        printf("Timeout expired.\n");
    }

    return 0;
}
