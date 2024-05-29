#ifndef FILE_UTILS_H
#define FILE_UTILS_H

void fillFileWithRandomChars(const char* filename, int numChars);
void sortBlock(int block, int fileSize, int blockAmount, const char* filename);
void mergeBlocks(int block, int fileSize, int blockAmount, const char* filename);
int compare(const void* a, const void* b);

#endif // FILE_UTILS_H
