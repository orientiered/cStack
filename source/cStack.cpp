#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "error_debug.h"
#include "logger.h"
#include "cStack.h"

const size_t DEALLOC_MIN_SIZE = 5;
enum StackSizeOp {
    OP_PUSH = 1,
    OP_POP = -1
};

static StackError stackChangeSize(Stack_t *stk, enum StackSizeOp op);

static StackError stackChangeSize(Stack_t *stk, enum StackSizeOp op) {
    MY_ASSERT(stk, abort());
    logPrint(L_EXTRA, 0, "Changing size %d\n", op);
    switch(op) {
    case OP_PUSH:
        if (stk->size == stk->capacity) {
            size_t newCapacity = 2 * stk->capacity;
            logPrint(L_DEBUG, 0, "Reallocating up: %lu --> %lu\n", stk->capacity, newCapacity);
            stk->data = (stkElem_t*) realloc(stk->data, newCapacity * sizeof(stkElem_t));
            memset(stk->data + stk->capacity, 0, newCapacity - stk->capacity);
            stk->capacity = newCapacity;
        }
        stk->size++;
        break;
    case OP_POP:
        if (stk->size == 0)
            return ERR_SIZE;

        stk->size--;
        if (stk->size > DEALLOC_MIN_SIZE &&
            (4 * stk->size) < stk->capacity) {
            size_t newCapacity = stk->capacity / 2;
            logPrint(L_DEBUG, 0, "Reallocating down\n");
            stk->data = (stkElem_t*) realloc(stk->data, newCapacity * sizeof(stkElem_t));
            stk->capacity = newCapacity;
        }
        break;
    default:
        return ERR_LOGIC;
    }
    return ERR_NO;
}

Stack_t stackCtor(size_t startCapacity) {
    Stack_t stk = {};
    stk.size = 0;
    stk.capacity = startCapacity;
    if (startCapacity == 0)
        stk.data = NULL;
    else
        stk.data = (stkElem_t *) calloc(startCapacity, sizeof(stkElem_t));

    if (!stk.data) stk.err = ERR_DATA;

    return stk;
}

enum StackError stackDtor(Stack_t *stk) {
    MY_ASSERT(stk, abort());
    free(stk->data);
    return ERR_NO;
}

enum StackError stackPush(Stack_t *stk, stkElem_t val) {
    MY_ASSERT(stk, abort());

    stackChangeSize(stk, OP_PUSH);

    stk->data[stk->size-1] = val;
    logPrint(L_EXTRA, 0, "Pushing %lf\n", val);
    return ERR_NO;
}

stkElem_t stackPop(Stack_t *stk) {
    MY_ASSERT(stk, abort());
    logPrint(L_EXTRA, 0, "Popping element: size = %lu\n", stk->size);
    MY_ASSERT((stk->size > 0), abort());
    stkElem_t val = stk->data[stk->size-1];
    stackChangeSize(stk, OP_POP);
    return val;
}
