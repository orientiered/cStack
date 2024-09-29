/// @file Logger
#ifndef LOGGER_H
#define LOGGER_H

/*------------------LOGGER----------------------------------------------------*/
/*------------------WITH DYNAMIC LOG LEVEL TO DEBUG NECESSARY CODE------------*/
/*------------------orientiered MIPT 2024-------------------------------------*/

enum LogLevel {
    L_ZERO,     ///< Essential information
    L_DEBUG,    ///< Debug information
    L_EXTRA     ///< Debug++
};

/// @brief Open log file
enum status logOpen();

/// @brief Close log file
enum status logClose();

/// @brief Set log level
void setLogLevel(enum LogLevel level);

/// @brief Print in log file with time signature
enum status logPrintWithTime(enum LogLevel level, bool copyToStderr, const char* fmt, ...);

/// @brief Print in log file
enum status logPrint(enum LogLevel level, bool copyToStderr, const char* fmt, ...);

/// @brief Print in log file with place in code
#define LOG_PRINT(level, ...)                                                                       \
    do {                                                                                            \
        logPrint(level, 0, "[DEBUG] %s:%d : %s \n", __FILE__, __LINE__, __PRETTY_FUNCTION__);       \
        logPrint(level, __VA_ARGS__);                                                               \
    } while(0)


#endif

