#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>
#include <getopt.h>
#include <locale.h>
#include <stdlib.h>

#define PATH_MAX_LENGTH 1000

enum FileTypes {
    REGULAR_FILE = 'f',
    SYMBOLIC_LINK = 'l',
    DIRECTORY = 'd',
    ALL_TYPES = '-'
};

int compareStrings(const void *a, const void *b) {
    return strcoll(*(const char **)a, *(const char **)b);
}
void printFileInfo(const char *path, char type) {
    printf("%s | %s\n", path,
           (type == SYMBOLIC_LINK) ? "Symbolic Link" :
           (type == DIRECTORY) ? "Directory" :
           (type == REGULAR_FILE) ? "Regular File" : "Unknown Type");
}

void listFiles(const char *basePath, const char *fileTypes, int isSortEnabled) {
    DIR *dir = opendir(basePath);

    if (!dir) {
        perror("Error opening directory");
        return;
    }

    struct dirent *dp;
    struct stat sb;

    int fileCount = 0;
    char **files = NULL;

    while ((dp = readdir(dir)) != NULL) {
        if (strcmp(dp->d_name, ".") != 0 && strcmp(dp->d_name, "..") != 0) {
            char path[PATH_MAX_LENGTH];
            snprintf(path, sizeof(path), "%s/%s", basePath, dp->d_name);

            if (lstat(path, &sb) == -1) {
                perror("Error getting file information");
                continue;
            }

            int isTypeMatched = 0;

            if (fileTypes[0] == SYMBOLIC_LINK && S_ISLNK(sb.st_mode)) {
                isTypeMatched = 1;
            } else if (fileTypes[1] == DIRECTORY && S_ISDIR(sb.st_mode)) {
                isTypeMatched = 1;
            } else if (fileTypes[2] == REGULAR_FILE && S_ISREG(sb.st_mode)) {
                isTypeMatched = 1;
            }

            if (isTypeMatched) {
                if (isSortEnabled) {
                    files = realloc(files, (fileCount + 1) * sizeof(char *));
                    files[fileCount] = strdup(path);
                    fileCount++;
                } else {
                    printFileInfo(path, (S_ISLNK(sb.st_mode)) ? SYMBOLIC_LINK :
                                        (S_ISDIR(sb.st_mode)) ? DIRECTORY :
                                        (S_ISREG(sb.st_mode)) ? REGULAR_FILE : ALL_TYPES);
                }
            }
        }
    }

    closedir(dir);

    if (isSortEnabled) {
        qsort(files, fileCount, sizeof(char *), compareStrings);
        for (int i = 0; i < fileCount; i++) {
            char path[PATH_MAX_LENGTH];
            strncpy(path, files[i], PATH_MAX_LENGTH - 1);
            path[PATH_MAX_LENGTH - 1] = '\0';

            struct stat sb;
            if (lstat(path, &sb) == -1) {
                perror("Error getting file information");
                continue;
            }
            printFileInfo(path, (S_ISLNK(sb.st_mode)) ? SYMBOLIC_LINK :
                                (S_ISDIR(sb.st_mode)) ? DIRECTORY :
                                (S_ISREG(sb.st_mode)) ? REGULAR_FILE : ALL_TYPES);
            free(files[i]);
        }
        free(files);
    }

    dir = opendir(basePath);

    if (!dir) {
        perror("Error reopening directory");
        return;
    }

    while ((dp = readdir(dir)) != NULL) {
        if (strcmp(dp->d_name, ".") != 0 && strcmp(dp->d_name, "..") != 0) {
            char path[PATH_MAX_LENGTH];
            snprintf(path, sizeof(path), "%s/%s", basePath, dp->d_name);

            if (lstat(path, &sb) == -1) {
                perror("Error getting file information");
                continue;
            }

            if (S_ISDIR(sb.st_mode)) {
                listFiles(path, fileTypes, isSortEnabled);
            }
        }
    }

    closedir(dir);
}
int main(int argc, char *argv[]) {
    setlocale(LC_COLLATE, "ru_RU.UTF-8");

    char basePath[PATH_MAX_LENGTH] = ".";
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
        strncpy(basePath, argv[pathIndex], PATH_MAX_LENGTH - 1);
        basePath[PATH_MAX_LENGTH - 1] = '\0';
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
                fprintf(stderr, "Usage: %s [-l] [-d] [-f] [-s] [path]\n", argv[0]);
                return 1;
        }
    }

    if (!showLinks && !showDirs && !showFiles) {
        showLinks = showDirs = showFiles = 1;
    }

    char fileTypes[4] = {showLinks ? SYMBOLIC_LINK : '\0', showDirs ? DIRECTORY : '\0', showFiles ? REGULAR_FILE : '\0', '\0'};

    listFiles(basePath, fileTypes, isSortEnabled);

    return 0;
}