#include <stdio.h>       // стандартный ввод/вывод: printf, fprintf
#include <stdlib.h>      // malloc, free, rand, exit
#include <time.h>        // time() — для еды; clock() не используется после исправления
#include <ncurses/ncurses.h>  // библиотека текстовой графики для терминала
#include <inttypes.h>    // точные типы: int32_t, uint8_t
#include <string.h>      // работа со строками (в данном коде почти не нужна)
#include <unistd.h>      // usleep() — микросекундная задержка

/* Минимальная координата Y (верхняя граница поля).
   Змейка не поднимается выше строки 2, чтобы не затирать интерфейс. */
#define MIN_Y  2

/* Задержка между кадрами в секундах.
   Глобальная, потому что меняется во время игры (ускорение при поедании). */
double DELAY = 0.1;

/* Количество змеек в игре */
#define PLAYERS  2

/* Направления движения: LEFT=1, UP=2, RIGHT=3, DOWN=4 (автоинкремент enum).
   STOP_GAME — клавиша F10 для выхода из игры. */
enum {LEFT=1, UP, RIGHT, DOWN, STOP_GAME=KEY_F(10)};

/* Игровые константы:
   MAX_TAIL_SIZE      — максимум сегментов хвоста (100)
   START_TAIL_SIZE    — начальная длина хвоста (3)
   MAX_FOOD_SIZE      — размер массива еды (20 ячеек, но используются 3)
   FOOD_EXPIRE_SECONDS — еда исчезает через 10 секунд
   SEED_NUMBER        — сколько порций еды одновременно на экране (3)
   CONTROLS           — количество наборов управления (2: стрелки и WASD) */
enum {MAX_TAIL_SIZE=100, START_TAIL_SIZE=3, MAX_FOOD_SIZE=20,
      FOOD_EXPIRE_SECONDS=10, SEED_NUMBER=3, CONTROLS=2
     };

/* --- Доступные цвета для змеек --- */
#define NUM_COLORS 6

/* Коды цветов ncurses для 6 вариантов */
int fg_colors[NUM_COLORS] = {COLOR_RED, COLOR_BLUE, COLOR_YELLOW,
                             COLOR_MAGENTA, COLOR_CYAN, COLOR_WHITE
                            };

/* Текстовые названия цветов для отображения в меню */
const char* color_names[NUM_COLORS] = {"Red", "Blue", "Yellow",
                                       "Magenta", "Cyan", "White"
                                      };

/* --- Настройки, выбранные в меню ---
   Заполняются функцией startMenu() до начала игры. */
int game_mode = 0;        // 0 = против ИИ, 1 = два игрока
int p1_color_idx = 0;     // индекс цвета первого игрока в массиве fg_colors
int p2_color_idx = 1;     // индекс цвета второго игрока
double start_delay = 0.1; // начальная скорость (задержка в секундах)

/* Структура: набор из четырёх клавиш для управления одной змейкой */
struct control_buttons
{
    int down;
    int up;
    int left;
    int right;
};

/* Массив из двух наборов управления по умолчанию: стрелки и WASD.
   В коде почти не используется — заменён отдельными переменными ниже. */
struct control_buttons default_controls[CONTROLS] = {
    {KEY_DOWN, KEY_UP, KEY_LEFT, KEY_RIGHT},
    {'s', 'w', 'a', 'd'}
};

/* Готовые наборы управления для первого и второго игрока */
struct control_buttons pleer1_controls = {KEY_DOWN, KEY_UP, KEY_LEFT, KEY_RIGHT};
struct control_buttons pleer2_controls = {'s', 'w', 'a', 'd'};

/* Объявлен ДО snake_t, чтобы избежать предупреждения компилятора
   о зависимости типов (в snake_t есть указатель на tail_t). */
typedef struct tail_t
{
    int x;  // координата X сегмента хвоста
    int y;  // координата Y сегмента хвоста
} tail_t;

/* Главная структура змейки */
typedef struct snake_t
{
    int x;                        // координата головы X
    int y;                        // координата головы Y
    int direction;                // текущее направление (LEFT/UP/RIGHT/DOWN)
    size_t tsize;                 // длина хвоста (количество сегментов)
    struct tail_t *tail;          // указатель на массив сегментов хвоста
    struct control_buttons controls; // клавиши управления этой змейкой
} snake_t;

/* Глобальный массив указателей на две змейки.
   Сделан глобальным, чтобы функция drawUI могла к нему обращаться. */
