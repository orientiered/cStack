#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdarg.h>

#include "error_debug.h"
#include "logger.h"

static FILE *logFile = NULL;
static const char *logFileName = "log.txt";
static enum LogLevel globalLogLevel = L_ZERO;


static struct tm getTime();
static void logTime();

static struct tm getTime() {
    time_t currentTime = time(NULL);
    struct tm result = *localtime(&currentTime);
    return result;
}

static void logTime() {
    MY_ASSERT(logFile, abort());
    struct tm currentTime = getTime();
    fprintf(logFile, "[%.2d.%.2d.%d %.2d:%.2d:%.2d] ",
        currentTime.tm_mday, currentTime.tm_mon, currentTime.tm_year + 1900,
        currentTime.tm_hour, currentTime.tm_min, currentTime.tm_sec);
}

enum status logOpen() {
    logFile = fopen(logFileName, "a");
    if (!logFile) return ERROR;
    setbuf(logFile, NULL); //disabling buffering

    fprintf(logFile, "------------------------------------------\n");
    logTime();
    fprintf(logFile, "Starting logging session\n");
    return SUCCESS;
}

enum status logClose() {
    if (!logFile) return ERROR;


    logTime();
    fprintf(logFile, "Ending logging session \n");
    fprintf(logFile, "-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*\n");

    fclose(logFile);

    return SUCCESS;
}

void setLogLevel(enum LogLevel level) {
    globalLogLevel = level;
}

enum status logPrintWithTime(enum LogLevel level, bool copyToStderr, const char* fmt, ...) {
    MY_ASSERT(logFile, abort());
    if (level > globalLogLevel)
        return SUCCESS;

    va_list args;

    if (copyToStderr) {
        va_start(args, fmt);
        vfprintf(stderr, fmt, args);
    }
    va_start(args, fmt);
    logTime();
    vfprintf(logFile, fmt, args);

    va_end(args);
    return SUCCESS;
}

enum status logPrint(enum LogLevel level, bool copyToStderr, const char* fmt, ...) {
    MY_ASSERT(logFile, abort());
    if (level > globalLogLevel)
        return SUCCESS;

    va_list args;

    if (copyToStderr) {
        va_start(args, fmt);
        vfprintf(stderr, fmt, args);
    }
    va_start(args, fmt);
    vfprintf(logFile, fmt, args);

    va_end(args);
    return SUCCESS;
}

