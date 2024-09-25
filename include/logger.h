#ifndef LOGGER_H
#define LOGGER_H

enum LogLevel {
    L_ZERO,
    L_DEBUG,
    L_EXTRA
};

enum status logOpen();

enum status logClose();

void setLogLevel(enum LogLevel level);

enum status logPrint(enum LogLevel level, bool copyToStderr, const char* fmt, ...);

#define LOG_PRINT(level, ...)                                                                       \
    do {                                                                                            \
        logPrint(level, 0, "[DEBUG] %s : %s : %d \n", __FILE__, __PRETTY_FUNCTION__, __LINE__);     \
        logPrint(level, __VA_ARGS__);                                                               \
    } while(0)


#endif