snake_t* snakes[PLAYERS];

/* Структура одной порции еды */
struct food
{
    int x;            // координата X
    int y;            // координата Y
    time_t put_time;  // время появления (для проверки истечения срока)
    char point;        // символ, которым рисуется ('$')
    uint8_t enable;    // активна ли еда (1 = да, 0 = съедена)
} food[MAX_FOOD_SIZE]; // массив из 20 ячеек, одновременно активны только 3

/* Обнуление массива еды: все порции становятся неактивными */
void initFood(struct food f[], size_t size)
{
    struct food init = {0, 0, 0, 0, 0}; // шаблон с нулями
    for (size_t i = 0; i < size; i++)
        f[i] = init;                    // заполняем весь массив
}

/* Выбор цветовой пары для отрисовки объекта.
   objectType: 1 = змейка 1, 2 = змейка 2, 3 = еда. */
void setColor(int objectType)
{
    // Сначала отключаем все три пары, чтобы не было наложения цветов
    attroff(COLOR_PAIR(1));
    attroff(COLOR_PAIR(2));
    attroff(COLOR_PAIR(3));
    // Включаем нужную
    switch (objectType) 
    {
    case 1:
        attron(COLOR_PAIR(1));
        break;  // цвет змейки 1
    case 2:
        attron(COLOR_PAIR(2));
        break;  // цвет змейки 2
    case 3:
        attron(COLOR_PAIR(3));
        break;  // цвет еды (зелёный)
    }
}

/* Размещение одной порции еды в случайном месте экрана */
void putFoodSeed(struct food *fp)
{
    int max_x = 0, max_y = 0;
    char spoint[2] = {0};                    // строка из одного символа + '\0'
    getmaxyx(stdscr, max_y, max_x);         // получаем размеры терминала

    // Если на этом месте уже была еда — стираем её пробелом
    if (fp->y >= 0 && fp->y < max_y && fp->x >= 0 && fp->x < max_x)
        mvprintw(fp->y, fp->x, " ");

    // Случайные координаты:
    // X — от 0 до max_x-2
    // Y — от 1 до max_y-2 (не в нулевой строке, там интерфейс)
    fp->x = rand() % (max_x - 1);
    fp->y = rand() % (max_y - 2) + 1;

    // Записываем время появления, символ, флаг активности
    fp->put_time = time(NULL);
    fp->point = '$';
    fp->enable = 1;
    spoint[0] = fp->point;  // кладём символ в строку для mvprintf

    // Рисуем еду зелёным цветом (пара 3)
    setColor(3);
    mvprintw(fp->y, fp->x, "%s", spoint);
}

/* Размещение нескольких порций еды (вызывает putFoodSeed в цикле) */
void putFood(struct food f[], size_t number_seeds)
{
    for (size_t i = 0; i < number_seeds; i++)
        putFoodSeed(&f[i]);
}

/* Обновление еды: перевыгенерация съеденной или протухшей (старше 10 секунд) */
void refreshFood(struct food f[], int nfood)
{
    for (size_t i = 0; i < nfood; i++)
    {
        // Если у порции есть время создания
        if (f[i].put_time)
        {
            // Если она съедена (!enable) или висит дольше FOOD_EXPIRE_SECONDS —
            // перегенерируем в новом месте
            if (!f[i].enable ||
                    (time(NULL) - f[i].put_time) > FOOD_EXPIRE_SECONDS)
            {
                putFoodSeed(&f[i]);
            }
        }
    }
}

/* Обнуление массива сегментов хвоста (все координаты в 0,0) */
void initTail(struct tail_t t[], size_t size)
{
    struct tail_t init_t = {0, 0};
    for (size_t i = 0; i < size; i++)
        t[i] = init_t;
}

/* Инициализация головы змейки: ставим в заданные координаты,
   направление по умолчанию — вправо */
void initHead(struct snake_t *head, int x, int y)
{
    head->x = x;
    head->y = y;
    head->direction = RIGHT;
}

/* Создание змейки: выделение памяти, инициализация головы и хвоста.
   head[] — массив указателей на змеек, i — индекс в этом массиве.
   size — начальная длина хвоста, (x, y) — стартовая позиция. */
