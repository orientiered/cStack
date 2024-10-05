#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "error_debug.h"
#include "logger.h"
#include "cStack.h"
#include "argvProcessor.h"

void test1();
void test2();

int main(int argc, const char *argv[]) {
    logOpen();
    setLogLevel(L_EXTRA);

    registerFlag(TYPE_BLANK, "-r", "--remove", "Delete old log file");
    processArgs(argc, argv);
    if (isFlagSet("-r")) {
        system("rm log.txt");
        logOpen();
    }

    test1();
    test2();
    logClose();
}

void test1() {
    Stack_t stk = {};
    stackCtor(&stk, 100);
    stackPush(&stk, 100);
    stackPush(&stk, 200);
    stackDump(&stk);
    for (int i = 0; i < 100; i++) {
        stackPush(&stk, 5*i);
    }
    for (int i = 0; i < 100; i++) {
        stackPop(&stk);
        if (i == 47) stackDump(&stk);
    }
    logPrint(L_ZERO, 1, "Val: %d\n", stackPop(&stk));
    logPrint(L_ZERO, 1, "Val: %d\n", stackPop(&stk));
    stackDtor(&stk);
    stackCtor(&stk, 100);
    stackDtor(&stk);
}

void test2() {
    Stack_t *stks = (Stack_t*) calloc(100, sizeof(Stack_t));
    for (int i = 0; i < 100; i++)
        stackCtor(&stks[i], i);

    for (int i = 0; i < 100; i++)
        stackPush(&stks[i], i+0.1235);

    for (int i = 0; i < 100; i++)
        stackDtor(&stks[i]);
    free(stks);
}
