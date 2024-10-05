/// @file Stack container
/*------------------STACK CONTAINER-------------------------------------------*/
/*------------------WITH CANARY AND HASH PROTECTION---------------------------*/
/*------------------orientiered-MIPT-2024-------------------------------------*/
#ifndef C_STACK_H
#define C_STACK_H

#define CANARY_PROTECTION
#define HASH_PROTECTION

/*------------------DEFINES FOR CONDITIONAL COMPILATION-----------------------*/

#ifndef NDEBUG

# ifdef CANARY_PROTECTION
#  define ON_CANARY(...) __VA_ARGS__
# else
#  define ON_CANARY(...)
# endif

# ifdef HASH_PROTECTION
#  define ON_HASH(...) __VA_ARGS__
# else
#  define ON_HASH(...)
# endif

# define ON_DEBUG(...) __VA_ARGS__

#else

# define ON_DEBUG(...)
# define ON_CANARY(...)
# define ON_HASH(...)

#endif

/*------------------STRUCTS AND CONSTANTS-------------------------------------*/

typedef int stkElem_t;
const stkElem_t POISON_ELEM = stkElem_t(0xABADF00DA2DDEAD3);        //this value is filled in empty memory
#define STK_ELEM_FMT "%d"
ON_CANARY(                                              \
static const uint64_t XOR_CONST =  0xEDABEDAF8A40FF15;  \
typedef uint64_t canary_t;                              \
)
ON_HASH(typedef uint64_t hash_t;)

typedef uint64_t StackError_t;
enum StackErrors {
    STACK_OK                = 0,                    ///< Ok
    ERR_NULLPTR             = 1 << 0,               ///< NULL stack_t pointer passed
    ERR_DATA                = 1 << 1,               ///< Data is NULL when capacity > 0
    ERR_SIZE                = 1 << 2,               ///< Size > maxSize
    ERR_CAPACITY            = 1 << 3,               ///< Capacity > maxSize
    ERR_LOGIC               = 1 << 4,               ///< Size > capacity

    ON_CANARY(
    ERR_CANARY_LEFT         = 1 << 5,               ///< Wrong left stack canary
    ERR_CANARY_RIGHT        = 1 << 6,               ///< Wrong right stack canary
    ERR_DATA_CANARY_LEFT    = 1 << 7,               ///< Wrong left data canary
    ERR_DATA_CANARY_RIGHT   = 1 << 8,               ///< Wrong right data canary
    ERR_CANARY              = ((1 << 4) - 1) << 5,  ///< Any canary is wrong
    )

    ON_HASH(
    ERR_HASH_DATA           = 1 << 9,               ///< Incorrect data hash
    ERR_HASH_STACK          = 1 << 10,              ///< Incorrect stack hash
    )
};

typedef struct {
    ON_CANARY(canary_t goose1;)                 ///< first canary
    ON_DEBUG(
    const char *initFile;                       ///< file where stack was constructed
    int initLine;                               ///< line in that file
    const char *name;                           ///< name passed in stackCtor
    )
    size_t size;                                ///< Number of elements in stack
    size_t capacity;                            ///< Size of reserved memory
    stkElem_t *data;                            ///< Array with elements
    ON_HASH(
    hash_t dataHash;                            ///< Hash of elements (of all allocated memory)
    hash_t stackHash;                           ///< Hash of struct itself
    )
    ON_CANARY(canary_t goose2;)                 ///< Second canary
} Stack_t;

/* -----------------FUNCTIONS TO WORK WITH STACK------------------------------*/

/// @brief Construct stack with given capacity
#define stackCtor(stk, startCapacity) stackCtorBase(stk, startCapacity ON_DEBUG(, __FILE__, __LINE__, #stk))

/// @brief Delete stack
StackError_t stackDtor(Stack_t *stk);

/// @brief Push element to stack
#define stackPush(stk, val) stackPushBase(stk, val ON_DEBUG(, __FILE__, __LINE__, #stk))

/// @brief Pop element from stack
/// You can't use this function when size is 0
#define stackPop(stk) stackPopBase(stk ON_DEBUG(, __FILE__, __LINE__, #stk))

/// @brief Get top element from stack
stkElem_t stackTop(Stack_t *stk);

/// @brief Get stack size
size_t stackGetSize(Stack_t *stk);

/// @brief Check stk for errors
/// Return false if there's any error, wright it in err field of stack
StackError_t stackVerify(Stack_t *stk);

/// @brief Wright stack dump is log file
#define stackDump(stk) stackDumpBase(stk, __FILE__, __LINE__, __PRETTY_FUNCTION__)

/// @brief Convert stack error code to string
const char *stackFirstErrorToStr(StackError_t err);

/* -----------------BASE LIBRARY FUNCTIONS; DO NOT USE------------------------*/

StackError_t stackCtorBase(Stack_t *stk, size_t startCapacity
                ON_DEBUG(, const char *initFile, int initLine, const char *name));

StackError_t stackPushBase(Stack_t *stk, stkElem_t val
                ON_DEBUG(, const char *file, int line, const char *name));

stkElem_t stackPopBase(Stack_t *stk
                ON_DEBUG(, const char *file, int line, const char *name));

StackError_t stackDumpBase(Stack_t *stk, const char *file, int line, const char *function);

/* -----------------ASSERTS FOR DEBUGGING-------------------------------------*/

#ifndef NDEBUG
# define STACK_ASSERT(stk)                                                                              \
    do {                                                                                                \
        StackError_t stkError = stackVerify(stk);                                                       \
        if (stkError) {                                                                                 \
            logPrintWithTime(L_ZERO, 0, "Stack error occurred: %s\n", stackFirstErrorToStr(stkError));  \
            stackDump(stk);                                                                             \
            MY_ASSERT(0, abort());                                                                      \
        }                                                                                               \
    } while (0)

//! DO NOT USE THIS MACRO
//! IT USES LOCAL VARIABLES NAME_, FILE_, LINE_
# define STACK_VERBOSE_ASSERT(stk)                                                                      \
    do {                                                                                                \
        StackError_t stkError = stackVerify(stk);                                                       \
        if (stkError) {                                                                                 \
            logPrintWithTime(L_ZERO, 1, "Stack \"%s\" error in %s:%d : %s\n",                           \
                            NAME_, FILE_, LINE_, stackFirstErrorToStr(stkError));                       \
            stackDump(stk);                                                                             \
            MY_ASSERT(0, abort());                                                                      \
        }                                                                                               \
    } while (0)

#else
# define STACK_ASSERT(stk)
# define STACK_VERBOSE_ASSERT(stk)
#endif

#endif
