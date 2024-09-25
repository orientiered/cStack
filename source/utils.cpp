#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "utils.h"

long long maxINT(long long a, long long b) {
    return (a > b) ? a : b;
}

long long minINT(long long a, long long b) {
    return (a > b) ? b : a;
}

void swap(void* a, void* b, size_t len) {
    //checking if a and b are correctly aligned
    if ((((size_t) a) % 8) != (((size_t) b) % 8)) { //TODO: sizeof(long long/uint64_t)
        swapByByte(a, b, len);
        return;
    }
    //aligning a and b if possible
    const unsigned blockSize = sizeof(long long); //TODO:uint64_t instead of long long
    const unsigned startOffset = (blockSize - ((size_t) a % 8)) % 8;
    swapByByte(a, b, startOffset);

    size_t llSteps = (len-startOffset) / sizeof(long long);
    //swapping every 8 bytes
    long long *lla = (long long*) ((size_t)a + startOffset), *llb = (long long*) ((size_t)b + startOffset);
    long long temp = 0;

    while (llSteps--) {
        temp = *lla;
        *lla++ = *llb;
        *llb++ = temp;
    }

    swapByByte(lla, llb, (len-startOffset) % 8);
}

void swapByByte(void* a, void* b, size_t len) {
    char *ac = (char*) a, *bc = (char*) b;
    char c = 0;
    while (len--) {
        c = *ac;
        *ac++ = *bc;
        *bc++ = c;
    }
}


void memcpy(void* copyTo, void* copyFrom, size_t length) {
    //checking if to and from are correctly aligned
    if (((size_t)copyTo % 8) != ((size_t)copyFrom % 8)) {
        memcpyByByte(copyTo, copyFrom, length);
        return;
    }
    //aligning to and from if possible
    const unsigned blockSize = sizeof(long long);
    const unsigned startOffset = (blockSize - ((size_t) copyTo % 8)) % 8;
    memcpyByByte(copyTo, copyFrom, startOffset);

    size_t llSteps = (length-startOffset) / sizeof(long long);
    //copying every 8 bytes
    long long   *llTo   = (long long*) ((size_t)copyTo    + startOffset),
                *llFrom = (long long*) ((size_t)copyFrom  + startOffset);

    while (llSteps--) *llTo++ = *llFrom++;

    memcpyByByte(llTo, llFrom, (length-startOffset) % 8);
}

void memcpyByByte(void *copyTo, void* copyFrom, size_t length) {
    char *to = (char*)copyTo, *from = (char*)copyFrom;
    while (length--) *to++ = *from++;
}

doublePair_t runningSTD(double value, int getResult) {
    //function to calculate standard deviation of some value
    //constructed to make calculations online, so static variables
    static doublePair_t result = {};
    static unsigned measureCnt = 0;     //number of values
    static double totalValue = 0;       //sum of value
    static double totalSqrValue = 0;    //sum of value^2
    // getResult > 1 --> calculate meanValue and std and return it
    // getResult = 0 --> store current value
    // getResult < 0 --> reset stored values
    if (getResult > 0) {
        if (measureCnt > 1) {
            result.first = totalValue / measureCnt;
            //printf("%g %g %u\n", totalSqrValue, totalValue, measureCnt);
            result.second = sqrt(totalSqrValue / measureCnt - result.first*result.first) / sqrt(measureCnt - 1);
        }
        return result;
    } else if (getResult == 0) {
        measureCnt++;
        totalValue += value;
        totalSqrValue += value*value;
    } else {
        measureCnt = 0;
        totalValue = totalSqrValue = 0;
    }
    return result;
}
