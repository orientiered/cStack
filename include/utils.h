/// @file
/// @brief
#ifndef UTILS_H
#define UTILS_H

#define FREE(p) {free(p); p = NULL;}
#define ARRAY_SIZE(array) (sizeof(array) / sizeof(*array))

typedef struct doublePair {
    double first;
    double second;
} doublePair_t;

typedef struct llPair {
    long long first;
    long long second;
} llPair_t;

typedef struct ullPair {
    unsigned long long first;
    unsigned long long second;
} ullPair_t;

typedef struct voidPtrPair {
    void *first;
    void *second;
} voidPtrPair_t;

long long maxINT(long long a, long long b);
long long minINT(long long a, long long b);

void swap(void* a, void* b, size_t len);
void swapByByte(void* a, void* b, size_t len);

void memcpy(void* copyTo, void* copyFrom, size_t length);
void memcpyByByte(void* copyTo, void* copyFrom, size_t length);

doublePair_t runningSTD(double value, int getResult);

#endif
