#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define MAX_PUMPS 100
#define MAX_PACKETS 100

typedef struct {
    int id;
    float Pt;
    float Pd;
    float Pu;
} Pump;

typedef struct {
    int packet_id;
    int pump_id;
    char command[20];
    float value;
} CommandPacket;

// Сортировки
void sort_by_pt_desc(Pump *table, int n) {
    for (int i = 0; i < n-1; i++) {
        for (int j = 0; j < n-i-1; j++) {
            if (table[j].Pt < table[j+1].Pt) {
                Pump temp = table[j];
                table[j] = table[j+1];
                table[j+1] = temp;
            }
        }
    }
}

void sort_by_pt_asc(Pump *table, int n) {
    for (int i = 0; i < n-1; i++) {
        for (int j = 0; j < n-i-1; j++) {
            if (table[j].Pt > table[j+1].Pt) {
                Pump temp = table[j];
                table[j] = table[j+1];
                table[j+1] = temp;
            }
        }
    }
}

void sort_by_packet_id_asc(CommandPacket *table, int n) {
    for (int i = 0; i < n-1; i++) {
        for (int j = 0; j < n-i-1; j++) {
            if (table[j].packet_id > table[j+1].packet_id) {
                CommandPacket temp = table[j];
                table[j] = table[j+1];
                table[j+1] = temp;
            }
        }
    }
}

// Основная логика обработки насосов
void process_pumps(Pump *pumps, int n, float *Pi) {
    sort_by_pt_desc(pumps, n);
    for (int i = 0; i < n; i++) pumps[i].Pu = pumps[i].Pt;
    sort_by_pt_asc(pumps, n);

    for (int i = 0; i < n; i++) {
        float delta = pumps[i].Pd - pumps[i].Pt;

        if (*Pi < 0) {
            if (pumps[i].Pt > -(*Pi)) {
                pumps[i].Pu = pumps[i].Pt + *Pi;
                *Pi += pumps[i].Pt;
            } else {
                pumps[i].Pu = 130;
                *Pi += pumps[i].Pt;
            }
        } else {
            if (delta <= *Pi) {
                pumps[i].Pu = pumps[i].Pd;
                *Pi -= pumps[i].Pd;
            } else {
                pumps[i].Pu = pumps[i].Pt * (1 + *Pi / 2650);
                *Pi -= pumps[i].Pd;
            }
        }
    }
}

// Запись результатов в файл
void write_results(Pump *pumps, int n, CommandPacket *packets, int packet_count, const char *filename) {
    FILE *file = fopen(filename, "w");
    if (!file) {
        printf("Ошибка записи в файл!\n");
        return;
    }

    fprintf(file, "Итоговые значения Pu:\n");
    for (int i = 0; i < n; i++) {
        fprintf(file, "Насос %d: Pu = %.2f\n", pumps[i].id, pumps[i].Pu);
    }

    fprintf(file, "\nПакеты команд:\n");
    int current_packet = 1;
    for (int i = 0; i < packet_count; i++) {
        if (packets[i].packet_id != current_packet) {
            current_packet = packets[i].packet_id;
            fprintf(file, "\nПакет %d:\n", current_packet);
        }
        fprintf(file, "Насос %d: %s = %.2f\n", packets[i].pump_id, packets[i].command, packets[i].value);
    }

    fclose(file);
}

int main() {
    Pump pumps[MAX_PUMPS];
    CommandPacket packets[MAX_PACKETS];
    int pump_count = 0;
    float Pg, Pi;
    char input_filename[100], output_filename[100];

    printf("=== Система управления насосами ===\n");

    // Выбор ввода
    printf("Ввод данных:\n1. Из файла\n2. Вручную\nВыберите: ");
    int choice;
    scanf("%d", &choice);

    if (choice == 1) {
        printf("Введите имя входного файла: ");
        scanf("%s", input_filename);

        FILE *file = fopen(input_filename, "r");
        if (!file) {
            printf("Ошибка открытия файла!\n");
            return 1;
        }

        fscanf(file, "%f %f", &Pg, &Pi);
        fscanf(file, "%d", &pump_count);
        for (int i = 0; i < pump_count; i++) {
            fscanf(file, "%d %f %f", &pumps[i].id, &pumps[i].Pt, &pumps[i].Pd);
            pumps[i].Pu = 0;
        }
        fclose(file);
    } else {
        printf("Введите Pg и Pi: ");
        scanf("%f %f", &Pg, &Pi);
        printf("Введите количество насосов: ");
        scanf("%d", &pump_count);

        for (int i = 0; i < pump_count; i++) {
            printf("Насос %d (id Pt Pd): ", i+1);
            scanf("%d %f %f", &pumps[i].id, &pumps[i].Pt, &pumps[i].Pd);
            pumps[i].Pu = 0;
        }
    }

    printf("Введите имя выходного файла: ");
    scanf("%s", output_filename);

    // Обработка
    process_pumps(pumps, pump_count, &Pi);

    // Формирование пакетов
    int packet_count = 0;
    for (int i = 0; i < pump_count; i++) {
        if (pumps[i].Pu != 0) {
            packets[packet_count].packet_id = (i % 3) + 1; // Группировка по 3 насоса
            packets[packet_count].pump_id = pumps[i].id;
            strcpy(packets[packet_count].command, "Мощность");
            packets[packet_count].value = pumps[i].Pu;
            packet_count++;
        }
    }

    // Сортировка пакетов
    sort_by_packet_id_asc(packets, packet_count);

    // Вывод в консоль
    printf("\nРезультаты:\n");
    for (int i = 0; i < pump_count; i++) {
        printf("Насос %d: Pu = %.2f\n", pumps[i].id, pumps[i].Pu);
    }

    printf("\nПакеты команд:\n");
    int current_packet = 1;
    for (int i = 0; i < packet_count; i++) {
        if (packets[i].packet_id != current_packet) {
            current_packet = packets[i].packet_id;
            printf("\nПакет %d:\n", current_packet);
        }
        printf("Насос %d: %s = %.2f\n", packets[i].pump_id, packets[i].command, packets[i].value);
    }

    // Запись в файл
    write_results(pumps, pump_count, packets, packet_count, output_filename);
    printf("\nРезультаты сохранены в %s\n", output_filename);

    return 0;
}