#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "file_utils.h"
#include "thread_funcs.h"

extern int count;
extern int countSecond;
extern pthread_mutex_t mutex;
extern pthread_mutex_t mutexSecond;
extern int* mapBlocks;
extern int check;

void* threadFunc(void* args) {
    ThreadArgs* threadArgs = (ThreadArgs*)args;

    for (int i = 0; i < threadArgs->blockAmount; i++) {
        mapBlocks[i] = 0;
    }

    pthread_mutex_lock(&mutex);
    count++;
    pthread_mutex_unlock(&mutex);

    while (count != threadArgs->threadAmount);

    int current = -1;
    while (1) {
        pthread_mutex_lock(&mutex);
        for (int i = 0; i < threadArgs->blockAmount; i++) {
            if (mapBlocks[i] == 0) {
                check++;
                mapBlocks[i] = 1;
                current = i + 1;
                break;
            } else {
                current = -1;
            }
        }
        pthread_mutex_unlock(&mutex);

        sortBlock(current, threadArgs->fileSize, threadArgs->blockAmount, threadArgs->filename);
        if (check == threadArgs->blockAmount) break;
    }

    pthread_mutex_lock(&mutexSecond);
    countSecond++;
    pthread_mutex_unlock(&mutexSecond);

    while (countSecond != threadArgs->threadAmount);

    current = -1;
    int notDone = 1;
    while (1) {
        while (1) {
            pthread_mutex_lock(&mutex);
            for (int i = 0; i < threadArgs->blockAmount; i++) {
                if (mapBlocks[i] == notDone) {
                    mapBlocks[i] = notDone + 1;
                    mapBlocks[i + 1] = notDone + 1;
                    current = i + 1;
                    break;
                } else {
                    current = -1;
                }
            }
            pthread_mutex_unlock(&mutex);
            mergeBlocks(current, threadArgs->fileSize, threadArgs->blockAmount, threadArgs->filename);
            if (current == -1) break;
        }
        threadArgs->blockAmount /= 2;
        notDone++;

        if (threadArgs->blockAmount == 1) break;
    }

    pthread_exit(NULL);
}
