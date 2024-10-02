/// @file
/// @brief Easily parse cmd args

#ifndef ARGV_PROCESSOR_H
#define ARGV_PROCESSOR_H

/*------------------STRUCTS DEFINITIONS---------------------------------------*/

const size_t MAX_REGISTERED_FLAGS = 50;   ///< maximum amount of flags

/// @brief Available types for cmd args
enum flagType {
    TYPE_BLANK = 0,     ///< Doesn't expect next argument
    TYPE_INT,           ///< Next argument is integer
    TYPE_FLOAT,         ///< Next argument is double
    TYPE_STRING,        ///< Next argument is string(char*)
};

/// @brief Describe cmd argument
typedef struct flagDescriptor {
    enum flagType type;          ///< Expected type of data
    const char *flagShortName;   ///< Short name (-e)
    const char *flagFullName;    ///< Full name (--encode)
    const char *flagHelp;        ///< Message to print in help call
} flagDescriptor_t;

/// @brief Universal type for flag values
typedef union {
    int int_;
    double float_;
    char *string_;
} fVal_t;

/// @brief Full flag information
typedef struct flagVal {
    flagDescriptor_t desc;          ///< Flag description
    fVal_t val;                     ///< Flag value
} flagVal_t;

/// @brief Store flags
typedef struct FlagsHolder {
    flagVal_t *flags;       ///< Array with flags
    size_t size;            ///< Size of flags array
    size_t reserved;        ///< Maximum number of flags
} FlagsHolder_t;

/*------------------FUNCTIONS TO PARSE CMD ARGUMENTS--------------------------*/


/// @brief Set header of help message
enum status setHelpMessageHeader(const char* header);

/*!
    @brief register cmd argument flag
*/
enum status registerFlag(enum flagType type,
                         const char* shortName,
                         const char* fullName,
                         const char* helpMessage);
/*!
    @brief Parse cmd args

    @return SUCCESS if parsed correctly, ERROR otherwise
*/
enum status processArgs(int argc, const char *argv[]);

/*!
    @brief Prints help message containing descriptions of all flags

    Prints all argHelps in args array
*/
void printHelpMessage();

/// @brief Check if flag with given name is set
bool isFlagSet(const char *flagName);

/// @brief Get value of flag with given name
fVal_t getFlagValue(const char *flagName);

/// @brief Delete flags
void deleteFlags();

#endif
