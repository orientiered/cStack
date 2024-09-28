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

#ifdef CANARY_PROTECTION
static bool canaryOk(canary_t canary, void *ptr);
static int fillCanaries(void *data, size_t len);
static ullPair_t getCanaries(void *data, size_t len);
static bool canariesOk(void *data, size_t len);
static size_t getCanaryAlign(size_t len);
static size_t getSizeWithCanary(size_t len);
static bool canaryOk(canary_t canary, void *ptr) {
    return ((canary ^ XOR_CONST) == (size_t)ptr);
}
static int fillCanaries(void *data, size_t len) {
    if ((uint64_t) data % 8 != 0 || len % 8 != 0 || len < 2*sizeof(canary_t) )
        return 0;
    *(canary_t*)data = (uint64_t)data ^ XOR_CONST;
    *(canary_t*)((char*)data + len - sizeof(canary_t))  = (uint64_t)data ^ XOR_CONST;
    return 1;
}
static ullPair_t getCanaries(void *data, size_t len) {
    ullPair_t result = {};
    if ((uint64_t) data % 8 != 0 || len % 8 != 0 || len < 2*sizeof(canary_t) )
        return result;
    result.first  = *(canary_t*)data;
    result.second = *(canary_t*)((char*)data + len - sizeof(canary_t));
    return result;
}
static size_t getCanaryAlign(size_t len) {
    return (8 - len % 8) % 8;
}
static size_t getSizeWithCanary(size_t len) {
    if (len == 0) return 0;
    return len + 2 * sizeof(canary_t) + getCanaryAlign(len);
}
static bool canariesOk(void *data, size_t len) {
    if (data == NULL) return true; //no canaries is ok
    ullPair_t canaries = getCanaries(data, len);
    return canaryOk(canaries.first, data) && canaryOk(canaries.second, data);
}
#endif
ON_HASH(                                                \
static uint64_t getDataHash(Stack_t *stk);              \
static uint64_t getStackHash(Stack_t *stk);             \
)

static enum StackError stackChangeSize(Stack_t *stk, enum StackSizeOp op);

static enum StackError stackChangeSize(Stack_t *stk, enum StackSizeOp op) {
    MY_ASSERT(stk, abort());
    MY_ASSERT(int(op) == 1 || int(op) == -1, abort());
    MY_ASSERT(!(int(op) == -1 && stk->size == 0), abort());
    STACK_ASSERT(stk);
    logPrintWithTime(L_EXTRA, 0, "Changing size %d: %lu -> %lu\n", op, stk->size, stk->size + int(op));
    bool needsRealloc = false;
    size_t newCapacity = 0;
    if        (op == OP_PUSH && stk->size >= stk->capacity) {
        needsRealloc = true;
        newCapacity = ((2 * stk->capacity) > ALLOC_MIN_SIZE) ? (2*stk->capacity) : ALLOC_MIN_SIZE;
    } else if (op == OP_POP  && stk->size > DEALLOC_MIN_SIZE && (4 * stk->size) < stk->capacity) {
        needsRealloc = true;
        newCapacity = stk->capacity / 2;
    }
    if (needsRealloc) {
        logPrintWithTime(L_DEBUG, 0, "Reallocating stack[%p] data: %lu --> %lu\n", stk, stk->capacity, newCapacity);
        ON_CANARY(                                                                                          \
        size_t capacityDelta = 2 * sizeof(canary_t) + getCanaryAlign(newCapacity * sizeof(stkElem_t));      \
        logPrint(L_DEBUG, 0, "Size with canaries in bytes: %lu --> %lu\n",                                  \
                 getSizeWithCanary(stk->capacity * sizeof(stkElem_t)),                                      \
                 getSizeWithCanary(newCapacity * sizeof(stkElem_t)));                                       \
        )
        if (stk->data == NULL)
            stk->data = (stkElem_t*) calloc(1, newCapacity * sizeof(stkElem_t) ON_CANARY(+ capacityDelta));
        else
            stk->data = (stkElem_t*) realloc((char*)stk->data ON_CANARY(- sizeof(canary_t)),
                                             newCapacity * sizeof(stkElem_t) ON_CANARY(+ capacityDelta));
        ON_CANARY(                                                                                          \
        fillCanaries(stk->data, newCapacity * sizeof(stkElem_t) + capacityDelta);                           \
        stk->data = (stkElem_t*)((char*)stk->data + sizeof(canary_t));                                      \
        )
        if (newCapacity > stk->capacity)
            memValSet(stk->data+stk->capacity, &POISON_ELEM, sizeof(POISON_ELEM), newCapacity-stk->capacity);
        stk->capacity = newCapacity;
    }
    stk->size += int(op);
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
    ON_DEBUG(MY_ASSERT(bornFile && name, abort());)
    memset(stk, 0, sizeof(*stk));
    ON_CANARY(                                          \
    fillCanaries(stk, sizeof(*stk));                    \
    )
    ON_DEBUG(                                           \
    stk->bornFile = bornFile;                           \
    stk->bornLine = bornLine;                           \
    stk->name = name + 1; /*first symbol is always &*/  \
    )
    stk->size = 0;
    stk->capacity = startCapacity;
    ON_CANARY(                                                                                          \
    size_t capacityDelta = 2 * sizeof(canary_t) + getCanaryAlign(startCapacity * sizeof(stkElem_t));    \
    )
    stk->data = (startCapacity == 0) ?                                   \
                NULL :                                                   \
                (stkElem_t *) calloc(1, startCapacity * sizeof(stkElem_t) + ON_CANARY(capacityDelta));