void initSnake(snake_t *head[], size_t size, int x, int y, int i)
{
    // Выделяем память под структуру змейки
    head[i] = (snake_t *)malloc(sizeof(snake_t));
    if (!head[i])                            // проверка: если malloc вернул NULL
    {
        fprintf(stderr, "malloc failed for snake %d\n", i);
        exit(1);
    }

    // Выделяем память под массив из 100 сегментов хвоста
    tail_t *tail = (tail_t *)malloc(MAX_TAIL_SIZE * sizeof(tail_t));
    if (!tail)                               // проверка
    {
        fprintf(stderr, "malloc failed for tail %d\n", i);
        free(head[i]);                       // освобождаем змейку
        exit(1);
    }

    initTail(tail, MAX_TAIL_SIZE);            // обнуляем хвост
    initHead(head[i], x, y);                 // ставим голову
    head[i]->tail = tail;                    // связываем хвост со змейкой
    head[i]->tsize = size + 1;               // длина = 3 + 1 = 4 (включая голову)
}

/* Движение головы змейки на один шаг в текущем направлении.
   colorType — номер цветовой пары (1 или 2). */
void go(struct snake_t *head, int colorType)
{
    char ch = '@';                           // символ головы
    int max_x = 0, max_y = 0;
    getmaxyx(stdscr, max_y, max_x);          // размеры экрана

    // Стираем голову с текущей позиции пробелом
    if (head->y >= 0 && head->y < max_y && head->x >= 0 && head->x < max_x)
        mvprintw(head->y, head->x, " ");

    setColor(colorType);                     // включаем цвет этой змейки

    // Двигаем голову в зависимости от направления
    switch (head->direction)
    {
    case LEFT:
        head->x--;
        if (head->x < 0) head->x = max_x - 1;  // заворот: вышел за левый край → к правому
        break;
    case RIGHT:
        head->x++;
        if (head->x >= max_x) head->x = 0;     // заворот: вышел за правый край → к левому
        break;
    case UP:
        head->y--;
        if (head->y < 1) head->y = max_y - 1;  // не поднимаемся выше строки 1 (там интерфейс)
        break;
    case DOWN:
        head->y++;
        if (head->y >= max_y) head->y = 1;      // не выходим за нижний край → к строке 1
        break;
    default:
        break;
    }

    // Рисуем голову на новой позиции (с проверкой границ)
    if (head->y >= 0 && head->y < max_y && head->x >= 0 && head->x < max_x)
        mvprintw(head->y, head->x, "%c", ch);

    refresh();                                   // отправляем изменения на экран
}

/* Смена направления по нажатой клавише.
   key — код клавиши, полученный из getch(). */
void changeDirection(struct snake_t *snake, const int32_t key)
{
    int32_t lower_key = key;
    // Если нажата заглавная буква — приводим к строчной
    // (чтобы работали и 'W', и 'w')
    if (key >= 'A' && key <= 'Z')
        lower_key = key + ('a' - 'A');

    // Сравниваем нажатую клавишу с клавишами управления этой змейки
    // и меняем направление. Для стрелок lower_key совпадает с key
    // (они не буквы, поэтому преобразование их не меняет).
    if (lower_key == snake->controls.down)
        snake->direction = DOWN;
    else if (lower_key == snake->controls.up)
        snake->direction = UP;
    else if (lower_key == snake->controls.right)
        snake->direction = RIGHT;
    else if (lower_key == snake->controls.left)
        snake->direction = LEFT;
}

/* Движение и отрисовка хвоста змейки.
   Сначала сдвигаем сегменты, потом стираем старый последний, потом рисуем. */
void goTail(struct snake_t *head, int colorType)
{
    char ch = '*';                           // символ сегмента хвоста
    int max_x, max_y;
    getmaxyx(stdscr, max_y, max_x);          // размеры экрана

    // Запоминаем позицию старого последнего сегмента — его потом сотрём
    int old_last_x = 0, old_last_y = 0;
    if (head->tsize > 0) 
    {
        old_last_x = head->tail[head->tsize - 1].x;
        old_last_y = head->tail[head->tsize - 1].y;
    }

    // Сначала сдвигаем хвост (идём с конца, чтобы не затереть данные)
    // Каждый сегмент берёт координаты предыдущего
    for (size_t i = head->tsize - 1; i > 0; i--)
        head->tail[i] = head->tail[i - 1];

    // Первый сегмент становится туда, где сейчас голова
    // (голова уже сдвинулась в go(), так что это «старая» позиция головы)
    head->tail[0].x = head->x;
    head->tail[0].y = head->y;

    // Стираем старый последний сегмент пробелом (с проверкой границ)
    if (old_last_y >= 0 && old_last_y < max_y &&
            old_last_x >= 0 && old_last_x < max_x)
        mvprintw(old_last_y, old_last_x, " ");

    setColor(colorType);                     // включаем цвет змейки

    // Рисуем все сегменты хвоста, начиная с первого (нулевой под головой)
    for (size_t i = 1; i < head->tsize; i++)
    {
        // Проверяем границы и что сегмент не (0,0) — неинициализированный
        if (head->tail[i].y >= 1 && head->tail[i].y < max_y &&
                head->tail[i].x >= 0 && head->tail[i].x < max_x &&
                (head->tail[i].y || head->tail[i].x))
            mvprintw(head->tail[i].y, head->tail[i].x, "%c", ch);
    }
}

