#define _GNU_SOURCE
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <semaphore.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <inttypes.h>
#include <time.h>
#include <pthread.h>

#define MAX_LEN 256
#define MAX_DATA (((MAX_LEN + 3) / 4) * 4)
#define MAX_THREADS 4096
#define RING_CAPACITY 512

typedef struct message{
  uint8_t type;
  uint16_t hash;
  uint8_t size;
  uint8_t data[MAX_DATA];
} message;                       // структура сообщения

typedef struct queue{
  int added_count;
  int extracted_count;
  int head;                     
  int tail;
  message buffer[RING_CAPACITY + 1];
} queue;                         // структура кольцевого буфера