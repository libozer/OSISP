#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>
#include <getopt.h>
#include <locale.h>
#include <stdlib.h>

#define MAX_PATH_LENGTH 1000

enum FileTypes {
    FILE_REGULAR = 'f',
    FILE_SYMBOLIC_LINK = 'l',
    FILE_DIRECTORY = 'd',
    FILE_ALL_TYPES = '-'
};

int compareStrings(const void *a, const void *b) {
    return strcoll(*(const char **)a, *(const char **)b);
}

void printFileDetails(const char *filePath, char type) {
    printf("%s | %s\n", filePath,
           (type == FILE_SYMBOLIC_LINK) ? "Символическая ссылка" :
           (type == FILE_DIRECTORY) ? "Директория" :
           (type == FILE_REGULAR) ? "Обычный файл" : "Неизвестный тип");
}

void listFiles(const char *basePath, const char *fileTypes, int isSortEnabled) {
    DIR *directory = opendir(basePath);

    if (!directory) {
        perror("Ошибка открытия каталога");
        return;
    }

    struct dirent *dirEntry;
    struct stat fileStat;

    int fileCount = 0;
    char **filePaths = NULL;

    while ((dirEntry = readdir(directory)) != NULL) {
        if (strcmp(dirEntry->d_name, ".") != 0 && strcmp(dirEntry->d_name, "..") != 0) {
            char path[MAX_PATH_LENGTH];
            snprintf(path, sizeof(path), "%s/%s", basePath, dirEntry->d_name);

            if (lstat(path, &fileStat) == -1) {
                perror("Ошибка получения информации о файле");
                continue;
            }

            int isTypeMatched = 0;

            if (fileTypes[0] == FILE_SYMBOLIC_LINK && S_ISLNK(fileStat.st_mode)) {
                isTypeMatched = 1;
            } else if (fileTypes[1] == FILE_DIRECTORY && S_ISDIR(fileStat.st_mode)) {
                isTypeMatched = 1;
            } else if (fileTypes[2] == FILE_REGULAR && S_ISREG(fileStat.st_mode)) {
                isTypeMatched = 1;
            }

            if (isTypeMatched) {
                if (isSortEnabled) {
                    filePaths = realloc(filePaths, (fileCount + 1) * sizeof(char *));
                    filePaths[fileCount] = strdup(path);
                    fileCount++;
                } else {
                    printFileDetails(path, (S_ISLNK(fileStat.st_mode)) ? FILE_SYMBOLIC_LINK :
                                        (S_ISDIR(fileStat.st_mode)) ? FILE_DIRECTORY :
                                        (S_ISREG(fileStat.st_mode)) ? FILE_REGULAR : FILE_ALL_TYPES);
                }
            }
        }
    }

    closedir(directory);

    if (isSortEnabled) {
        qsort(filePaths, fileCount, sizeof(char *), compareStrings);
        for (int i = 0; i < fileCount; i++) {
            char path[MAX_PATH_LENGTH];
            strncpy(path, filePaths[i], MAX_PATH_LENGTH - 1);
            path[MAX_PATH_LENGTH - 1] = '\0';

            struct stat fileStat;
            if (lstat(path, &fileStat) == -1) {
                perror("Ошибка получения информации о файле");
                continue;
            }
            printFileDetails(path, (S_ISLNK(fileStat.st_mode)) ? FILE_SYMBOLIC_LINK :
                                (S_ISDIR(fileStat.st_mode)) ? FILE_DIRECTORY :
                                (S_ISREG(fileStat.st_mode)) ? FILE_REGULAR : FILE_ALL_TYPES);
            free(filePaths[i]);
        }
        free(filePaths);
    }

    directory = opendir(basePath);

    if (!directory) {
        perror("Ошибка повторного открытия каталога");
        return;
    }

    while ((dirEntry = readdir(directory)) != NULL) {
        if (strcmp(dirEntry->d_name, ".") != 0 && strcmp(dirEntry->d_name, "..") != 0) {
            char path[MAX_PATH_LENGTH];
            snprintf(path, sizeof(path), "%s/%s", basePath, dirEntry->d_name);

            if (lstat(path, &fileStat) == -1) {
                perror("Ошибка получения информации о файле");
                continue;
            }

            if (S_ISDIR(fileStat.st_mode)) {
                listFiles(path, fileTypes, isSortEnabled);
            }
        }
    }

    closedir(directory);
}

int main(int argc, char *argv[]) {
    setlocale(LC_COLLATE, "ru_RU.UTF-8");

    char basePath[MAX_PATH_LENGTH] = ".";
    int isSortEnabled = 0;

    int showLinks = 0, showDirs = 0, showFiles = 0;

    int opt;
    int pathIndex = -1;

    for (int i = 1; i < argc; i++) {
        if (argv[i][0] != '-') {
            pathIndex = i;
            break;
        }
    }
    if (pathIndex != -1) {
        strncpy(basePath, argv[pathIndex], MAX_PATH_LENGTH - 1);
        basePath[MAX_PATH_LENGTH - 1] = '\0';
        for (int i = pathIndex; i < argc - 1; i++) {
            argv[i] = argv[i + 1];
        }
        argc--;
    }
    while ((opt = getopt(argc, argv, "ldfs")) != -1) {
        switch (opt) {
            case 'l':
                showLinks = 1;
                break;
            case 'd':
                showDirs = 1;
                break;
            case 'f':
                showFiles = 1;
                break;
            case 's':
                isSortEnabled = 1;
                break;
            default:
                fprintf(stderr, "Использование: %s [-l] [-d] [-f] [-s] [путь]\n", argv[0]);
                return 1;
        }
    }

    if (!showLinks && !showDirs && !showFiles) {
        showLinks = showDirs = showFiles = 1;
    }

    char fileTypes[4] = {showLinks ? FILE_SYMBOLIC_LINK : '\0', showDirs ? FILE_DIRECTORY : '\0', showFiles ? FILE_REGULAR : '\0', '\0'};

    listFiles(basePath, fileTypes, isSortEnabled);

    return 0;
}
