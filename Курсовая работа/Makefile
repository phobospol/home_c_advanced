# Makefile для сборки программы вычисления площади

CC      = gcc
CFLAGS  = -Wall -Wextra -std=c99 -O2
LDFLAGS = -lm

# Объектные файлы
OBJS = main.o root.o integral.o functions.o

# Цель по умолчанию — полная сборка программы
all: program

# Линковка исполняемого файла
program: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDFLAGS)

# Правила компиляции каждого модуля (зависимости указаны явно)
main.o:      main.c functions.h root.h integral.h
	$(CC) $(CFLAGS) -c main.c

root.o:      root.c root.h functions.h
	$(CC) $(CFLAGS) -c root.c

integral.o:  integral.c integral.h functions.h
	$(CC) $(CFLAGS) -c integral.c

functions.o: functions.c functions.h
	$(CC) $(CFLAGS) -c functions.c

# Очистка промежуточных и исполняемых файлов
clean:
	rm -f $(OBJS) program
