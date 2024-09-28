#ifndef C_STACK_H
#define C_STACK_H

//TODO: -D flag
#define CANARY_PROTECTION
#define HASH_PROTECTION

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

const uint64_t POISON_ELEM = 0xABADF00D;
typedef double stkElem_t;
#define STK_ELEM_FMT "%lf"
ON_CANARY(                                      \
static const uint64_t XOR_CONST =  0xEDABEDAF;  \
typedef uint64_t canary_t;                      \
)
ON_HASH(typedef uint64_t hash_t;)

enum StackError {
    ERR_NO = 0,
    ERR_DATA,
    ERR_SIZE,
    ERR_CAPACITY,
    ERR_LOGIC
    ON_CANARY(, ERR_CANARY)
    ON_HASH(, ERR_HASH_DATA, ERR_HASH_STACK)
};

typedef struct {
    ON_CANARY(canary_t goose1;)
    ON_DEBUG(
    const char *bornFile;
    int bornLine;
    const char *name;
    )
    enum StackError err;
    size_t size;
    size_t capacity;
    //TODO: data canaries
    stkElem_t *data;
    ON_HASH(
    hash_t dataHash;
    hash_t stackHash;
    )
    ON_CANARY(canary_t goose2;)
} Stack_t;

enum StackError stackCtorBase(Stack_t *stk, size_t startCapacity
                ON_DEBUG(, const char *bornFile, int bornLine, const char *name));
#define stackCtor(stk, startCapacity) stackCtorBase(stk, startCapacity ON_DEBUG(, __FILE__, __LINE__, #stk))
enum StackError stackDtor(Stack_t *stk);

enum StackError stackPush(Stack_t *stk, stkElem_t val);
stkElem_t stackPop(Stack_t *stk);
stkElem_t stackTop(Stack_t *stk);

bool stackOk(Stack_t *stk);
bool stackDumpBase(Stack_t *stk, const char *file, int line, const char *function);
#define stackDump(stk) stackDumpBase(stk, __FILE__, __LINE__, __PRETTY_FUNCTION__)

#ifndef NDEBUG
#define STACK_SAFE(expr, stk)                                   \
    do {                                                        \
        (expr);                                                 \
        enum StackError err = stackOk(stk);                     \
        if (err != ERR_NO) {                                    \
            logPrint(L_ZERO, 1, "Stack %p is broken\n", stk);   \
            abort();                                            \
        }                                                       \
    } while (0)
#else
#define STACK_SAFE(expr, stk) (expr);
#endif

//TODO: maybe do not abort
#ifndef NDEBUG
# define STACK_ASSERT(stk)                                                                          \
    do {                                                                                            \
        if (!stackOk(stk)) {                                                                        \
            logPrintWithTime(L_ZERO, 0, "Stack error occurred: %s\n", stackErrorToStr(stk->err));   \
            stackDump(stk);                                                                         \
            MY_ASSERT(0, abort());                                                                  \
        }                                                                                           \
    } while (0)
#else
# define STACK_ASSERT(stk)
#endif

const char *stackErrorToStr(enum StackError err);

#endif
