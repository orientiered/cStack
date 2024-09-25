#include <stdio.h>
#include <stdlib.h>

#include "error_debug.h"
#include "logger.h"
#include "cStack.h"

int main() {
    logOpen();
    setLogLevel(L_EXTRA);
    Stack_t stk = stackCtor(10);
    stackPush(&stk, 100);
    stackPush(&stk, 200);
    for (int i = 0; i < 100; i++) {
        stackPush(&stk, 5*i);
    }
    for (int i = 0; i < 100; i++) {
        stackPop(&stk);
    }
    logPrint(L_ZERO, 1, "Val: %lf\n", stackPop(&stk));
    logPrint(L_ZERO, 1, "Val: %lf\n", stackPop(&stk));
    stackDtor(&stk);
    logClose();
}
