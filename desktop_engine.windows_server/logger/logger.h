#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>
#include <time.h>

typedef enum log_level {
    LOG_LEVEL_FATAL = 0,
    LOG_LEVEL_ERROR = 1,
    LOG_LEVEL_WARN = 2,
    LOG_LEVEL_INFO = 3,
    LOG_LEVEL_DEBUG = 4
} log_level;

/* Функционал
1. Конфигурационный файл логгера (Для permanent конфигураций логов)
2. Логирование в файл или в stdout runtime
3. Цвета логгера
4. Отслеживание процессов и времени (Дабы было понятно, где возникла ошибка: в ipc от определенного процесса, в самом сервере, в клиенте)
5. Форматирование
6. Асинхронное логирование
*/

void init_logging();
void log_message(log_level level, const char* text, ...);

#endif