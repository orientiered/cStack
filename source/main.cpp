#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "error_debug.h"
#include "logger.h"
#include "cStack.h"

int main() {
    logOpen();
    setLogLevel(L_DEBUG);
    Stack_t stk = {};
    stackCtor(&stk, 0);
    stackPush(&stk, 100);
    stackPush(&stk, 200);
    //stk.data[1] = 5;
    for (int i = 0; i < 100; i++) {
        stackPush(&stk, 5*i);
    }
    ((char*)stk.data)[-3] = 25;
    for (int i = 0; i < 100; i++) {
        stackPop(&stk);
    }
    logPrint(L_ZERO, 1, "Val: %lf\n", stackPop(&stk));
    logPrint(L_ZERO, 1, "Val: %lf\n", stackPop(&stk));
    stackDtor(&stk);
    logClose();
}
