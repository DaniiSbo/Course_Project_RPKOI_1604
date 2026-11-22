/*
 * Сбоев Даниил Игоревич, группа бОТИ-261
 */

/*
 * Файл - программа контроля облачной инфраструктуры (этап 2).
 * Программа хранит измерения компонентов WEB, DBS и IOP в одномерных
 * массивах, рассчитывает показатель Q[i] = ln(X[i] / X[i-k]) для
 * заданного k, обрабатывает ошибки, выявляет инциденты и имеет меню.
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define POINT_COUNT 5           /* число отсчётов */
#define MAX_MEASURE 5           /* максимум измерений компонента */
#define NAME_LEN 4              /* длина кода компонента */
#define WEB_COUNT 4             /* число измерений WEB в наборе 1 */
#define DBS_COUNT 5             /* число измерений DBS в наборе 1 */
#define IOP_COUNT 4             /* число измерений IOP в наборе 1 */
#define TEST_COUNT 5            /* число измерений в наборе 2 */
#define DEFAULT_K 1             /* период сравнения по умолчанию */
#define MAX_K (MAX_MEASURE - 1) /* максимальный период сравнения */
#define DEFAULT_L 1.0           /* порог показателя Q по умолчанию */
#define EPS 1e-9                /* точность проверки нуля */

#define STATE_NONE (-1)         /* состояние не определено */
#define STATE_NORMAL 0          /* состояние "норма" */
#define STATE_ALARM 1           /* состояние "тревога" */
#define STATE_ERROR 2           /* ошибка вычисления показателя */

#define CALC_SKIP (-1)          /* пропуск: нет пары для расчёта */
#define CALC_OK 0               /* расчёт выполнен успешно */
#define CALC_DIV_ZERO 1         /* ошибка: деление на ноль */
#define CALC_BAD_LOG 2          /* ошибка: аргумент ln <= 0 */

#define INCIDENT_ALL_ALARM 1    /* тип инцидента: тревога всех трёх */
#define DATASET_CONTROL 1       /* основной контрольный набор */
#define DATASET_ERRORS 2        /* тестовый набор с ошибками */

#define MENU_EXIT 0             /* пункт меню: выход */
#define MENU_RUN 1              /* пункт меню: выполнить расчёт */
#define MENU_DATASET 2          /* пункт меню: выбрать набор данных */
#define MENU_SET_K 3            /* пункт меню: задать период k */
#define MENU_SET_L 4            /* пункт меню: задать порог L */
#define MENU_RESET 5            /* пункт меню: сброс настроек */

/*
 * структура записи журнала инцидентов
 */
typedef struct {
	int time;               /* номер отсчёта */
	int type;               /* тип инцидента */
} Incident;

/*
 * структура данных и результатов одного компонента
 */
typedef struct {
	char name[NAME_LEN];    /* код компонента */
	float values[MAX_MEASURE]; /* измеренные значения */
	int times[MAX_MEASURE]; /* номера отсчётов */
	int count;              /* число измерений */
	double q_vals[POINT_COUNT]; /* значения показателя Q */
	int codes[POINT_COUNT]; /* коды результата расчёта */
	int states[POINT_COUNT]; /* состояния по отсчётам */
} Component;

int calc_q(float x_curr, float x_prev, double *pq);
int check_component(Component *pcomp, int k, double limit);
int is_incident(int web_state, int dbs_state, int iop_state);
int find_incidents(const Component *pweb, const Component *pdbs,
	const Component *piop, Incident incidents[]);
int print_component_rows(const Component *pcomp, int k);
void print_results(const Component *pweb, const Component *pdbs,
	const Component *piop, const Incident incidents[], int inc_count,
	int q_count, int k, double limit);
void run_analysis(int dataset, int k, double limit);
const char *state_name(int state);
int read_int_range(const char *pprompt, int min_val, int max_val);
double read_positive_double(const char *pprompt);

/*
 * Назначение: точка входа, меню настройки режима и запуска анализа.
 * Возврат: 0 при успешном завершении.
 */
