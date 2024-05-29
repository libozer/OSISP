#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "file_utils.h"
#include "thread_funcs.h"

int count = 0;
int countSecond = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexSecond = PTHREAD_MUTEX_INITIALIZER;
int* mapBlocks;
int check = 0;

int main(int argc, char *argv[]) {
    char command[256];
    snprintf(command, sizeof(command), "%s %s", "cat", argv[4]);
    mapBlocks = malloc(atoi(argv[2]) * sizeof(int));

    const char* filename = argv[4];
    int numChars = atoi(argv[1]);

    fillFileWithRandomChars(filename, numChars);
    printf("file before sort:\n\n");
    system(command);

    int threadCount = atoi(argv[3]);
    pthread_t threads[threadCount];

    for(int i = 0; i < threadCount; i++) {
        ThreadArgs* args = malloc(sizeof(ThreadArgs));
        args->fileSize = numChars;
        args->blockAmount = atoi(argv[2]);
        args->threadAmount = threadCount;
        args->filename = filename;

        int result = pthread_create(&threads[i], NULL, threadFunc, (void*)args);
        if (result != 0) {
            perror("pthread_create");
            exit(EXIT_FAILURE);
        }
    }

    for(int i = 0; i < threadCount; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("\n\nfile after sort:\n\n");
    system(command);
    printf("\n");
    free(mapBlocks);
    return 0;
}