/* Проверка: съела ли змейка еду.
   Возвращает 1, если голова совпала с активной едой, иначе 0. */
_Bool haveEat(struct snake_t *head, struct food f[])
{
    for (size_t i = 0; i < MAX_FOOD_SIZE; i++)
    {
        // Если еда активна и координаты совпадают с головой
        if (f[i].enable && f[i].x == head->x && f[i].y == head->y)
        {
            f[i].enable = 0;                 // помечаем как съеденную
            mvprintw(f[i].y, f[i].x, " ");   // стираем с экрана
            return 1;
        }
    }
    return 0;
}

/* Рост хвоста на один сегмент.
   новый сегмент = копия текущего последнего, потом tsize++. */
void addTail(struct snake_t *head)
{
    if (head->tsize >= MAX_TAIL_SIZE)        // если хвост уже максимальный — выходим
        return;

    // Новый сегмент получает координаты последнего существующего
    head->tail[head->tsize].x = head->tail[head->tsize - 1].x;
    head->tail[head->tsize].y = head->tail[head->tsize - 1].y;
    head->tsize++;                           // увеличиваем длину
}

/* Проверка допустимости смены направления.
   Запрещает разворот на 180° (если змейка идёт вверх, нельзя нажать «вниз»).
   Возвращает 0 (запрет) или 1 (можно). */
int checkDirection(snake_t *snake, int32_t key)
{
    int32_t lower_key = key;
    // Нормализация регистра (та же логика, что в changeDirection)
    if (key >= 'A' && key <= 'Z')
        lower_key = key + ('a' - 'A');

    // Если нажата «вниз», а змейка идёт вверх — запрет
    if (lower_key == snake->controls.down && snake->direction == UP)
        return 0;
    // Если нажата «вверх», а змейка идёт вниз — запрет
    if (lower_key == snake->controls.up && snake->direction == DOWN)
        return 0;
    // Если нажата «влево», а змейка идёт вправо — запрет
    if (lower_key == snake->controls.left && snake->direction == RIGHT)
        return 0;
    // Если нажата «вправо», а змейка идёт влево — запрет
    if (lower_key == snake->controls.right && snake->direction == LEFT)
        return 0;
    return 1;  // во всех остальных случаях — можно
}

/* Манхэттенское расстояние между змейкой и едой.
   Сумма модулей разностей по X и Y. Используется ИИ. */
int distance(const snake_t snake, const struct food food)
{
    return (abs(snake.x - food.x) + abs(snake.y - food.y));
}

/* Вычисление следующей позиции головы при движении в направлении dir.
   Саму змейку не двигает — только записывает результат в nx, ny.
   Учитывает заворот экрана (выход за край → противоположный край). */
void getNextPos(snake_t *snake, int dir, int *nx, int *ny)
{
    int max_x, max_y;
    getmaxyx(stdscr, max_y, max_x);

    *nx = snake->x;                          // начинаем с текущих координат
    *ny = snake->y;

    switch (dir) 
    {
    case LEFT:
        (*nx)--;
        if (*nx < 0) *nx = max_x - 1;    // заворот
        break;
    case RIGHT:
        (*nx)++;
        if (*nx >= max_x) *nx = 0;       // заворот
        break;
    case UP:
        (*ny)--;
        if (*ny < 1) *ny = max_y - 1;    // не выше строки 1
        break;
    case DOWN:
        (*ny)++;
        if (*ny >= max_y) *ny = 1;       // не ниже нижнего края
        break;
    }
}

/* Проверка: безопасен ли ход в позицию (nx, ny).
   Возвращает 0, если новая позиция совпадает с любым сегментом хвоста. */
