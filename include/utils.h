/// @file
/// @brief
#ifndef UTILS_H
#define UTILS_H

#define FREE(ptr) do {free(ptr); ptr = NULL;} while (0)
#define ARRAY_SIZE(array) (sizeof(array) / sizeof(*array))

/*------------------STRUCTS DEFINITIONS---------------------------------------*/

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

/*------------------SIMPLE AND CONVENIENT FUNCTIONS---------------------------*/

long long maxINT(long long a, long long b);
long long minINT(long long a, long long b);

void swap(void* a, void* b, size_t len);
void swapByByte(void* a, void* b, size_t len);

/// @brief memset with multiple byte values
void memValSet(void *start, const void *elem, size_t elemSize, size_t length);

/// @brief Incrementally compute standard deviation
/// @return Pair with mean value and its delta
/// getResult > 1 --> calculate meanValue and std and return it <br>
/// getResult = 0 --> store current value <br>
/// getResult < 0 --> reset stored values <br>
doublePair_t runningSTD(double value, int getResult);

/// @brief djb2 hash for any data
uint64_t memHash(const void *arr, size_t len);

#endif
