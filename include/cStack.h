#ifndef C_STACK_H
#define C_STACK_H

typedef double stkElem_t;

enum StackError {
    ERR_NO = 0,
    ERR_DATA,
    ERR_SIZE,
    ERR_CAPACITY,
    ERR_LOGIC
};

typedef struct {
    enum StackError err;
    stkElem_t *data;
    size_t size;
    size_t capacity;
} Stack_t;

Stack_t stackCtor(size_t startCapacity);
enum StackError stackDtor(Stack_t *stk);

enum StackError stackPush(Stack_t *stk, stkElem_t val);
stkElem_t stackPop(Stack_t *stk);

#endif
