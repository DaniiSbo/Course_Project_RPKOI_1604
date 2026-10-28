#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define POINT_COUNT 5
#define K 1
#define L 1.0

typedef struct {
    int time;
    int type;
} Incident;

/* Считает Q как логарифм отношения текущего значения к предыдущему. */
double calc_q(float current, float previous) {
    return log((double)current / previous);
}

/* Проверяет выход показателя за порог L. */
int is_alarm(double q) {
    return q < -L || q > L;
}

/* Считает Q одного компонента, печатает строки и отмечает тревоги по отсчётам. */
int check_component(const char *name, const float values[], const int times[],
                    int count, int alarms[]) {
    int pairs = 0;

    for (int i = K; i < count; i++) {
        double q = calc_q(values[i], values[i - K]);
        int alarm = is_alarm(q);

        alarms[times[i] - 1] = alarm;
        pairs++;
        printf("%s\t%d\t%.1f\t%d\t%.1f\t%.4f\t%s\n", name,
               times[i], values[i], times[i - K], values[i - K],
               q, alarm ? "ТРЕВОГА" : "НОРМА");
    }

    return pairs;
}

/* Возвращает текст состояния компонента. */
const char *state_name(int state) {
    if (state < 0) {
        return "нет Q";
    }
    if (state == 1) {
        return "ТРЕВОГА";
    }

    return "НОРМА";
}

/* Проверяет правило инцидента: все три компонента в тревоге. */
int is_incident(int web_state, int dbs_state, int iop_state) {
    return web_state == 1 && dbs_state == 1 && iop_state == 1;
}

int main(void) {
    float web[POINT_COUNT] = {41.9f, 48.3f, 48.8f, 49.3f};
    int web_times[POINT_COUNT] = {1, 3, 4, 5};
    float dbs[POINT_COUNT] = {57.4f, 55.4f, 53.4f, 51.4f, 54.8f};
    int dbs_times[POINT_COUNT] = {1, 2, 3, 4, 5};
    float iop[POINT_COUNT] = {30.2f, 29.5f, 34.1f, 32.6f};
    int iop_times[POINT_COUNT] = {1, 2, 3, 5};
    int web_state[POINT_COUNT] = {-1, -1, -1, -1, -1};
    int dbs_state[POINT_COUNT] = {-1, -1, -1, -1, -1};
    int iop_state[POINT_COUNT] = {-1, -1, -1, -1, -1};
    Incident incidents[POINT_COUNT];
    int incident_count = 0;
    int q_count = 0;

    system("chcp 65001 > nul");

    puts("Курсовой проект: контроль облачной инфраструктуры");
    puts("Вариант 25, этап 1: контрольный расчёт на пяти отсчётах");
    puts("Компоненты: WEB, DBS, IOP");
    printf("Q[i] = ln(X[i] / X[i-k]), k = %d, L = %.1f\n", K, L);
    printf("Диапазон нормы: [%.1f; %.1f]\n", -L, L);
    printf("Тестовый набор: 13 измерений, %d отсчётов\n\n", POINT_COUNT);

    puts("Таблица 1. Расчёт показателя Q по доступным парам измерений");
    puts("Компонент\tВремя i\tX[i]\tВремя i-k\tX[i-k]\tQ\tСостояние");
    q_count += check_component("WEB", web, web_times, 4, web_state);
    q_count += check_component("DBS", dbs, dbs_times, 5, dbs_state);
    q_count += check_component("IOP", iop, iop_times, 4, iop_state);

    puts("\nТаблица 2. Состояния компонентов по отсчётам");
    puts("Время\tWEB\tDBS\tIOP\tИнцидент");
    for (int t = 0; t < POINT_COUNT; t++) {
        int incident = is_incident(web_state[t], dbs_state[t], iop_state[t]);

        printf("%d\t%s\t%s\t%s\t%s\n", t + 1,
               state_name(web_state[t]), state_name(dbs_state[t]),
               state_name(iop_state[t]), incident ? "ДА" : "нет");

        if (incident) {
            incidents[incident_count].time = t + 1;
            incidents[incident_count].type = 1;
            incident_count++;
        }
    }

    puts("\nЖурнал инцидентов:");
    if (incident_count == 0) {
        puts("Инцидентов не обнаружено.");
    } else {
        for (int i = 0; i < incident_count; i++) {
            printf("Время %d, тип %d\n", incidents[i].time, incidents[i].type);
        }
    }

    printf("\nРассчитано значений Q: %d\n", q_count);

    system("pause");
    return 0;
}
