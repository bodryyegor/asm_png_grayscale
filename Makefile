CC = gcc
NASM = nasm
NASMFLAGS = -f elf64
LDFLAGS = -lm
OPT_LEVELS = O0 O1 O2 O3 Ofast

# Объектный файл ассемблера общий для всех вариантов
ASM_OBJ = process_asm.o

# Имена исполняемых файлов для каждого уровня оптимизации
TARGETS = $(addprefix prog_,$(OPT_LEVELS))

# Правило по умолчанию: собрать всё
all: $(TARGETS)

# Сборка ассемблерного объекта (один раз)
$(ASM_OBJ): process_asm.asm
        $(NASM) $(NASMFLAGS) $< -o $@

# Шаблон для сборки каждого варианта оптимизации
# $* подставит O0, O1, O2, O3, Ofast
prog_%: main_%.o process_c_%.o $(ASM_OBJ)
        $(CC) $^ -o $@ $(LDFLAGS)

# Компиляция main.c с определённым уровнем оптимизации
main_%.o: main.c
        $(CC) -c -$* $< -o $@

# Компиляция process_c.c с определённым уровнем оптимизации
process_c_%.o: process_c.c
        $(CC) -c -$* $< -o $@

# Запуск бенчмарка: для каждого уровня выполняем программу и собираем результат
benchmark: $(TARGETS)
        @echo "Optimization,C_time_ms,ASM_time_ms" > results.csv
        @for opt in $(OPT_LEVELS); do \
                echo "Running prog_$$opt..."; \
                OUT=$$(./prog_$$opt input.png out_$$opt.png 2>&1); \
                C_TIME=$$(echo "$$OUT" | grep "C version" | awk '{print $$3}'); \
                ASM_TIME=$$(echo "$$OUT" | grep "ASM version" | awk '{print $$3}'); \
                echo "$$opt,$$C_TIME,$$ASM_TIME" >> results.csv; \
        done
        @echo "Benchmark finished. results.csv:"
        @cat results.csv

# Очистка всех сгенерированных файлов
clean:
        rm -f $(ASM_OBJ) main_*.o process_c_*.o prog_* results.csv out_*.png c_*.png asm_*.png

.PHONY: all benchmark clean
