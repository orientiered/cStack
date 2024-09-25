#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "utils.h"
#include "mystring.h"
//#include <string.h>
char *strcpy(char *s, const char *ct) {
    char *s_copy = s;
    while ((*s++=*ct++) != '\0')
        ;
    return s_copy;
}

char *strncpy(char *s, const char *ct, size_t n) {
    char *s_copy = s;
    while (n && (*s++ = *ct++) != '\0')n--;
    if (!n) *s = '\0';
    return s_copy;
}

char *strcat(char *s, const char *ct) {
    char *s_copy = s;
    while (*s) s++;
    while ((*s++ = *ct++) != '\0')
        ;
    return s_copy;
}

char *strncat(char *s, const char *ct, size_t n) { // TODO: K&R is not always the best naming reference
    char *s_copy = s;
    while (*s) s++;
    while (n && (*s++ = *ct++) != '\0') n--;
    if (!n) *s = '\0';
    return s_copy;
}

int strcmp(const char *firstStr, const char *secondStr) {
    for (; *firstStr && (*firstStr == *secondStr); firstStr++, secondStr++);

    return int(*firstStr) - int(*secondStr);
}

int strcmpBackward(const char *firstStr, const char *secondStr) {
    const char *firstStart = firstStr, *secondStart = secondStr;
    //slow, basic version
    //works on any strings without additional info
    firstStr  += maxINT((long long)strlen(firstStr)  - 1, 0);
    secondStr += maxINT((long long)strlen(secondStr) - 1, 0);

    for (; (firstStr > firstStart) && (secondStr > secondStart) && (*firstStr == *secondStr); firstStr--, secondStr--)
        ;
    if ((*firstStr == *secondStr) && ((firstStr == firstStart) ^ (secondStr == secondStart)))
        return int(firstStr - firstStart) - int(secondStr - secondStart);
    return int(*firstStr) - int(*secondStr);
}

int stralphacmp(const char *firstStr, const char *secondStr) {
    while (*firstStr && *secondStr) {
        firstStr = findAlphabetChar(firstStr);
        secondStr = findAlphabetChar(secondStr);

        if (tolower(*firstStr) != tolower(*secondStr))
            return tolower(*firstStr) - tolower(*secondStr);
        if (*firstStr)  firstStr++;
        if (*secondStr) secondStr++;
    }
    //firstStr = findAlphabetChar(firstStr);
    //secondStr = findAlphabetChar(secondStr);
    return tolower(*firstStr) - tolower(*secondStr);
}

int stralphacmpBackward(const char *firstStr, const char *secondStr) {
    const char *firstStart = firstStr, *secondStart = secondStr;
    firstStr  += maxINT((long long)strlen(firstStr)  - 1, 0);
    secondStr += maxINT((long long)strlen(secondStr) - 1, 0);

    while (firstStr > firstStart && secondStr > secondStart) {
        firstStr = findAlphabetCharBackward(firstStr, firstStart);
        secondStr = findAlphabetCharBackward(secondStr, secondStart);
        if (tolower(*firstStr) != tolower(*secondStr))
            return tolower(*firstStr) - tolower(*secondStr);
        if (firstStr > firstStart) firstStr--;
        if (secondStr > secondStart) secondStr--;
    }
    firstStr = findAlphabetCharBackward(firstStr, firstStart);
    secondStr = findAlphabetCharBackward(secondStr, secondStart);
    if ((tolower(*firstStr) == tolower(*secondStr)) && ((firstStr == firstStart) ^ (secondStr == secondStart)))
        return int(firstStr - firstStart) - int(secondStr - secondStart);
    return tolower(*firstStr) - tolower(*secondStr);
}


const char *findAlphabetChar(const char *str) {
    while (*str && !isalpha(*str)) str++;
    return str;
}

const char *findAlphabetCharBackward(const char *str, const char *end) {
    while (str > end && !isalpha(*str)) str--;
    return str;
}

int strncmp(const char *s, const char *t, size_t n) {
    for (;n && *s && (*s == *t); s++, t++, n--);

    if (*s != *t)
        return int(*s) - *t;
    return 0;
}

char *strchr(char *s, char c) {
    while (*s != c && *s) s++;
    if (*s == c)
        return s;
    else
        return NULL;
}

char *strrchr(char *s, char c) {
    const char *start = s;
    while (*s) s++;
    while (*s != c && s > start) --s;
    if (*s == c)
        return s;
    else
        return NULL;
}

const char* strstrPrefix(const char* text, const char* str) {
    size_t textSize = strlen(text);
    size_t strSize =  strlen(str);

    if (strSize > textSize) return NULL;

    size_t* prefix = (size_t*) calloc(strSize, sizeof(size_t)); // TODO: calloc in strstr??
    prefix[0] = 0;
    for (size_t i = 1; i < strSize; i++) {
        size_t curLen = prefix[i-1];
        while (curLen > 0 && str[i] != str[curLen])
            curLen = prefix[curLen-1];
        if (str[i] == str[curLen])
            curLen++;
        prefix[i] = curLen;
    }

    size_t curLen = 0;
    for (size_t i = 0; i < textSize; i++) {
        while (curLen > 0 && text[i] != str[curLen])
            curLen = prefix[curLen-1];

        if (text[i] == str[curLen])
            curLen++;

        if (curLen == strSize) {
            free(prefix);
            return text + i - strSize + 1;
        }
    }

    free(prefix);
    return NULL;
}

size_t strlen(const char *s) {
    size_t len = 0;
    while (*s++) len++;
    return len;
}

char* joinStrings(const char **strings, size_t len, const char *separator) {
    size_t fullLen = 0;
    for (size_t idx = 0; idx < len; idx++)
        fullLen += strlen(strings[idx]);
    fullLen += strlen(separator) * (len-1);
    fullLen += 1;
    char *joined = (char*) calloc(fullLen, sizeof(char));
    char *writePtr = joined;
    for (size_t idx = 0; idx < (len - 1); idx++) {
        for (const char *strPtr = strings[idx]; *strPtr; strPtr++)
            *writePtr++ = *strPtr;
        for (const char *sepPtr = separator; *sepPtr; sepPtr++)
            *writePtr++ = *sepPtr;
    }
    for (const char *strPtr = strings[len-1]; *strPtr; strPtr++)
            *writePtr++ = *strPtr;
    *writePtr = '\0';
    return joined;
}
