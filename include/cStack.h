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
//Add more digits to xor_const and poison elem
const stkElem_t POISON_ELEM = stkElem_t(0xABADF00DA2DDEAD3);        //this value is filled in empty memory
#define STK_ELEM_FMT "%d"
ON_CANARY(                                              \
static const uint64_t XOR_CONST =  0xEDABEDAF8A40FF15;  \
typedef uint64_t canary_t;                              \
)
ON_HASH(typedef uint64_t hash_t;)

typedef struct {
    bool ERR_DATA       :1;                 ///< Data is NULL when capacity > 0
    bool ERR_SIZE       :1;                 ///< Size > maxSize
    bool ERR_CAPACITY   :1;                 ///< Capacity > maxSize
    bool ERR_LOGIC      :1;                 ///< Size > capacity
    ON_CANARY(
    bool ERR_CANARY     :1;                 ///< At least one canary is broken
    )
    ON_HASH(
    bool ERR_HASH_DATA  :1;                 ///< Incorrect data and struct hash
    bool ERR_HASH_STACK :1;
    )
} StackError_t;

typedef struct {
    ON_CANARY(canary_t goose1;)                 ///< first canary
    ON_DEBUG(
    const char *initFile;                       ///< file where stack was constructed
    int initLine;                               ///< line in that file
    const char *name;                           ///< name passed in stackCtor
    )
    StackError_t err;                           ///< Error code
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
bool stackVerify(Stack_t *stk);

/// @brief Wright stack dump is log file
#define stackDump(stk) stackDumpBase(stk, __FILE__, __LINE__, __PRETTY_FUNCTION__)

/// @brief Convert stack error code to string
const char *stackErrorToStr(StackError_t err);

/* -----------------BASE LIBRARY FUNCTIONS; DO NOT USE------------------------*/

StackError_t stackCtorBase(Stack_t *stk, size_t startCapacity
                ON_DEBUG(, const char *initFile, int initLine, const char *name));

StackError_t stackPushBase(Stack_t *stk, stkElem_t val
                ON_DEBUG(, const char *file, int line, const char *name));

stkElem_t stackPopBase(Stack_t *stk
                ON_DEBUG(, const char *file, int line, const char *name));

bool stackDumpBase(Stack_t *stk, const char *file, int line, const char *function);

/* -----------------ASSERTS FOR DEBUGGING-------------------------------------*/

#ifndef NDEBUG
# define STACK_ASSERT(stk)                                                                          \
    do {                                                                                            \
        if (!stk) {                                                                                 \
            logPrintWithTime(L_ZERO, 1, "NULL \"%s\" stack pointer passed\n");                      \
            MY_ASSERT(0, abort());                                                                  \
        }                                                                                           \
        if (!stackVerify(stk)) {                                                                    \
            logPrintWithTime(L_ZERO, 0, "Stack error occurred: %s\n", stackErrorToStr(stk->err));   \
            stackDump(stk);                                                                         \
            MY_ASSERT(0, abort());                                                                  \
        }                                                                                           \
    } while (0)

# define STACK_VERBOSE_ASSERT(stk, file, line, name)                                                \
    do {                                                                                            \
        if (!stk) {                                                                                 \
            logPrintWithTime(L_ZERO, 1, "NULL \"%s\" stack pointer passed in %s:%d\n",              \
                            name, file, line);                                                      \
            MY_ASSERT(0, abort());                                                                  \
        }                                                                                           \
        if (!stackVerify(stk)) {                                                                    \
            logPrintWithTime(L_ZERO, 1, "Stack \"%s\" error in %s:%d : %s\n",                       \
                            name, file, line, stackErrorToStr(stk->err));                           \
            stackDump(stk);                                                                         \
            MY_ASSERT(0, abort());                                                                  \
        }                                                                                           \
    } while (0)
#else
# define STACK_ASSERT(stk)
#endif

#endif