int main(void) {
	int k = DEFAULT_K;              /* текущий период сравнения */
	double limit = DEFAULT_L;       /* текущий порог нормы */
	int dataset = DATASET_CONTROL;  /* выбранный набор данных */
	int choice;                     /* пункт меню */

	system("chcp 65001 > nul");

	puts("Программа контроля облачной инфраструктуры");
	puts("Назначение: расчёт показателя Q и выявление инцидентов");
	puts("Вариант 25: компоненты WEB, DBS, IOP, Q[i] = ln(X[i] / X[i-k])");
	puts("");

	do {
		printf("Режим: набор %d (%s), k = %d, L = %.2f\n",
			dataset,
			dataset == DATASET_CONTROL ? "вариант 25" : "тест ошибок",
			k, limit);
		puts("1 - Выполнить расчёт");
		puts("2 - Выбрать набор данных (1 - вариант 25, 2 - тест ошибок)");
		puts("3 - Задать период сравнения k");
		puts("4 - Задать порог нормы L");
		puts("5 - Сбросить настройки по умолчанию");
		puts("0 - Выход");

		choice = read_int_range("Выберите действие: ", MENU_EXIT,
			MENU_RESET);

		switch (choice) {
		case MENU_RUN:
			/* запуск анализа с текущими настройками */
			run_analysis(dataset, k, limit);
			break;
		case MENU_DATASET:
			/* переключение набора данных */
			dataset = read_int_range(
				"Номер набора (1 или 2): ",
				DATASET_CONTROL, DATASET_ERRORS);
			break;
		case MENU_SET_K:
			/* изменение периода сравнения k */
			k = read_int_range(
				"Введите период k (от 1 до 4): ",
				DEFAULT_K, MAX_K);
			break;
		case MENU_SET_L:
			/* изменение порога L */
			limit = read_positive_double(
				"Введите положительный порог L: ");
			break;
		case MENU_RESET:
			/* возврат к параметрам варианта 25 */
			k = DEFAULT_K;
			limit = DEFAULT_L;
			dataset = DATASET_CONTROL;
			puts("Настройки сброшены.");
			break;
		case MENU_EXIT:
			/* завершение программы */
			puts("Работа программы завершена.");
			break;
		default:
			/* защита от неизвестного пункта */
			puts("Неверный пункт меню.");
			break;
		}
		if (choice != MENU_EXIT) {
			puts("");
		}
	} while (choice != MENU_EXIT);

	system("pause");
	return 0;
}

/*
 * Назначение: вычисление Q = ln(x_curr / x_prev) с проверкой ошибок.
 * Параметры: x_curr - текущее значение X[i];
 *            x_prev - предыдущее значение X[i-k];
 *            pq - указатель для записи результата.
 * Возврат: код CALC_OK, CALC_DIV_ZERO или CALC_BAD_LOG.
 */
int calc_q(float x_curr, float x_prev, double *pq) {
	double ratio;   /* отношение X[i] к X[i-k] */

	if (fabs((double)x_prev) < EPS) {
		return CALC_DIV_ZERO;
	}

	ratio = (double)x_curr / x_prev;

	if (ratio <= 0.0) {
		return CALC_BAD_LOG;
	}

	*pq = log(ratio);
	return CALC_OK;
}

/*
 * Назначение: расчёт показателя Q и состояний одного компонента.
 * Параметры: pcomp - указатель на структуру компонента;
 *            k - период сравнения;
 *            limit - порог L диапазона нормы.
 * Возврат: число успешно рассчитанных значений Q.
 */
int check_component(Component *pcomp, int k, double limit) {
	int i;          /* индекс измерения и отсчёта */
	int pos;        /* индекс отсчёта в массиве результатов */
	int code;       /* код завершения calc_q */
	int ok_cnt = 0; /* число успешных расчётов */
	double q = 0.0; /* вычисленное значение Q */

	for (i = 0; i < POINT_COUNT; i++) {
		pcomp->q_vals[i] = 0.0;
		pcomp->codes[i] = CALC_SKIP;
		pcomp->states[i] = STATE_NONE;
	}

	for (i = k; i < pcomp->count; i++) {
		pos = pcomp->times[i] - 1;
		if (pos < 0 || pos >= POINT_COUNT) {
			continue;
		}

		code = calc_q(pcomp->values[i], pcomp->values[i - k], &q);
		pcomp->codes[pos] = code;

		if (code == CALC_OK) {
			pcomp->q_vals[pos] = q;
			if (q < -limit || q > limit) {
				pcomp->states[pos] = STATE_ALARM;
			} else {
				pcomp->states[pos] = STATE_NORMAL;
			}
			ok_cnt++;
		} else {
			pcomp->states[pos] = STATE_ERROR;
		}
	}

	return ok_cnt;
}

