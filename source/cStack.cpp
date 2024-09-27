#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#include "error_debug.h"
#include "logger.h"
#include "utils.h"
#include "cStack.h"

const size_t DEALLOC_MIN_SIZE = 5;
const size_t ALLOC_MIN_SIZE = 5;
const size_t MAX_STACK_SIZE = 1 << 28;
enum StackSizeOp {
    OP_PUSH = 1,
    OP_POP = -1
};

ON_CANARY(                                              \
static bool canaryOk(canary_t canary, void *ptr);       \
static bool canaryOk(canary_t canary, void *ptr) {      \
    return ((canary ^ XOR_CONST) == (size_t)ptr);       \
}

)
ON_HASH(                                                \
static uint64_t getDataHash(Stack_t *stk);              \
static uint64_t getStackHash(Stack_t *stk);             \
)

static enum StackError stackChangeSize(Stack_t *stk, enum StackSizeOp op);

static enum StackError stackChangeSize(Stack_t *stk, enum StackSizeOp op) {
    MY_ASSERT(stk, abort());
    STACK_ASSERT(stk);

    logPrintWithTime(L_EXTRA, 0, "Changing size %d: %lu -> %lu\n", op, stk->size, stk->size + int(op));
    switch(op) {
    case OP_PUSH:
        if (stk->size == stk->capacity) {
            size_t newCapacity = ((2 * stk->capacity) > ALLOC_MIN_SIZE) ? (2*stk->capacity) : ALLOC_MIN_SIZE;
            logPrintWithTime(L_DEBUG, 0, "Reallocating up: %lu --> %lu\n", stk->capacity, newCapacity);
            stk->data = (stkElem_t*) realloc(stk->data, newCapacity * sizeof(stkElem_t));
            //POISON value instead of 0
            memValSet(stk->data+stk->capacity, &POISON_ELEM, sizeof(POISON_ELEM), newCapacity-stk->capacity);
            stk->capacity = newCapacity;
        }
        stk->size++;
        break;
    case OP_POP:
        if (stk->size == 0) {
            stk->err = ERR_SIZE;
            break;
        }

        stk->size--;
        if (stk->size > DEALLOC_MIN_SIZE &&
            (4 * stk->size) < stk->capacity) {
            size_t newCapacity = stk->capacity / 2;
            logPrintWithTime(L_DEBUG, 0, "Reallocating down: %lu --> %lu\n", stk->capacity, newCapacity);
            stk->data = (stkElem_t*) realloc(stk->data, newCapacity * sizeof(stkElem_t));
            stk->capacity = newCapacity;
        }
        break;
    default:
        stk->err = ERR_LOGIC;
        break;
    }
    ON_HASH(                                    \
    stk->dataHash  = getDataHash(stk);          \
    stk->stackHash = getStackHash(stk);         \
    )
    STACK_ASSERT(stk);
    return stk->err;
}

enum StackError stackCtorBase(Stack_t *stk, size_t startCapacity
                ON_DEBUG(, const char *bornFile, int bornLine, const char *name)) {
    MY_ASSERT(stk, abort());
    ON_CANARY(                                  \
    stk->goose1 = (size_t)stk ^ XOR_CONST;      \
    stk->goose2 = (size_t)stk ^ XOR_CONST;      \
    )
    ON_DEBUG(                                           \
    stk->bornFile = bornFile;                           \
    stk->bornLine = bornLine;                           \
    stk->name = name + 1; /*first symbol is always &*/  \
    )
    stk->size = 0;
    stk->capacity = startCapacity;
    stk->data = (startCapacity == 0) ?                                   \
                NULL :                                                   \
                (stkElem_t *) calloc(startCapacity, sizeof(stkElem_t));

    ON_HASH(                                    \
    stk->dataHash  = getDataHash(stk);          \
    stk->stackHash = getStackHash(stk);         \
    )
    STACK_ASSERT(stk);

    return stk->err;
}

enum StackError stackDtor(Stack_t *stk) {
    MY_ASSERT(stk, abort());
    STACK_ASSERT(stk);
    free(stk->data); stk->data = NULL;
    stk->size = 0;
    stk->capacity = 0;
    stk->err = ERR_NO;
    return ERR_NO;
}

enum StackError stackPush(Stack_t *stk, stkElem_t val) {
    MY_ASSERT(stk, abort());
    STACK_ASSERT(stk);

    stackChangeSize(stk, OP_PUSH);
    stk->data[stk->size-1] = val;
    ON_HASH(                            \
    stk->dataHash  = getDataHash(stk);  \
    stk->stackHash = getStackHash(stk); \
    )
    STACK_ASSERT(stk);
    logPrintWithTime(L_EXTRA, 0, "Push in %p: " STK_ELEM_FMT "\n", stk, val);
    return stk->err;
}

stkElem_t stackPop(Stack_t *stk) {
    MY_ASSERT(stk, abort());
    STACK_ASSERT(stk);
    MY_ASSERT((stk->size > 0), abort());

    logPrintWithTime(L_EXTRA, 0, "Popping element: size = %lu, val = " STK_ELEM_FMT "\n", stk->size, stk->data[stk->size-1]);
    stkElem_t val = stk->data[stk->size-1];
    stackChangeSize(stk, OP_POP);
    ON_HASH(                            \
    stk->dataHash  = getDataHash(stk);  \
    stk->stackHash = getStackHash(stk); \
    )

    STACK_ASSERT(stk);
    return val;
}

stkElem_t stackTop(Stack_t *stk) {
    MY_ASSERT(stk, abort());
    STACK_ASSERT(stk);
    MY_ASSERT((stk->size > 0), abort());

    return stk->data[stk->size-1];
}

