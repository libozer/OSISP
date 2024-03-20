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

struct Options {
    int showLinks;
    int showDirs;
    int showFiles;
    int isSortEnabled;
};

struct Options parseOptions(int argc, char *argv[]) {
    struct Options options = {0};

    int opt;
    while ((opt = getopt(argc, argv, "ldfs")) != -1) {
        switch (opt) {
            case 'l':
                options.showLinks = 1;
                break;
            case 'd':
                options.showDirs = 1;
                break;
            case 'f':
                options.showFiles = 1;
                break;
            case 's':
                options.isSortEnabled = 1;
                break;
            default:
                fprintf(stderr, "Usage: %s [-l] [-d] [-f] [-s] [path]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    // If no options specified, show all types
    if (!options.showLinks && !options.showDirs && !options.showFiles) {
        options.showLinks = options.showDirs = options.showFiles = 1;
    }

    return options;
}

void printFileInfo(const char *path, char type) {
    printf("%s | %s\n", path,
           (type == SYMBOLIC_LINK) ? "Symbolic Link" :
           (type == DIRECTORY) ? "Directory" :
           (type == REGULAR_FILE) ? "Regular File" : "Unknown Type");
}

int compare(const void *a, const void *b) {
    return strcoll(*(const char **)a, *(const char **)b);
}

void listFiles(const char *basePath, const struct Options *options);

void processDirectory(const char *basePath, const struct Options *options) {
    DIR *dir = opendir(basePath);
    if (!dir) {
        perror("Error opening directory");
        return;
    }

    char *files[PATH_MAX_LENGTH];
    int fileCount = 0;

    struct dirent *dp;
    while ((dp = readdir(dir)) != NULL) {
        if (strcmp(dp->d_name, ".") != 0 && strcmp(dp->d_name, "..") != 0) {
            char path[PATH_MAX_LENGTH];
            snprintf(path, sizeof(path), "%s/%s", basePath, dp->d_name);

            struct stat sb;
            if (lstat(path, &sb) == -1) {
                perror("Error getting file information");
                continue;
            }

            if (S_ISDIR(sb.st_mode)) {
                if (options->showDirs)
                    printFileInfo(path, DIRECTORY);
            } else if (S_ISLNK(sb.st_mode) && options->showLinks) {
                printFileInfo(path, SYMBOLIC_LINK);
            } else if (S_ISREG(sb.st_mode) && options->showFiles) {
                printFileInfo(path, REGULAR_FILE);
            }

            files[fileCount++] = strdup(path);
        }
    }

    closedir(dir);

    if (options->isSortEnabled) {
        qsort(files, fileCount, sizeof(char *), compare);
    }

    for (int i = 0; i < fileCount; i++) {
        printf("%s\n", files[i]);
        free(files[i]);
    }
}

int main(int argc, char *argv[]) {
    setlocale(LC_COLLATE, "ru_RU.UTF-8");

    struct Options options = parseOptions(argc, argv);

    char basePath[PATH_MAX_LENGTH] = ".";
    if (optind < argc) {
        strncpy(basePath, argv[optind], PATH_MAX_LENGTH - 1);
        basePath[PATH_MAX_LENGTH - 1] = '\0';
    }

    listFiles(basePath, &options);

    return 0;
}