/*
 * Назначение: проверка правила инцидента для одного отсчёта.
 * Параметры: web_state, dbs_state, iop_state - состояния компонентов.
 * Возврат: 1, если все три компонента в состоянии тревоги, иначе 0.
 */
int is_incident(int web_state, int dbs_state, int iop_state) {
	return web_state == STATE_ALARM && dbs_state == STATE_ALARM
		&& iop_state == STATE_ALARM;
}

/*
 * Назначение: поиск инцидентов по всем отсчётам и запись в журнал.
 * Параметры: pweb, pdbs, piop - данные трёх компонентов;
 *            incidents - массив записей инцидентов.
 * Возврат: количество обнаруженных инцидентов.
 */
int find_incidents(const Component *pweb, const Component *pdbs,
	const Component *piop, Incident incidents[]) {
	int t;          /* индекс отсчёта */
	int count = 0;  /* счётчик инцидентов */

	for (t = 0; t < POINT_COUNT; t++) {
		if (is_incident(pweb->states[t], pdbs->states[t],
			piop->states[t])) {
			incidents[count].time = t + 1;
			incidents[count].type = INCIDENT_ALL_ALARM;
			count++;
		}
	}

	return count;
}

/*
 * Назначение: печать строк таблицы 1 для одного компонента.
 * Параметры: pcomp - указатель на данные компонента;
 *            k - период сравнения.
 * Возврат: число строк с ошибками вычислений.
 */
int print_component_rows(const Component *pcomp, int k) {
	int i;          /* индекс измерения */
	int pos;        /* индекс отсчёта */
	int errs = 0;   /* счётчик ошибок */

	for (i = 0; i < pcomp->count; i++) {
		pos = pcomp->times[i] - 1;
		printf("%-9s %-7d %-5.1f ", pcomp->name,
			pcomp->times[i], pcomp->values[i]);
		if (i < k) {
			printf("%-9s %-6s нет Q    %s\n", "-", "-",
				"пропуск (первые k)");
		} else if (pcomp->codes[pos] == CALC_OK) {
			printf("%-9d %-6.1f %-8.4f %s\n",
				pcomp->times[i - k], pcomp->values[i - k],
				pcomp->q_vals[pos],
				state_name(pcomp->states[pos]));
		} else {
			printf("%-9d %-6.1f ОШИБКА   %s\n",
				pcomp->times[i - k], pcomp->values[i - k],
				pcomp->codes[pos] == CALC_DIV_ZERO
					? "деление на 0" : "аргумент ln <= 0");
			errs++;
		}
	}

	return errs;
}

/*
 * Назначение: вывод таблиц расчёта, состояний и журнала инцидентов.
 * Параметры: pweb, pdbs, piop - результаты по компонентам;
 *            incidents - массив инцидентов;
 *            inc_count - число инцидентов;
 *            q_count - число рассчитанных Q;
 *            k - период сравнения;
 *            limit - порог L.
 */
void print_results(const Component *pweb, const Component *pdbs,
	const Component *piop, const Incident incidents[], int inc_count,
	int q_count, int k, double limit) {
	int t;          /* индекс отсчёта или инцидента */
	int errs = 0;   /* общее число ошибок вычислений */

	printf("\nПараметры: k = %d, L = %.2f, норма: [%.2f; %.2f]\n",
		k, limit, -limit, limit);
	puts("\nТаблица 1. Расчёт показателя Q по измерениям");
	puts("Компонент Время i X[i]  Время i-k X[i-k] Q        Состояние");

	errs += print_component_rows(pweb, k);
	errs += print_component_rows(pdbs, k);
	errs += print_component_rows(piop, k);

	puts("\nТаблица 2. Состояния компонентов по отсчётам");
	puts("Время\tWEB\tDBS\tIOP\tИнцидент");

	for (t = 0; t < POINT_COUNT; t++) {
		printf("%d\t%s\t%s\t%s\t%s\n", t + 1,
			state_name(pweb->states[t]),
			state_name(pdbs->states[t]),
			state_name(piop->states[t]),
			is_incident(pweb->states[t], pdbs->states[t],
				piop->states[t]) ? "ДА" : "нет");
	}

	puts("\nЖурнал инцидентов:");
	if (inc_count == 0) {
		puts("Инцидентов не обнаружено.");
	} else {
		for (t = 0; t < inc_count; t++) {
			printf("Время %d, тип %d\n", incidents[t].time,
				incidents[t].type);
		}
	}

	printf("\nРассчитано значений Q: %d, ошибок вычислений: %d\n",
		q_count, errs);
}

