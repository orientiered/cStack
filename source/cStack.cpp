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

/// @brief Stack change size supported operations
enum StackSizeOp {
    OP_PUSH = 1,    ///< push element (size += 1)
    OP_POP = -1     ///< pop  element (size -= 1)
};


ON_CANARY(
static bool canaryOk(canary_t canary, void *ptr);
static bool canariesOk(void *data, size_t len, bool doOffset);
static int fillCanaries(void *data, size_t len);
static ullPair_t getCanaries(void *data, size_t len);
static size_t getCanaryAlign(size_t len);
static size_t getSizeWithCanary(size_t len);
)

static void *smartRecalloc(void *data, size_t newLen, size_t oldLen, size_t elemSize) {
    logPrintWithTime(L_EXTRA, 0, "---------------------MEMORY LOG---------------------\n");

    ON_CANARY(
    size_t sizeDelta = 2 * sizeof(canary_t) + getCanaryAlign(newLen * elemSize);
    if (data != NULL)
        data = (char*) data - sizeof(canary_t);
    )

    if (newLen == 0) {
        logPrint(L_EXTRA, 0, "FREE: %p\n", data);
        free(data);
    } else if (data == NULL) {
        data = calloc(newLen * elemSize ON_CANARY(+ sizeDelta), 1);
        logPrint(L_DEBUG, 0, "CALLOC: %p\n", data);
        logPrint(L_DEBUG, 0, "SIZE = %zu * %zu\n", newLen, elemSize);
    } else {
        logPrint(L_DEBUG, 0, "REALLOC from: %p\n", data);
        data = realloc(data, newLen * elemSize ON_CANARY(+ sizeDelta));
        logPrint(L_DEBUG, 0, "REALLOC   to: %p\n", data);
        logPrint(L_DEBUG, 0, "OLDSIZE = %zu * %zu\n", oldLen, elemSize);
        logPrint(L_DEBUG, 0, "NEWSIZE = %zu * %zu\n", newLen, elemSize);

    }

    if (newLen > oldLen) {
        char *fillStart = (char*) data ON_CANARY(+ sizeof(canary_t)) + elemSize * oldLen;
        memValSet(fillStart, &POISON_ELEM, elemSize, newLen - oldLen);
    }

    ON_CANARY(
    logPrint(L_DEBUG, 0, "WITH_CANARIES (bytes): %lu --> %lu\n",
                getSizeWithCanary(oldLen * elemSize),
                getSizeWithCanary(newLen * elemSize));
    if (newLen != 0)
        fillCanaries(data, newLen * elemSize + sizeDelta);
    data = (char*) data + sizeof(canary_t);
    )
    logPrintWithTime(L_DEBUG, 0, "-------------------------\n");
    return data;
}

ON_HASH(
static uint64_t getDataHash(Stack_t *stk);
static uint64_t getStackHash(Stack_t *stk);
)

static StackError_t stackChangeSize(Stack_t *stk, enum StackSizeOp op);

static StackError_t stackChangeSize(Stack_t *stk, enum StackSizeOp op) {
    MY_ASSERT(stk, abort());
    MY_ASSERT(int(op) == 1 || int(op) == -1, abort());
    MY_ASSERT(!(int(op) == -1 && stk->size == 0), abort());
    STACK_ASSERT(stk);
    logPrintWithTime(L_EXTRA, 0, "Stack_t[%p] size change %d: %lu -> %lu\n", stk, op, stk->size, stk->size + int(op));

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
        stk->data = (stkElem_t*) smartRecalloc(stk->data, newCapacity, stk->capacity, sizeof(stkElem_t));
        stk->capacity = newCapacity;
    }

    stk->size += int(op);
    if (op == OP_POP) memcpy(stk->data + stk->size, &POISON_ELEM, sizeof(stkElem_t));
    ON_HASH(
    stk->dataHash  = getDataHash(stk);
    stk->stackHash = getStackHash(stk);
    )
    STACK_ASSERT(stk);
    return stk->err;
}