    ON_CANARY(
    if (stk->data) {                                                                        \
        fillCanaries(stk->data, startCapacity * sizeof(stkElem_t) + capacityDelta);         \
        stk->data = (stkElem_t*)((char*)stk->data + sizeof(canary_t));                      \
    }                                                                                       \
    )
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
    free((char*)stk->data ON_CANARY(- sizeof(canary_t)));
    memset(stk, 0, sizeof(*stk));
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
    if ((!stk->err) && !canariesOk(stk, sizeof(*stk)))                                  \
        stk->err = ERR_CANARY;                                                          \
    )
    if ((!stk->err) && (stk->size > stk->capacity))
        stk->err = ERR_LOGIC;
    if ((!stk->err) && (stk->size > MAX_STACK_SIZE))
        stk->err = ERR_SIZE;
    if ((!stk->err) && (stk->capacity > MAX_STACK_SIZE))
        stk->err = ERR_CAPACITY;
    if ((!stk->err) && ((stk->capacity > 0) ^ bool(stk->data))) //cap > 0 and data == 0 or cap == 0 and data !=0
        stk->err = ERR_DATA;
    ON_CANARY(                                                                                                                  \
    char *addr = (stk->data) ? ((char*) stk->data - sizeof(canary_t)) : NULL;                                                   \
    if (!(stk->err) && !canariesOk(addr, getSizeWithCanary(stk->capacity * sizeof(stkElem_t))))                                 \
        stk->err = ERR_CANARY;                                                                                                  \
    )
    ON_HASH(                                                    \
    if ((!stk->err) && (stk->dataHash != getDataHash(stk)))     \
        stk->err = ERR_HASH_DATA;                               \
    if ((!stk->err) && (stk->stackHash != getStackHash(stk)))   \
        stk->err = ERR_HASH_STACK;                              \
    )
    return (stk->err == ERR_NO);
}