_Bool isValidMove(snake_t *snake, int nx, int ny)
{
    for (size_t i = 0; i < snake->tsize; i++) 
    {
        if (nx == snake->tail[i].x && ny == snake->tail[i].y)
            return 0;                        // врежемся в хвост
    }
    return 1;                                // безопасно
}

/* ИИ змейки: выбирает направление к ближайшей еде.
   Алгоритм:
   1. Найти ближайшую активную еду.
   2. Перебрать 4 направления (кроме разворота на 180°).
   3. Выбрать то, которое ближе всего к еде и не врежется в хвост.
   4. Если ни одно не приближает — идти в любой безопасный ход. */
void autoChangeDirection(snake_t *snake, struct food food[], int foodSize)
{
    int pointer = -1;                        // индекс ближайшей еды
    // Ищем ближайшую активную еду
    for (int i = 0; i < foodSize; i++) 
    {
        if (!food[i].enable) continue;       // пропускаем съеденную
        if (pointer == -1 || distance(*snake, food[i]) < distance(*snake, food[pointer]))
            pointer = i;                     // запоминаем, если ближе
    }

    if (pointer == -1)                       // если еды нет — оставляем направление
        return;

    const int dirs[4] = {UP, DOWN, LEFT, RIGHT};
    int bestDir = -1;                        // оптимальный ход (ближе к еде)
    int bestDist = -1;                       // расстояние при оптимальном ходе
    int safeFallback = -1;                   // любой безопасный ход (запасной)

    // Перебираем все четыре направления
    for (int k = 0; k < 4; k++) 
    {
        int d = dirs[k];

        // Пропускаем разворот на 180°
        if ((snake->direction == UP && d == DOWN) ||
                (snake->direction == DOWN && d == UP) ||
                (snake->direction == LEFT && d == RIGHT) ||
                (snake->direction == RIGHT && d == LEFT))
            continue;

        int nx, ny;
        getNextPos(snake, d, &nx, &ny);      // куда попадёт голова

        if (!isValidMove(snake, nx, ny))     // если врежемся в хвост — пропускаем
            continue;

        if (safeFallback == -1)              // запоминаем первый безопасный ход
            safeFallback = d;

        // Создаём «виртуальную» змейку в новой позиции
        snake_t probe;
        probe.x = nx;
        probe.y = ny;
        int newDist = distance(probe, food[pointer]);  // расстояние до еды

        // Если это направление приближает к еде сильнее — запоминаем
        if (bestDir == -1 || newDist < bestDist) 
        {
            bestDist = newDist;
            bestDir = d;
        }
    }

    // В конце: если нашли направление к еде — идём туда.
    // Иначе — идём в любой безопасный ход.
    if (bestDir != -1)
        snake->direction = bestDir;
    else if (safeFallback != -1)
        snake->direction = safeFallback;
}

/* Перерисовка интерфейса каждый кадр, чтобы текст не затирался змейкой.
   Рисует в нулевой строке: подсказку по управлению (слева)
   и длины обеих змеек (справа). */
void drawUI(snake_t *sn[], int mode)
{
    int max_x = getmaxx(stdscr);
    if (mode == 0)
        mvprintw(0, 0, "P1: arrows  P2: AI      Press 'F10' for EXIT");
    else
        mvprintw(0, 0, "P1: arrows  P2: WSAD    Press 'F10' for EXIT");
    mvprintw(0, max_x - 30, "P1 len: %3d  P2 len: %3d",
             (int)sn[0]->tsize, (int)sn[1]->tsize);
}

/* Обновление одной змейки за один кадр.
   head     — указатель на змейку
   f        — массив еды
   key      — нажатая клавиша (или ERR, если ничего не нажато)
   is_auto  — 1, если змейкой управляет ИИ; 0, если игрок
   colorType — номер цветовой пары (1 или 2) */
