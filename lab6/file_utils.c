#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <time.h>
#include "file_utils.h"

void fillFileWithRandomChars(const char* filename, int numChars) {
    int fd = open(filename, O_RDWR | O_CREAT, 0666);
    if (fd == -1) {
        printf("error 6\n");
        return;
    }

    size_t fileSize = sizeof(char) * numChars;
    if (ftruncate(fd, fileSize) == -1) {
        printf("error 5\n");
        close(fd);
        return;
    }

    char* fileData = mmap(NULL, fileSize, PROT_WRITE, MAP_SHARED, fd, 0);
    if (fileData == MAP_FAILED) {
        printf("error 4\n");
        close(fd);
        return;
    }

    srand(time(NULL));
    for (int i = 0; i < numChars; i++) {
        fileData[i] = 'A' + rand() % 26;
    }

    if (munmap(fileData, fileSize) == -1) {
        printf("error 3\n");
    }
    close(fd);
}

int compare(const void* a, const void* b) {
    char char1 = *((char*)a);
    char char2 = *((char*)b);
    return char1 - char2;
}

void sortBlock(int block, int fileSize, int blockAmount, const char* filename) {
    if (block == -1) return;

    size_t offset = (fileSize / blockAmount) * (block - 1); //start
    size_t endBlock = (fileSize / blockAmount) * block;     //end

    FILE* file = fopen(filename, "r+");
    if (file == NULL) {
        printf("error 1\n");
        return;
    }

    fseek(file, offset, SEEK_SET);  // * move to the start block
    size_t blockElements = endBlock - offset;
    char* blockData = (char*)malloc(blockElements * sizeof(char));
    if (blockData == NULL) {
        printf("error 7\n");
        fclose(file);
        return;
    }

    fread(blockData, sizeof(char), blockElements, file);
    qsort(blockData, blockElements, sizeof(char), compare);
    fseek(file, offset, SEEK_SET);
    fwrite(blockData, sizeof(char), blockElements, file);

    free(blockData);
    fclose(file);
}

void mergeBlocks(int block, int fileSize, int blockAmount, const char* filename) {
    if (block == -1) return;

    size_t offset = (fileSize / blockAmount) * (block - 1);
    size_t endBlock = (fileSize / blockAmount) * block;

    FILE* file = fopen(filename, "r+");
    if (file == NULL) {
        printf("error 1\n");
        return;
    }

    fseek(file, offset, SEEK_SET);
    size_t blockElements = endBlock - offset;
    char* blockDataFirst = (char*)malloc(blockElements * sizeof(char));
    if (blockDataFirst == NULL) {
        printf("error 7\n");
        fclose(file);
        return;
    }
    fread(blockDataFirst, sizeof(char), blockElements, file);

    fseek(file, offset + blockElements, SEEK_SET);  //second block
    char* blockDataSecond = (char*)malloc(blockElements * sizeof(char));
    if (blockDataSecond == NULL) {
        printf("error 7\n");
        fclose(file);
        return;
    }
    fread(blockDataSecond, sizeof(char), blockElements, file);

    char* blockDataFinal = (char*)malloc(2 * blockElements * sizeof(char));
    int j = 0;
    size_t first = 0, second = 0;

    while (first < blockElements && second < blockElements) {
        if (blockDataFirst[first] < blockDataSecond[second]) {
            blockDataFinal[j++] = blockDataFirst[first++];
        } else {
            blockDataFinal[j++] = blockDataSecond[second++];
        }
    }

    while (first < blockElements) {
        blockDataFinal[j++] = blockDataFirst[first++];
    }

    while (second < blockElements) {
        blockDataFinal[j++] = blockDataSecond[second++];
    }

    fseek(file, offset, SEEK_SET);
    fwrite(blockDataFinal, sizeof(char), 2 * blockElements, file);

    free(blockDataFirst);
    free(blockDataSecond);
    free(blockDataFinal);
    fclose(file);
}
