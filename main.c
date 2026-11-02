/*
 * Сбоев Даниил Игоревич, группа бОТИ-261
 */

/*
 * Файл - программа контроля облачной инфраструктуры.
 * Программа рассчитывает показатель динамики Q для компонентов
 * WEB, DBS и IOP, определяет состояния и выявляет инциденты.
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define POINT_COUNT 5       /* число отсчётов */
#define K 1                 /* период сравнения */
#define L 1.0               /* порог показателя Q */
#define L_MIN (-L)          /* нижняя граница нормы */
#define L_MAX L             /* верхняя граница нормы */
#define SOURCE_COUNT 13     /* общее число измерений */
#define PAIR_COUNT 10       /* число рассчитанных пар */

#define STATE_NONE (-1)     /* состояние не определено */
#define STATE_NORMAL 0      /* состояние "норма" */
#define STATE_ALARM 1       /* состояние "тревога" */

#define INCIDENT_ALL_ALARM 1 /* тип инцидента: тревога всех компонентов */

/*
 * структура записи журнала инцидентов
 */
typedef struct {
	int time;   /* номер отсчёта */
	int type;   /* тип инцидента */
} Incident;

int check_pair(const char *pname, int t, float x, int t_prev,
	float x_prev);
int is_incident(int web_state, int dbs_state, int iop_state);
void print_point(int t, int web_state, int dbs_state, int iop_state);
void check_incident(int t, int web_state, int dbs_state, int iop_state,
	int *pincident_count);
const char *state_name(int state);

/*
 * Назначение: контрольный расчёт на тестовом наборе.
 * Возврат: 0 при успешном завершении.
 */
int main(void) {
	/* измерения: у WEB нет отсчёта 2, у IOP нет отсчёта 4 */
	float web1 = 41.9f, web3 = 48.3f, web4 = 48.8f, web5 = 49.3f;
	float dbs1 = 57.4f, dbs2 = 55.4f, dbs3 = 53.4f, dbs4 = 51.4f;
	float dbs5 = 54.8f;
	float iop1 = 30.2f, iop2 = 29.5f, iop3 = 34.1f, iop5 = 32.6f;

	/* состояния компонентов по отсчётам */
	int web_1 = STATE_NONE, web_2 = STATE_NONE, web_3 = STATE_NONE;
	int web_4 = STATE_NONE, web_5 = STATE_NONE;
	int dbs_1 = STATE_NONE, dbs_2 = STATE_NONE, dbs_3 = STATE_NONE;
	int dbs_4 = STATE_NONE, dbs_5 = STATE_NONE;
	int iop_1 = STATE_NONE, iop_2 = STATE_NONE, iop_3 = STATE_NONE;
	int iop_4 = STATE_NONE, iop_5 = STATE_NONE;

	/* число найденных инцидентов */
	int incident_count = 0;

	system("chcp 65001 > nul");

	puts("Программа контроля облачной инфраструктуры");
	puts("Назначение: расчёт показателя динамики и выявление инцидентов");
	puts("Контрольный расчёт на пяти тестовых отсчётах");
	puts("Компоненты: WEB, DBS, IOP");
	printf("Q[i] = ln(X[i] / X[i-k]), k = %d, L = %.1f\n", K, L);
	printf("Диапазон нормы: [%.1f; %.1f]\n", L_MIN, L_MAX);
	printf("Тестовый набор: %d измерений, %d отсчётов\n\n",
		SOURCE_COUNT, POINT_COUNT);

	puts("Таблица 1. Расчёт показателя Q по доступным парам измерений");
	puts("Компонент Время i X[i] Время i-k X[i-k] Q       Состояние");

	/* расчёт показателя по доступным парам измерений */
	web_3 = check_pair("WEB", 3, web3, 1, web1);
	web_4 = check_pair("WEB", 4, web4, 3, web3);
	web_5 = check_pair("WEB", 5, web5, 4, web4);
	dbs_2 = check_pair("DBS", 2, dbs2, 1, dbs1);
	dbs_3 = check_pair("DBS", 3, dbs3, 2, dbs2);
	dbs_4 = check_pair("DBS", 4, dbs4, 3, dbs3);
	dbs_5 = check_pair("DBS", 5, dbs5, 4, dbs4);
	iop_2 = check_pair("IOP", 2, iop2, 1, iop1);
	iop_3 = check_pair("IOP", 3, iop3, 2, iop2);
	iop_5 = check_pair("IOP", 5, iop5, 3, iop3);

	puts("\nТаблица 2. Состояния компонентов по отсчётам");
	puts("Время\tWEB\tDBS\tIOP\tИнцидент");
	print_point(1, web_1, dbs_1, iop_1);
	print_point(2, web_2, dbs_2, iop_2);
	print_point(3, web_3, dbs_3, iop_3);
	print_point(4, web_4, dbs_4, iop_4);
	print_point(5, web_5, dbs_5, iop_5);

	puts("\nЖурнал инцидентов:");
	check_incident(1, web_1, dbs_1, iop_1, &incident_count);
	check_incident(2, web_2, dbs_2, iop_2, &incident_count);
	check_incident(3, web_3, dbs_3, iop_3, &incident_count);
	check_incident(4, web_4, dbs_4, iop_4, &incident_count);
	check_incident(5, web_5, dbs_5, iop_5, &incident_count);

	if (incident_count == 0) {
		puts("Инцидентов не обнаружено.");
	}

	printf("\nРассчитано значений Q: %d\n", PAIR_COUNT);

	system("pause");
	return 0;
}

