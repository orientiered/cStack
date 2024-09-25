#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
//#define DEBUG_PRINTS
#include "mystring.h"
#include "error_debug.h"
#include "logger.h"
#include "argvProcessor.h"
#include "utils.h"

/*!
    @brief Scans argument in full form (--encode)

    @param flags [out] Array with flags
    @param remainToScan [in] Number of arguments that were'nt already scanned
    @param argv [in] Current argv position

    @return Number of arguments remained to scan. Can return int < 0, is something goes wrong

    Counts current argument as processed only after next argument was processed by scanToFlag() function
*/
static int scanFullArgument(FlagDescHolder_t desc, FlagsHolder_t *flags, int remainToScan, const char *argv[]);

/*!
    @brief Scans argument in short form (-eio)

    @param flags [out] Array with flags
    @param remainToScan [in] Number of arguments that weren't already scanned
    @param argv [in] Current argv position

    @return Number of arguments remained to scan. Can return int < 0, is something goes wrong

    Counts current argument as processed only after all next argument were processed by scanToFlag() function

*/
static int scanShortArguments(FlagDescHolder_t desc, FlagsHolder_t *flags, int remainToScan, const char *argv[]);

/*!
    @brief Scan value to flag

    @param flag [out] Flag to write value
    @param remainToScan Number of arguments that weren't scanned
    @param argv [in] Current argv position

    @return Number of arguments remained to scan. Can return int < 0, is something goes wrong

    Decreases remainToScan by number of elements it processed (typically 0 or 1)

    argv must point to value, that should be scanned to flag
*/
static int scanToFlag(flagDescriptor_t desc, FlagsHolder_t *flags, int remainToScan, const char *argv[]);

static flagVal_t *findFlag(FlagsHolder_t flags, const char *flagName);
static enum status addFlag(FlagsHolder_t *flags, flagDescriptor desc, fVal_t val);

enum status processArgs(FlagDescHolder_t desc, FlagsHolder_t *flags, int argc, const char *argv[]) {
    flags->flags = (flagVal_t*) calloc (FLAGS_RESERVED, sizeof(flagVal_t));
    if (!flags->flags) {
        LOG_PRINT(L_DEBUG, 0, "Memory allocation failed\n");
        return ERROR;
    }
    flags->reserved = FLAGS_RESERVED;

    for (int i = 1; i < argc;) {
        if (argv[i][0] != '-')  {   //all arguments start with -
            i++;                    //parameters of args are skipped inside scan...Argument() functions
            continue;
        }

        int remainToScan = 0;
        if (argv[i][1] == '-') //-abcd or --argument
            remainToScan = scanFullArgument(desc, flags, argc-i, argv+i);
        else
            remainToScan = scanShortArguments(desc, flags, argc-i, argv+i);

        if (remainToScan < 0) { //remainToScan < 0 is universal error code
            deleteFlags(flags);
            logPrint(L_ZERO, 1, "Wrong flags format\n");
            return ERROR;
        }
        i  = argc - remainToScan; //moving to next arguments
    }

    char *argvConcatenated = joinStrings(argv, argc, " ");
    logPrint(L_DEBUG, 0, "%s\n", argvConcatenated);
    free(argvConcatenated);
    return SUCCESS;
}

static int scanFullArgument(FlagDescHolder_t desc, FlagsHolder_t *flags, int remainToScan, const char *argv[]) {
    for (size_t flagIndex = 0; flagIndex < desc.size; flagIndex++) {        //just iterating over all flags
        if (strcmp(argv[0], desc.args[flagIndex].flagFullName) != 0) continue;
        return scanToFlag(desc.args[flagIndex], flags, remainToScan, argv + 1) - 1;   //we pass remainToScan forward
    }                                                                   //but scanToFlag reads flag argument, so argv+1
    return -1;                                                          //-1 because we read argv flag
}