StackError_t stackCtorBase(Stack_t *stk, size_t startCapacity
                ON_DEBUG(, const char *initFile, int initLine, const char *name)) {
    MY_ASSERT(stk, abort());
    ON_DEBUG(MY_ASSERT(initFile && name, abort());)

    memset(stk, 0, sizeof(*stk));
    ON_CANARY(
    fillCanaries(stk, sizeof(*stk));
    )
    ON_DEBUG(
    stk->initFile = initFile;
    stk->initLine = initLine;
    stk->name = name;
    )
    stk->size = 0;
    MY_ASSERT(startCapacity < MAX_STACK_SIZE, {
        ON_DEBUG(
        logPrint(L_ZERO, 1, "\"%s\" in %s:%d\n", name, initFile, initLine);
        )
        logPrint(L_ZERO, 1, "Capacity is too big\n");
        abort();
    });
    stk->capacity = startCapacity;
    stk->data = (startCapacity == 0) ?
                NULL :
                (stkElem_t *) smartRecalloc(NULL, startCapacity, 0, sizeof(stkElem_t));

    ON_HASH(
    stk->dataHash  = getDataHash(stk);
    stk->stackHash = getStackHash(stk);
    )
    STACK_ASSERT(stk);

    return stk->err;
}

StackError_t stackDtor(Stack_t *stk) {
    STACK_ASSERT(stk);
    smartRecalloc(stk->data, 0, stk->capacity, sizeof(stkElem_t));
    memset(stk, 0, sizeof(*stk));
    return stk->err;
}

StackError_t stackPushBase(Stack_t *stk, stkElem_t val
                ON_DEBUG(, const char *file, int line, const char *name)) {
    STACK_VERBOSE_ASSERT(stk, file, line, name);

    stackChangeSize(stk, OP_PUSH);
    stk->data[stk->size-1] = val;
    ON_HASH(
    stk->dataHash  = getDataHash(stk);
    stk->stackHash = getStackHash(stk);
    )

    STACK_VERBOSE_ASSERT(stk, file, line, name);
    logPrintWithTime(L_EXTRA, 0, "Push in %p: " STK_ELEM_FMT "\n", stk, val);
    return stk->err;
}

stkElem_t stackPopBase(Stack_t *stk
            ON_DEBUG(, const char *file, int line, const char *name)) {
    STACK_VERBOSE_ASSERT(stk, file, line, name);
    MY_ASSERT((stk->size > 0), abort());

    logPrintWithTime(L_EXTRA, 0, "Popping element: size = %lu, val = " STK_ELEM_FMT "\n", stk->size, stk->data[stk->size-1]);
    stkElem_t val = stk->data[stk->size-1];
    stackChangeSize(stk, OP_POP);
    ON_HASH(
    stk->dataHash  = getDataHash(stk);
    stk->stackHash = getStackHash(stk);
    )
    STACK_VERBOSE_ASSERT(stk, file, line, name);
    return val;
}

stkElem_t stackTop(Stack_t *stk) {
    STACK_ASSERT(stk);
    MY_ASSERT((stk->size > 0), abort());

    return stk->data[stk->size-1];
}

size_t stackGetSize(Stack_t *stk) {
    STACK_ASSERT(stk);
    return stk->size;
}

bool stackVerify(Stack_t *stk) {
    MY_ASSERT(stk, abort());

    //cap > 0 and data == 0 or cap == 0 and data !=0
    bool dataError = (stk->capacity > 0) ^ bool(stk->data);

    ON_HASH(
    if (stk->stackHash != getStackHash(stk))
        stk->err |= ERR_HASH_STACK;
    if (!(dataError || (stk->err & ERR_HASH_STACK)) && (stk->dataHash != getDataHash(stk)))
        stk->err |= ERR_HASH_DATA;
    )
    bool dataCorrupted = dataError || (stk->err & ERR_HASH_STACK);
    ON_CANARY(
    if (!canariesOk(stk, sizeof(*stk), 0))
        stk->err |= ERR_CANARY;
    if (!dataCorrupted && !canariesOk(stk->data, stk->capacity * sizeof(stkElem_t), 1))
        stk->err |= ERR_CANARY;
    )
    if (dataError)
        stk->err |= ERR_DATA;
    if (stk->size > stk->capacity)
        stk->err |= ERR_LOGIC;
    if (stk->size > MAX_STACK_SIZE)
        stk->err |= ERR_SIZE;
    if (stk->capacity > MAX_STACK_SIZE)
        stk->err |= ERR_CAPACITY;

    return !stk->err;
}