/*
 * Назначение: расчёт показателя Q для пары измерений и вывод
 * строки таблицы 1.
 * Параметры: pname - имя компонента;
 *            t, x - текущее измерение;
 *            t_prev, x_prev - предыдущее измерение.
 * Возврат: состояние компонента на текущем отсчёте.
 */
int check_pair(const char *pname, int t, float x, int t_prev,
	float x_prev) {
	double q;
	int state;

	q = log((double)x / x_prev);

	if (q < L_MIN || q > L_MAX) {
		state = STATE_ALARM;
	} else {
		state = STATE_NORMAL;
	}

	printf("%-9s %-7d %-4.1f %-9d %-6.1f %-7.4f %s\n",
		pname, t, x, t_prev, x_prev, q, state_name(state));

	return state;
}

/*
 * Назначение: проверка правила инцидента.
 * Параметры: web_state, dbs_state, iop_state - состояния компонентов.
 * Возврат: 1 при тревоге всех компонентов, иначе 0.
 */
int is_incident(int web_state, int dbs_state, int iop_state) {
	return web_state == STATE_ALARM && dbs_state == STATE_ALARM
		&& iop_state == STATE_ALARM;
}

/*
 * Назначение: вывод строки таблицы 2 для одного отсчёта.
 * Параметры: t - номер отсчёта;
 *            web_state, dbs_state, iop_state - состояния компонентов.
 */
void print_point(int t, int web_state, int dbs_state, int iop_state) {
	printf("%d\t%s\t%s\t%s\t%s\n", t, state_name(web_state),
		state_name(dbs_state), state_name(iop_state),
		is_incident(web_state, dbs_state, iop_state) ? "ДА" : "нет");
}

/*
 * Назначение: проверка инцидента на отсчёте и вывод записи журнала.
 * Параметры: t - номер отсчёта;
 *            web_state, dbs_state, iop_state - состояния компонентов;
 *            pincident_count - счётчик найденных инцидентов.
 */
void check_incident(int t, int web_state, int dbs_state, int iop_state,
	int *pincident_count) {
	Incident incident;

	if (!is_incident(web_state, dbs_state, iop_state)) {
		return;
	}

	incident.time = t;
	incident.type = INCIDENT_ALL_ALARM;
	printf("Время %d, тип %d\n", incident.time, incident.type);
	(*pincident_count)++;
}

/*
 * Назначение: текстовое название состояния компонента.
 * Параметр: state - код состояния STATE_NONE, STATE_NORMAL
 *           или STATE_ALARM.
 * Возврат: указатель на строку с названием состояния.
 */
const char *state_name(int state) {
	if (state == STATE_NONE) {
		return "нет Q";
	}
	if (state == STATE_ALARM) {
		return "ТРЕВОГА";
	}
	return "НОРМА";
}
