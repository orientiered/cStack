/// @file
/// @brief header with structs and functions to easily parse cmd args

#ifndef ARGV_PROCESSOR_H
#define ARGV_PROCESSOR_H

const size_t FLAGS_RESERVED = 10;
/// @brief Enum with available types for cmd args
enum flagType {
    TYPE_BLANK = 0,     ///< Doesn't expect next argument
    TYPE_INT,           ///< Next argument is integer
    TYPE_FLOAT,         ///< Next argument is double
    TYPE_STRING,        ///< Next argument is string(char*)
};


/// @brief Struct to describe cmd arguments
typedef struct flagDescriptor {
    enum flagType type;          ///< Expected type of data
    const char *flagShortName;   ///< Short name (-e)
    const char *flagFullName;    ///< Full name (--encode)
    const char *flagHelp;        ///< Message to print in help call
} flagDescriptor_t;

typedef union {
    int int_;
    double float_;
    char *string_;
} fVal_t;

/// @brief Struct with argument parameters
typedef struct flagVal {
    flagDescriptor_t desc;          ///< Flag description
    fVal_t val;
} flagVal_t;

typedef struct FlagsHolder {
    flagVal_t *flags;
    size_t size;
    size_t reserved;
} FlagsHolder_t;

typedef struct FlagDescHolder {
    flagDescriptor_t *args;
    size_t size;
} FlagDescHolder_t;

/*!
    @brief Parse cmd args

    @return SUCCESS if parsed correctly, ERROR otherwise

*/
enum status processArgs(FlagDescHolder_t desc, FlagsHolder_t *flags, int argc, const char *argv[]);

/*!
    @brief Prints help message containing descriptions of all flags

    Prints all argHelps in args array
*/
void printHelpMessage(FlagDescHolder_t desc);

bool isFlagSet(const FlagsHolder_t flags, const char *flagName);

fVal_t getFlagValue(const FlagsHolder_t flags, const char *flagName);

void deleteFlags(FlagsHolder_t *flags);

#endif
