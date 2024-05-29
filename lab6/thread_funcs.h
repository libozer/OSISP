#ifndef THREAD_FUNCS_H
#define THREAD_FUNCS_H

typedef struct {
    int fileSize;
    int blockAmount;
    int threadAmount;
    const char* filename;
} ThreadArgs;

void* threadFunc(void* args);

#endif // THREAD_FUNCS_H