static bool stackDumpData(Stack_t *stk);
static bool stackDumpErr (Stack_t *stk);

static bool stackDumpErr(Stack_t *stk) {
    logPrint(L_ZERO, 0, "\terr = {\n");
    logPrint(L_ZERO, 0, "\t\tERR_SIZE       = %u\n", (bool) (stk->err & ERR_SIZE));
    logPrint(L_ZERO, 0, "\t\tERR_DATA       = %u\n", (bool) (stk->err & ERR_DATA));
    logPrint(L_ZERO, 0, "\t\tERR_CAPACITY   = %u\n", (bool) (stk->err & ERR_CAPACITY));
    logPrint(L_ZERO, 0, "\t\tERR_LOGIC      = %u\n", (bool) (stk->err & ERR_LOGIC));
    ON_CANARY(
    logPrint(L_ZERO, 0, "\t\tERR_CANARY     = %u\n", (bool) (stk->err & ERR_CANARY));
    )
    ON_HASH(
    logPrint(L_ZERO, 0, "\t\tERR_HASH_DATA  = %u\n", (bool) (stk->err & ERR_HASH_DATA));
    logPrint(L_ZERO, 0, "\t\tERR_HASH_STACK = %u\n", (bool) (stk->err & ERR_HASH_STACK));
    )
    logPrint(L_ZERO, 0, "\t}\n");
    return true;
}

static bool stackDumpData(Stack_t *stk) {
    logPrint(L_ZERO, 0, "\tdata[%p] {\n", stk->data);
    if (!stk->data) {
        logPrint(L_ZERO, 0, "\t}\n");
        return true;
    }
    if (stk->err & ERR_HASH_STACK || stk->err & ERR_DATA) {
        logPrint(L_ZERO, 0, "\t!!!Data may be corrupted!!!\n");
        logPrint(L_ZERO, 0, "\t}\n");
        return true;
    }
    ON_CANARY(                                                                                                              \
    ullPair_t canaries = getCanaries((char*)stk->data - sizeof(canary_t),                                                   \
                                        getSizeWithCanary(stk->capacity * sizeof(stkElem_t)));                              \
    logPrint(L_ZERO, 0, "\t^ [ -1] %zX (DataCanary1)\n", canaries.first);                                                   \
    if (!canaryOk(canaries.first, (char*)stk->data - sizeof(canary_t)))                                                     \
        logPrint(L_ZERO, 0, "BROKEN:     %zX is correct canary\n", ((size_t)stk->data - sizeof(canary_t)) ^ XOR_CONST);     \
    )
    for (size_t index = 0; index < stk->size && index < stk->capacity; index++) {
        logPrint(L_ZERO, 0, "\t* ");
        if (memcmp(stk->data + index, &POISON_ELEM, sizeof(stkElem_t)) == 0)
            logPrint(L_ZERO, 0, "[%3d] %zX (POISON)\n", index, POISON_ELEM);
        else
            logPrint(L_ZERO, 0, "[%3d] " STK_ELEM_FMT "\n", index, stk->data[index]);
    }

    // 0 -> no poison elements
    // 1 -> encountered poison element
    // 2 -> poison elements are repeating; don't print them
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
    stackVerify(stk); //double checking errors
    #ifndef NDEBUG
    if (!stk->name || !stk->initFile)
        logPrint(L_ZERO, 0, "[%p], no initialization information\n", stk);
    else
        logPrint(L_ZERO, 0, "\"%s\"[%p], created at %s:%d \n", stk->name, stk, stk->initFile, stk->initLine);
    #else
    logPrint(L_ZERO, 0, "\[%p], use debug version for more info \n", stk);
    #endif
    logPrint(L_ZERO, 0, "{\n");

    ON_CANARY(
    logPrint(L_ZERO, 0, "\tCanary1  = %zX\n", stk->goose1);
    if (!canaryOk(stk->goose1, stk))
        logPrint(L_ZERO, 0, "\tBROKEN: must be %zX\n", (size_t)stk ^ XOR_CONST);
    )

    stackDumpErr(stk);

    logPrint(L_ZERO, 0, "\tsize     = %lu\n", stk->size);
    if (stk->size > MAX_STACK_SIZE)
        logPrint(L_ZERO, 0, "\t!!!SIZE OVERFLOW\n");
    logPrint(L_ZERO, 0, "\tcapacity = %lu\n", stk->capacity);
    if (stk->capacity > MAX_STACK_SIZE)
        logPrint(L_ZERO, 0, "\t!!!CAPACITY OVERFLOW\n");
    if (stk->size > stk->capacity)
        logPrint(L_ZERO, 0, "\t!!!SIZE > CAPACITY\n");

    stackDumpData(stk);

    ON_HASH(
    logPrint(L_ZERO, 0, "\tdataHash  = %#.16zX\n", stk->dataHash);
    if (stk->err & (ERR_DATA + ERR_HASH_STACK))
        logPrint(L_ZERO, 0, "\tData may be corrupted, can't calculate hash\n");
    else if (stk->dataHash != getDataHash(stk))
        logPrint(L_ZERO, 0, "\tWrong hash: %#.16zX is correct hash\n", getDataHash(stk));
    logPrint(L_ZERO, 0, "\tstackHash = %#.16zX\n");
    if (stk->stackHash != getStackHash(stk))
        logPrint(L_ZERO, 0, "\tWrong hash: %#.16zX is correct hash\n", getStackHash(stk));
    )

    ON_CANARY(
    logPrint(L_ZERO, 0, "\tCanary2   = %zX\n", stk->goose2);
    if (!canaryOk(stk->goose2, stk))
        logPrint(L_ZERO, 0, "\tBROKEN: must be %zX\n", (size_t)stk ^ XOR_CONST);
    )

    logPrint(L_ZERO, 0, "}\n");
    return true;
}