void update(snake_t *head, struct food f[], int key, int is_auto, int colorType)
{
    if (is_auto)
        autoChangeDirection(head, f, SEED_NUMBER);  // ИИ выбирает направление
    else
    {
        // Если клавиша нажата и направление допустимо — меняем
        if (key != ERR && checkDirection(head, key))
            changeDirection(head, key);
    }

    go(head, colorType);                     // двигаем и рисуем голову
    goTail(head, colorType);                 // двигаем и рисуем хвост

    // Используем параметр f, а не глобальный food
    refreshFood(f, SEED_NUMBER);             // обновляем еду (перегенерация устаревшей)

    if (haveEat(head, f))                    // если съели еду
    {
        addTail(head);                       // растим хвост
        DELAY -= 0.009;                       // ускоряем игру
        if (DELAY < 0.01)                    // но не быстрее 0.01 секунды
            DELAY = 0.01;
    }

    refresh();                               // отправляем кадр на экран

    // Переводим секунды в микросекунды (умножение на 1 000 000)
    usleep((useconds_t)(DELAY * 1000000));
}

/* Проверка столкновения головы с собственным хвостом.
   Начинаем с i=1, пропуская tail[0] (он под головой).
   Возвращает 1 при столкновении, иначе 0. */
_Bool isCrush(snake_t *snake)
{
    for (size_t i = 1; i < snake->tsize; i++)
    {
        if (snake->x == snake->tail[i].x && snake->y == snake->tail[i].y)
            return 1;                        // голова совпала с сегментом хвоста
    }
    return 0;
}

/* Проверка столкновения текущей змейки с другой змейкой.
   Проверяет голову current против головы и хвоста всех остальных.
   snakes[] — массив змеек, current — индекс текущей, total — всего змеек. */
_Bool isCrushWithOther(snake_t *snakes[], int current, int total)
{
    for (int j = 0; j < total; j++)
    {
        if (j == current)                     // себя не проверяем
            continue;

        // Голова текущей совпала с головой другой
        if (snakes[current]->x == snakes[j]->x &&
                snakes[current]->y == snakes[j]->y)
            return 1;

        // Голова текущей совпала с любым сегментом хвоста другой
        for (size_t i = 0; i < snakes[j]->tsize; i++)
        {
            if (snakes[current]->x == snakes[j]->tail[i].x &&
                    snakes[current]->y == snakes[j]->tail[i].y)
                return 1;
        }
    }
    return 0;
}

/* Проверка: не появилась ли еда на теле змейки или на другой еде.
   Если да — перегенерируем в новом месте. */
void repairSeed(struct food f[], size_t nfood, struct snake_t *head)
{
    // Проверяем: не легла ли еда на какой-либо сегмент хвоста
    for (size_t i = 0; i < head->tsize; i++)
        for (size_t j = 0; j < nfood; j++)
        {
            if (f[j].enable && head->tail[i].x == f[j].x &&
                    head->tail[i].y == f[j].y)
                putFoodSeed(&f[j]);           // перегенерируем
        }

    // Проверяем: не совпали ли две порции еды по координатам
    for (size_t i = 0; i < nfood; i++)
        for (size_t j = 0; j < nfood; j++)
        {
            if (i != j && f[i].enable && f[j].enable &&
                    f[i].x == f[j].x && f[i].y == f[j].y)
                putFoodSeed(&f[j]);           // перегенерируем вторую
        }
}

