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

    setHelpMessageHeader("Hello\n");
    registerFlag(TYPE_BLANK, "-h", "--help", "Prints help message");
    registerFlag(TYPE_STRING, "-s", "--string", "Get string");
    processArgs(argc, argv);

    if (isFlagSet("-h")) printHelpMessage();
    if (isFlagSet("-s")) printf("%s\n", getFlagValue("-s").string_);
    test1();
    test2();
    logClose();
}

void test1() {
    Stack_t stk = {};
    stackCtor(&stk, 1);
    stackPush(&stk, 100);
    stackPush(&stk, 200);
    //stk.data[1] = 5;
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