static int scanShortArguments(FlagDescHolder_t desc, FlagsHolder_t *flags, int remainToScan, const char *argv[]) {
    for (const char *shortName = argv[0]+1; (*shortName != '\0') && (remainToScan > 0); shortName++) { //iterating over short flags string
        bool scannedArg = false;
        for (size_t flagIndex = 0; flagIndex < desc.size; flagIndex++) {
            if (*shortName != desc.args[flagIndex].flagShortName[1]) continue;
            scannedArg = true;

            int newRemainToScan = scanToFlag(desc.args[flagIndex], flags, remainToScan, argv+1); //scanning flag param
            argv += remainToScan - newRemainToScan; //moving argv
            if (newRemainToScan < 0) return newRemainToScan; //checking for error
            remainToScan = newRemainToScan;
        }
        if (!scannedArg) return -1;
    }
    return remainToScan-1; //scanned current argv -> -1
}

static int scanToFlag(flagDescriptor_t desc, FlagsHolder_t *flags, int remainToScan, const char *argv[]) {
    fVal_t val = {};

    if (desc.type != TYPE_BLANK) {
        if (--remainToScan <= 0) {
            logPrint(L_ZERO, 1, "Expected to get parameter for flag %s, but failed\n", desc.flagFullName);
            return remainToScan;
        }
        switch(desc.type) {
        case TYPE_INT:
            sscanf(argv[0], "%d", &val.int_);
            break;
        case TYPE_FLOAT:
            sscanf(argv[0], "%lf", &val.float_);
            break;
        case TYPE_STRING:
            {
            size_t len = strlen(argv[0]);
            val.string_ = (char *) calloc(len + 1, sizeof(char));
            sscanf(argv[0], "%s", val.string_);
            break;
            }
        default:
            MY_ASSERT(0, fprintf(stderr, "Logic error, unknown flag type"); abort(););
            break;
        }
    }

    if (addFlag(flags, desc, val) != SUCCESS) {
        if (desc.type == TYPE_STRING)
            FREE(val.string_);
        return -1;
    }
    return remainToScan;
}

void printHelpMessage(FlagDescHolder_t desc) {           //building help message from flags descriptions
    printf("Sort strings in file\n");
    printf("Example: ./main -io in.txt out.txt\n");
    printf("You can concatenate short version of flags\n");
    printf("Available flags:\n");
    for (size_t i = 0; i < desc.size; i++) {
        printf("%5s,%10s %s\n", desc.args[i].flagShortName, desc.args[i].flagFullName, desc.args[i].flagHelp);
    }
    printf("orientiered, MIPT 2024\n");
}

static flagVal_t *findFlag(FlagsHolder_t flags, const char *flagName) {
    MY_ASSERT(flagName, abort());
    for (size_t flagIndex = 0; flagIndex < flags.size; flagIndex++) {
        if ((strcmp(flagName, flags.flags[flagIndex].desc.flagShortName) == 0) ||
            (strcmp(flagName, flags.flags[flagIndex].desc.flagFullName) == 0))
            return &(flags.flags[flagIndex]);
    }
    return NULL;
}

bool isFlagSet(const FlagsHolder_t flags, const char *flagName) {
    return findFlag(flags, flagName) != NULL;
}

fVal_t getFlagValue(const FlagsHolder_t flags, const char *flagName) {
    flagVal_t *flag = findFlag(flags, flagName);
    if (flag != NULL) return flag->val;
    fVal_t result = {};
    return result;
}

static enum status addFlag(FlagsHolder_t *flags, flagDescriptor desc, fVal_t val) {
    DBG_PRINTF("Adding %s flag\n", desc.flagFullName);
    if (findFlag(*flags, desc.flagShortName) != NULL) {
        fprintf(stderr, "Repeating flags not accepted\n");
        return ERROR; //don't accept repeating flags
    }
    if (flags->size == flags->reserved) {
        fprintf(stderr, "Number of flags is limited by %lu\n", FLAGS_RESERVED);
        return ERROR;
    }

    flags->flags[flags->size].desc = desc;
    flags->flags[flags->size].val = val;
    flags->size++;
    return SUCCESS;
}

void deleteFlags(FlagsHolder_t *flags) {
    for (size_t index = 0; index < flags->size; index++) {
        if (flags->flags[index].desc.type == TYPE_STRING)
            FREE(flags->flags[index].val.string_);
    }
    FREE(flags->flags);
}