/*
 Стартовое меню: приветствие, выбор режима, цвета змеек и скорости.
 Использует цветные пары 4..4+NUM_COLORS-1 для превью цветов в меню.
 После выбора записывает результат в глобальные переменные:
 game_mode, p1_color_idx, p2_color_idx, start_delay.
*/
void startMenu()
{
    int ch;
    int row;

    /* --- Приветствие --- */
    clear();                                 // стираем экран
    mvprintw(3, 10, "**************************************");
    mvprintw(4, 10, "*                                    *");
    mvprintw(5, 10, "*        S N A K E   G A M E         *");
    mvprintw(6, 10, "*                                    *");
    mvprintw(7, 10, "*         by Alexey Polydov          *");
    mvprintw(8, 10, "*                                    *");
    mvprintw(9, 10, "**************************************");
    mvprintw(10, 10,"      Welcome to the Snake Game!");
    mvprintw(11, 10,"     Press any key to continue...");
    refresh();                               // отправляем на экран
    getch();                                  // ждём нажатия любой клавиши

    /* --- Выбор режима игры --- */
    clear();
    mvprintw(3, 10, "=== Select Game Mode ===");
    mvprintw(5, 10, "  1. Player vs AI");
    mvprintw(6, 10, "  2. Two players (local)");
    mvprintw(8, 10, "Press 1 or 2: ");
    refresh();
    // Бесконечный цикл, ждём нажатия '1' или '2'
    while (1) 
    {
        ch = getch();
        if (ch == '1') 
        {
            game_mode = 0;    // 0 = против ИИ
            break;
        }
        if (ch == '2') {
            game_mode = 1;    // 1 = два игрока
            break;
        }
    }

    /* --- Цвет для игрока 1 --- */
    clear();
    mvprintw(3, 10, "=== Player 1 - Choose Color ===");
    // Для каждого из 6 цветов рисуем образец и название
    for (int i = 0; i < NUM_COLORS; i++) 
    {
        row = 5 + i;
        // Включаем временную пару (4+i) для превью цвета в меню
        attron(COLOR_PAIR(4 + i));
        mvprintw(row, 12, "####");           // четыре решётки как образец цвета
        attroff(COLOR_PAIR(4 + i));
        mvprintw(row, 20, "%d. %s", i + 1, color_names[i]); // номер и название
    }
    mvprintw(5 + NUM_COLORS + 1, 10, "Press 1-%d: ", NUM_COLORS);
    refresh();
    // Ждём нажатия клавиши от '1' до '6'
    while (1) 
    {
        ch = getch();
        if (ch >= '1' && ch <= '0' + NUM_COLORS) 
        {
            p1_color_idx = ch - '1';         // переводим символ в индекс: '1'→0, '2'→1 ...
            break;
        }
    }

    /* --- Цвет для игрока 2 (или ИИ) --- */
    clear();
    // Заголовок зависит от режима
    if (game_mode == 1)
        mvprintw(3, 10, "=== Player 2 - Choose Color ===");
    else
        mvprintw(3, 10, "=== AI Snake - Choose Color ===");
    // Тот же список цветов, но рядом с занятым — пометка
    for (int i = 0; i < NUM_COLORS; i++) 
    {
        row = 5 + i;
        attron(COLOR_PAIR(4 + i));
        mvprintw(row, 12, "####");
        attroff(COLOR_PAIR(4 + i));
        mvprintw(row, 20, "%d. %s", i + 1, color_names[i]);
        // Если цвет уже занят первым игроком — помечаем
        if (i == p1_color_idx)
            mvprintw(row, 35, "(taken by P1)");
    }
    mvprintw(5 + NUM_COLORS + 1, 10, "Press 1-%d: ", NUM_COLORS);
    refresh();
    // Ждём нажатия, проверяем, что цвет не занят
    while (1) 
    {
        ch = getch();
        if (ch >= '1' && ch <= '0' + NUM_COLORS) 
        {
            int idx = ch - '1';
            if (idx == p1_color_idx) 
            {
                // Цвет уже занят — предупреждаем и ждём заново
                mvprintw(5 + NUM_COLORS + 2, 10, "Color already taken! Choose another.");
                refresh();
                continue;
            }
            p2_color_idx = idx;
            break;
        }
    }

    /* --- Выбор сложности (скорости) --- */
    clear();
    mvprintw(3, 10, "=== Difficulty ===");
    mvprintw(5, 10, "  1. Easy   (slow)");     // задержка 0.15 с
    mvprintw(6, 10, "  2. Normal (medium)");  // задержка 0.10 с
    mvprintw(7, 10, "  3. Hard   (fast)");     // задержка 0.05 с
    mvprintw(9, 10, "Press 1, 2 or 3: ");
    refresh();
    while (1) {
        ch = getch();
        if (ch == '1') 
        {
            start_delay = 0.15;
            break;
        }
        if (ch == '2') {
            start_delay = 0.1;
            break;
        }
        if (ch == '3') {
            start_delay = 0.05;
            break;
        }
    }

    /* --- Итоговая сводка --- */
    clear();
    mvprintw(3, 10, "=== Game Settings ===");
    mvprintw(5, 10, "Mode:   %s", game_mode == 0 ? "Player vs AI" : "Two players");
    mvprintw(6, 10, "P1:     %s", color_names[p1_color_idx]);
    mvprintw(7, 10, "P2:     %s", color_names[p2_color_idx]);
    // Название скорости определяется по значению start_delay
    mvprintw(8, 10, "Speed:  %s", start_delay == 0.15 ? "Easy" :
             (start_delay == 0.1 ? "Normal" : "Hard"));
    mvprintw(10, 10, "Controls:  P1 = arrows   P2 = %s",
             game_mode == 0 ? "AI" : "WSAD");
    mvprintw(12, 10, "Press any key to start!");
    refresh();
    getch();                                  // ждём нажатия
    clear();                                  // очищаем экран перед стартом
}