bool stackOk(Stack_t *stk) {
    MY_ASSERT(stk, abort());

    ON_CANARY(                                                                          \
    if ((!stk->err) && (!canaryOk(stk->goose1, stk) || !canaryOk(stk->goose2, stk)))    \
        stk->err = ERR_CANARY;                                                          \
    )
    ON_HASH(                                                    \
    if ((!stk->err) && (stk->dataHash != getDataHash(stk)))     \
        stk->err = ERR_HASH_DATA;                               \
    if ((!stk->err) && (stk->stackHash != getStackHash(stk)))   \
        stk->err = ERR_HASH_STACK;                              \
    )
    if ((!stk->err) && (stk->size > stk->capacity))
        stk->err = ERR_LOGIC;
    if ((!stk->err) && (stk->size > MAX_STACK_SIZE))
        stk->err = ERR_SIZE;
    if ((!stk->err) && (stk->capacity > MAX_STACK_SIZE))
        stk->err = ERR_CAPACITY;
    if ((!stk->err) && ((stk->capacity > 0) ^ bool(stk->data))) //cap > 0 and data == 0 or cap == 0 and data !=0
        stk->err = ERR_DATA;

    return (stk->err == ERR_NO);
}

bool stackDumpBase(Stack_t *stk, const char *file, int line, const char *function) {
    logPrintWithTime(L_ZERO, 0, "Stack_t dump:\n");
    logPrint(L_ZERO, 0, "called from %s:%d (%s)\n", file, line, function);
    if (!stk) {
        logPrint(L_ZERO, 0, "NULL pointer has been passed\n");
        return false;
    }
    #ifndef NDEBUG
    logPrint(L_ZERO, 0, "\"%s\"[%p], created in %s:%d \n", stk->name, stk, stk->bornFile, stk->bornLine);
    #else
    logPrint(L_ZERO, 0, "\[%p], use debug version for more info \n", stk);
    #endif
    logPrint(L_ZERO, 0, "{\n");
    ON_CANARY(                                                                      \
    logPrint(L_ZERO, 0, "\tCanary1  = %lu\n", stk->goose1);                         \
    if (!canaryOk(stk->goose1, stk))                                                \
        logPrint(L_ZERO, 0, "\tBROKEN: must be %lu\n", (size_t)stk ^ XOR_CONST);    \
    )
    logPrint(L_ZERO, 0, "\terr      = %s(%d)\n", stackErrorToStr(stk->err), int(stk->err));
    logPrint(L_ZERO, 0, "\tsize     = %lu\n", stk->size);
    logPrint(L_ZERO, 0, "\tcapacity = %lu\n", stk->capacity);
    logPrint(L_ZERO, 0, "\tdata[%p] {\n", stk->data);
    if (stk->data) {
        for (size_t index = 0; index < stk->capacity; index++) {
            if (index < stk->size)
                logPrint(L_ZERO, 0, "\t* ");
            else
                logPrint(L_ZERO, 0, "\t  ");
            if (memcmp(stk->data + index, &POISON_ELEM, sizeof(POISON_ELEM)) == 0)
                logPrint(L_ZERO, 0, "[%3d] %x (POISON)\n", index, POISON_ELEM);
            else
                logPrint(L_ZERO, 0, "[%3d] " STK_ELEM_FMT "\n", index, stk->data[index]);
        }
    }
    logPrint(L_ZERO, 0, "\t}\n");
    ON_HASH(                                                                                    \
        logPrint(L_ZERO, 0, "\tdataHash  = %llu\n", stk->dataHash);                             \
        if (stk->dataHash != getDataHash(stk))                                                  \
            logPrint(L_ZERO, 0, "\tIncorrect data hash: must be %llu\n", getDataHash(stk));     \
        logPrint(L_ZERO, 0, "\tstackHash = %llu\n");                                            \
        if (stk->stackHash != getStackHash(stk))                                                \
            logPrint(L_ZERO, 0, "\tIncorrect stack hash: must be %llu\n", getStackHash(stk));   \
    )
    ON_CANARY(                                                                      \
    logPrint(L_ZERO, 0, "\tCanary2  = %lu\n", stk->goose2);                         \
    if (!canaryOk(stk->goose2, stk))                                                \
        logPrint(L_ZERO, 0, "\tBROKEN: must be %lu\n", (size_t)stk ^ XOR_CONST);    \
    )
    logPrint(L_ZERO, 0, "}\n");
    return true;
}

const char *stackErrorToStr(enum StackError err) {
    static const char *errStrings[] = {
        "ERR_NO",
        "ERR_DATA",
        "ERR_SIZE",
        "ERR_CAPACITY",
        "ERR_LOGIC"
        ON_CANARY(, "ERR_CANARY")
        ON_HASH(, "ERR_HASH_DATA", "ERR_HASH_STACK")
    };
    return errStrings[int(err)];
}

ON_HASH(                                                            \
static uint64_t getDataHash(Stack_t *stk) {                         \
    MY_ASSERT(stk, abort());                                        \
    return memHash(stk->data, stk->capacity*sizeof(stkElem_t));     \
}                                                                   \
static uint64_t getStackHash(Stack_t *stk) {                        \
    MY_ASSERT(stk, abort());                                        \
    uint64_t oldHash = stk->stackHash;                              \
    stk->stackHash = 0;                                             \
    uint64_t newHash = memHash(stk, sizeof(Stack_t));               \
    stk->stackHash = oldHash;                                       \
    return newHash;                                                 \
}                                                                   \
)