/*
 * Назначение: подготовка выбранного набора данных и запуск анализа.
 * Параметры: dataset - номер набора данных (1 или 2);
 *            k - период сравнения;
 *            limit - порог L.
 */
void run_analysis(int dataset, int k, double limit) {
	Component ctrl_web = {"WEB", {41.9f, 48.3f, 48.8f, 49.3f},
		{1, 3, 4, 5}, WEB_COUNT, {0}, {0}, {0}};
	Component ctrl_dbs = {"DBS", {57.4f, 55.4f, 53.4f, 51.4f, 54.8f},
		{1, 2, 3, 4, 5}, DBS_COUNT, {0}, {0}, {0}};
	Component ctrl_iop = {"IOP", {30.2f, 29.5f, 34.1f, 32.6f},
		{1, 2, 3, 5}, IOP_COUNT, {0}, {0}, {0}};
	Component err_web = {"WEB", {10.0f, 30.0f, 0.0f, 15.0f, -5.0f},
		{1, 2, 3, 4, 5}, TEST_COUNT, {0}, {0}, {0}};
	Component err_dbs = {"DBS", {40.0f, 10.0f, 12.0f, 12.5f, 0.0f},
		{1, 2, 3, 4, 5}, TEST_COUNT, {0}, {0}, {0}};
	Component err_iop = {"IOP", {5.0f, 20.0f, 21.0f, 19.0f, 18.0f},
		{1, 2, 3, 4, 5}, TEST_COUNT, {0}, {0}, {0}};
	Component web;                   /* рабочий ряд WEB */
	Component dbs;                   /* рабочий ряд DBS */
	Component iop;                   /* рабочий ряд IOP */
	Incident incidents[POINT_COUNT]; /* журнал инцидентов */
	int q_count = 0;                 /* число рассчитанных Q */
	int inc_count;                   /* число инцидентов */

	if (dataset == DATASET_CONTROL) {
		web = ctrl_web;
		dbs = ctrl_dbs;
		iop = ctrl_iop;
	} else {
		web = err_web;
		dbs = err_dbs;
		iop = err_iop;
	}

	q_count += check_component(&web, k, limit);
	q_count += check_component(&dbs, k, limit);
	q_count += check_component(&iop, k, limit);

	inc_count = find_incidents(&web, &dbs, &iop, incidents);
	print_results(&web, &dbs, &iop, incidents, inc_count, q_count,
		k, limit);
}

/*
 * Назначение: текстовое название состояния компонента.
 * Параметр: state - код состояния.
 * Возврат: указатель на строку с названием состояния.
 */
const char *state_name(int state) {
	if (state == STATE_NONE) {
		return "нет Q";
	}
	if (state == STATE_ALARM) {
		return "ТРЕВОГА";
	}
	if (state == STATE_ERROR) {
		return "ОШИБКА";
	}
	return "НОРМА";
}

/*
 * Назначение: безопасный ввод целого числа в заданном диапазоне.
 * Параметры: pprompt - строка приглашения ко вводу;
 *            min_val, max_val - границы допустимого диапазона.
 * Возврат: корректно введённое целое число.
 */
int read_int_range(const char *pprompt, int min_val, int max_val) {
	int val;        /* введённое значение */
	int ok = 0;     /* флаг успешного ввода */
	int ch;         /* символ для очистки буфера */

	while (!ok) {
		printf("%s", pprompt);
		if (scanf("%d", &val) == 1 && val >= min_val
			&& val <= max_val) {
			ok = 1;
		} else {
			printf("Ошибка: введите целое число от %d до %d.\n",
				min_val, max_val);
		}
		while ((ch = getchar()) != '\n' && ch != EOF) {
		}
	}

	return val;
}

/*
 * Назначение: безопасный ввод положительного вещественного числа.
 * Параметр: pprompt - строка приглашения ко вводу.
 * Возврат: введённое число больше нуля.
 */
double read_positive_double(const char *pprompt) {
	double val;     /* введённое значение */
	int ok = 0;     /* флаг успешного ввода */
	int ch;         /* символ для очистки буфера */

	while (!ok) {
		printf("%s", pprompt);
		if (scanf("%lf", &val) == 1 && val > 0.0) {
			ok = 1;
		} else {
			puts("Ошибка: введите число больше нуля.");
		}
		while ((ch = getchar()) != '\n' && ch != EOF) {
		}
	}

	return val;
}