int main()
{
    /* Создаём двух змеек:
       Змейка 0: старт в (10, 2), начальный хвост 3
       Змейка 1: старт в (20, 4), начальный хвост 3 */
    for (int i = 0; i < PLAYERS; i++)
        initSnake(snakes, START_TAIL_SIZE, 10 + i * 10, 2 + i * 2, i);

    // Назначаем клавиши управления: стрелки — первому, WASD — второму
    snakes[0]->controls = pleer1_controls;
    snakes[1]->controls = pleer2_controls;

    /* Инициализация ncurses */
    initscr();                  // запуск экрана
    keypad(stdscr, TRUE);       // включаем обработку стрелок
    raw();                      // отключаем буферизацию: клавиши идут сразу
    noecho();                   // не печатать нажатые клавиши на экран
    curs_set(FALSE);            // спрятать курсор

    /* Включаем цветной режим */
    start_color();

    /* Создаём 6 временных пар (4–9) для превью цветов в меню:
       каждый цвет на чёрном фоне */
    for (int i = 0; i < NUM_COLORS; i++)
        init_pair(4 + i, fg_colors[i], COLOR_BLACK);

    /* Показываем меню — заполняет game_mode, p1_color_idx,
       p2_color_idx, start_delay */
    startMenu();

    /* Создаём игровые цветовые пары на основе выбранных в меню цветов:
       пара 1 — цвет первого игрока
       пара 2 — цвет второго игрока (или ИИ)
       пара 3 — еда (зелёная) */
    init_pair(1, fg_colors[p1_color_idx], COLOR_BLACK);
    init_pair(2, fg_colors[p2_color_idx], COLOR_BLACK);
    init_pair(3, COLOR_GREEN, COLOR_BLACK);

    /* Применяем выбранную скорость */
    DELAY = start_delay;

    /* Делаем getch() неблокирующим: если клавиш не нажато,
       возвращает ERR сразу, не дожидаясь ввода */
    timeout(0);

    /* Обнуляем массив еды и раскладываем 3 порции */
    initFood(food, MAX_FOOD_SIZE);
    putFood(food, SEED_NUMBER);

    int key_pressed = 0;       // последняя нажатая клавиша
    int game_over = 0;         // флаг окончания игры

    /* Главный игровой цикл — работает, пока не нажат F10 или не game_over */
    while (key_pressed != STOP_GAME && !game_over)
    {
        key_pressed = getch(); // читаем клавишу (неблокирующе)

        // Если что-то нажато — вычитываем все остальные клавиши
        // из буфера, оставляя последнюю. Так не теряется ввод при быстром нажатии.
        if (key_pressed != ERR)
        {
            int tmp;
            while ((tmp = getch()) != ERR)
                key_pressed = tmp;
        }

        /* Обновляем каждую змейку */
        for (int i = 0; i < PLAYERS; i++)
        {
            // Автопилот только для второй змейки в режиме «против ИИ»
            int is_auto = (game_mode == 0 && i == 1) ? 1 : 0;

            // Обновляем змейку: движение, отрисовка, проверка еды
            update(snakes[i], food, key_pressed, is_auto, i + 1);

            // Проверка столкновения с собственным хвостом
            if (isCrush(snakes[i]))
            {
                game_over = 1;
                break;
            }

            // Проверка столкновения с другой змейкой
            if (isCrushWithOther(snakes, i, PLAYERS))
            {
                game_over = 1;
                break;
            }

            // Проверяем: не легла ли еда на тело этой змейки
            repairSeed(food, SEED_NUMBER, snakes[i]);
        }

        // Если игра не окончена — перерисовываем интерфейс,
        // чтобы текст не затёрся змейкой
        if (!game_over)
            drawUI(snakes, game_mode);
    }

    /* --- Экран Game Over --- */
    clear();
    mvprintw(5, 10, "=== GAME OVER ===");
    mvprintw(7, 10, "Press any key to exit...");
    refresh();
    nodelay(stdscr, FALSE);   // включаем блокирующий режим ввода
    getch();                   // ждём нажатия любой клавиши

    /* --- Очистка памяти --- */
    // Освобождаем: сначала хвост, потом саму змейку
    for (int i = 0; i < PLAYERS; i++)
    {
        free(snakes[i]->tail);
        free(snakes[i]);
    }

    // Корректное завершение ncurses — возвращает терминал в нормальный режим
    endwin();
    return 0;
}