static bool stackDumpData(Stack_t *stk);
static bool stackDumpData(Stack_t *stk) {
    logPrint(L_ZERO, 0, "\tdata[%p] {\n", stk->data);
    if (stk->data) {
        ON_CANARY(                                                                                                              \
        ullPair_t canaries = getCanaries((char*)stk->data - sizeof(canary_t),                                                   \
                                            getSizeWithCanary(stk->capacity * sizeof(stkElem_t)));                              \
        logPrint(L_ZERO, 0, "\t^ [ -1] %zX (DataCanary1)\n", canaries.first);                                                   \
        if (!canaryOk(canaries.first, (char*)stk->data - sizeof(canary_t)))                                                     \
            logPrint(L_ZERO, 0, "BROKEN:     %zX is correct canary\n", ((size_t)stk->data - sizeof(canary_t)) ^ XOR_CONST);     \
        )
        for (size_t index = 0; index < stk->size && index < stk->capacity; index++) {
            logPrint(L_ZERO, 0, "\t* ");
            if (memcmp(stk->data + index, &POISON_ELEM, sizeof(POISON_ELEM)) == 0)
                logPrint(L_ZERO, 0, "[%3d] %zX (POISON)\n", index, POISON_ELEM);
            else
                logPrint(L_ZERO, 0, "[%3d] " STK_ELEM_FMT "\n", index, stk->data[index]);
        }
        int poisonElems = 0;
        for (size_t index = stk->size; index < stk->capacity; index++) {
            if (memcmp(stk->data + index, &POISON_ELEM, sizeof(POISON_ELEM)) == 0) {
                if (poisonElems == 0) {
                    poisonElems = 1;
                    logPrint(L_ZERO, 0, "\t  [%3d] %zX (POISON)\n", index, POISON_ELEM);
                } else if (poisonElems == 1) {
                    logPrint(L_ZERO, 0, "\t  [|||] %zX (POISON)\n", POISON_ELEM);
                    poisonElems = 2;
                }
            } else {
                if (poisonElems == 2) {
                    poisonElems = 0;
                    logPrint(L_ZERO, 0, "\t  [%3d] %zX (POISON)\n", index-1, POISON_ELEM);
                }
                logPrint(L_ZERO, 0, "\t  [%3d] " STK_ELEM_FMT "\n", index, stk->data[index]);
            }
        }
        if (poisonElems == 2) {
            poisonElems = 0;
            logPrint(L_ZERO, 0, "\t  [%3d] %zX (POISON)\n", stk->capacity-1, POISON_ELEM);
        }
    ON_CANARY(                                                                                                              \
    logPrint(L_ZERO, 0, "\t^ [%3d] %zX (DataCanary2)\n", stk->capacity, canaries.second);                                   \
    if (!canaryOk(canaries.second, (char*)stk->data - sizeof(canary_t)))                                                    \
        logPrint(L_ZERO, 0, "BROKEN:     %zX is correct canary\n", ((size_t)stk->data - sizeof(canary_t)) ^ XOR_CONST);     \
    )
    }
    logPrint(L_ZERO, 0, "\t}\n");
    return true;
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
    logPrint(L_ZERO, 0, "\tCanary1  = %zX\n", stk->goose1);                         \
    if (!canaryOk(stk->goose1, stk))                                                \
        logPrint(L_ZERO, 0, "\tBROKEN: must be %zX\n", (size_t)stk ^ XOR_CONST);    \
    )
    logPrint(L_ZERO, 0, "\terr      = %s(%d)\n", stackErrorToStr(stk->err), int(stk->err));
    logPrint(L_ZERO, 0, "\tsize     = %lu\n", stk->size);
    logPrint(L_ZERO, 0, "\tcapacity = %lu\n", stk->capacity);
    stackDumpData(stk);
    ON_HASH(                                                                                    \
    logPrint(L_ZERO, 0, "\tdataHash  = %.16zX\n", stk->dataHash);                               \
    if (stk->dataHash != getDataHash(stk))                                                      \
        logPrint(L_ZERO, 0, "\tWrong hash: %.16zX is correct hash\n", getDataHash(stk));        \
    logPrint(L_ZERO, 0, "\tstackHash = %.16zX\n");                                              \
    if (stk->stackHash != getStackHash(stk))                                                    \
        logPrint(L_ZERO, 0, "\tWrong hash: %.16zX is correct hash\n", getStackHash(stk));       \
    )
    ON_CANARY(                                                                      \
    logPrint(L_ZERO, 0, "\tCanary2   = %zX\n", stk->goose2);                        \
    if (!canaryOk(stk->goose2, stk))                                                \
        logPrint(L_ZERO, 0, "\tBROKEN: must be %zX\n", (size_t)stk ^ XOR_CONST);    \
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