const char *stackFirstErrorToStr(StackError_t err) {
    if (err & ERR_DATA)
        return "ERR_DATA";
    if (err & ERR_SIZE)
        return "ERR_SIZE";
    if (err & ERR_CAPACITY)
        return "ERR_CAPACITY";
    if (err & ERR_LOGIC)
        return "ERR_LOGIC";
    ON_CANARY(
    if (err & ERR_CANARY)
        return "ERR_CANARY";
    )
    ON_HASH(
    if (err & ERR_HASH_DATA)
        return "ERR_HASH_DATA";
    if (err & ERR_HASH_STACK)
        return "ERR_HASH_STACK";
    )
    return "OK";
}


ON_HASH(
static uint64_t getDataHash(Stack_t *stk) {
    MY_ASSERT(stk, abort());
    return memHash(stk->data, stk->capacity*sizeof(stkElem_t));
}

static uint64_t getStackHash(Stack_t *stk) {
    const hash_t magicNumber = 1337;
    MY_ASSERT(stk, abort());
    uint64_t oldHash = stk->stackHash;
    stk->stackHash = magicNumber;
    uint64_t newHash = memHash(stk, sizeof(Stack_t));
    stk->stackHash = oldHash;
    return newHash;
}
)

ON_CANARY(
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
    return (8 - len % 8) % 8; // round up (len + 7) / 8
}

static size_t getSizeWithCanary(size_t len) {
    if (len == 0) return 0;
    return len + 2 * sizeof(canary_t) + getCanaryAlign(len); // 2 * sizeof(canary) + ROUNDUP(len, 8);
}

static bool canariesOk(void *data, size_t len, bool doOffset) {
    if (data == NULL) return true; //no canaries is ok
    if (doOffset) {
        data = (char*) data - sizeof(canary_t);
        len = getSizeWithCanary(len);
    }
    ullPair_t canaries = getCanaries(data, len);
    return canaryOk(canaries.first, data) && canaryOk(canaries.second, data);
}
)
