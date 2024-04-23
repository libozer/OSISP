#define _GNU_SOURCE

#include <sys/wait.h>
#include <sys/prctl.h>
#include <sys/ipc.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <time.h>

#define QUEUE_MAX_SIZE 15
#define MAX_CHILD 5

sem_t *taken_msgs, *free_msgs, *mutex;

pid_t consumers[MAX_CHILD];
char *consumers_name[MAX_CHILD];
int cons_counter;
pid_t producers[MAX_CHILD];
char *producers_name[MAX_CHILD];
int prod_counter;

typedef struct {
    uint8_t type;
    uint16_t hash;
    uint8_t size;
    char *data;
} message;

typedef struct {
    message *head;
    int h;
    message *tail;
    int t;
    message buffer[QUEUE_MAX_SIZE];

    int counter_added;
    int counter_extracted;
} queue;

queue *message_queue;

// Алгоритм хэширования FNV-1
uint16_t fnv1_hash(const void *data, size_t size);

uint8_t init_type(uint8_t size);

uint8_t init_size();

char *init_data(uint8_t data_size);

message *create_message();

int add_message(message *msg);

int extract_message(message **msg);

void create_producer();

void remove_producer();

void create_consumer();

void remove_consumer();

void prepare();

void quit();

void test();

void print_processes();

void print_usage();

int main() {
    atexit(quit);

    prepare();

    printf("Write 'u' to see usage.\n");

    while (1) {
        char ch;
        rewind(stdin);
        ch = getchar();
        switch (ch) {
            case '1': {
                create_producer();
                break;
            }
            case '2': {
                remove_producer();
                break;
            }
            case '3': {
                create_consumer();
                break;
            }
            case '4': {
                remove_consumer();
                break;
            }
            case 'p': {
                print_processes();
                break;
            }
            case 'q': {
                quit();
                break;
            }
            case 't': {
                test();
                break;
            }
            case 'u': {
                print_usage();
                break;
            }
            default:
                break;
        }
    }
    return 0;
}

// Алгоритм хэширования FNV-1
uint16_t fnv1_hash(const void *data, size_t size) {
    const uint8_t *bytes = (const uint8_t *) data;
    uint16_t hash = 0x811C;
    for (size_t i = 0; i < size; ++i) {
        hash = (hash * 0x0101) ^ bytes[i];
    }
    return hash;
}

uint8_t init_type(uint8_t size) {
    return size >= 128 ? 1 : 0;
}

uint8_t init_size() {
    int size = 0;
    while (size <= 0) size = rand() % 257;
    if (size == 256) {
        size = 0;
    }
    return size;
}

char *init_data(uint8_t data_size) {
    int real_size = data_size;
    if (data_size == 0) real_size = 256;
    char *new_data = (char *) malloc(real_size);
    if (new_data == NULL) {
        perror("message data");
    }
    srand(time(NULL));

    for (int i = 0; i < real_size - 1; i++) {
        new_data[i] = (rand() % 26) + 'a';
    }
    new_data[real_size] = '\0';

    return new_data;
}

message *create_message() {
    message *msg;
    msg = (message *) malloc(sizeof(message));

    msg->size = init_size();
    msg->data = init_data(((msg->size + 3) / 4) * 4);
    msg->type = init_type(msg->size);
    msg->hash = 0;
    msg->hash = fnv1_hash(msg->data, strlen(msg->data));

    return msg;
}

int add_message(message *msg) {

    sem_wait(free_msgs);
    sem_wait(mutex);

    if (message_queue->counter_added - message_queue->counter_extracted == QUEUE_MAX_SIZE) {
        printf("Can't add more message: queue is full.\n");
        return -1;
    }

    if (message_queue->counter_added - message_queue->counter_extracted == 0) { // если очередь пуста
        message_queue->buffer[0] = *msg;
        message_queue->head = &message_queue->buffer[0];
        message_queue->head = &message_queue->buffer[0];
    } else {
        ++(message_queue->t);
    }

    ++(message_queue->counter_added);
    message_queue->t = message_queue->t % QUEUE_MAX_SIZE;
    message_queue->buffer[message_queue->t] = *msg;
    message_queue->tail = &message_queue->buffer[message_queue->t];

    sem_post(mutex);
    sem_post(taken_msgs);

    return message_queue->counter_added;
}

int extract_message(message **msg) {
    sem_wait(taken_msgs);
    sem_wait(mutex);

    if (message_queue->counter_added == message_queue->counter_extracted) {
        printf("Can't extract more message: queue is empty.\n");
        return -1;
    }

    *msg = message_queue->head;
    ++(message_queue->counter_extracted);
    ++(message_queue->h);
    message_queue->h = message_queue->h % QUEUE_MAX_SIZE;
    message_queue->head = &message_queue->buffer[message_queue->h];

    sem_post(mutex);
    sem_post(free_msgs);

    return message_queue->counter_extracted;
}

void create_producer() {
    if (prod_counter == MAX_CHILD) {
        fprintf(stderr, "Can't create producer: too much producers!\n");
        return;
    }
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork producer");
    } else if (pid == 0) {
        char name[16];
        sprintf(name, "producer_%02d", prod_counter);

        message *msg;
        int counter_added;
        int result;
        while (1) {
            msg = create_message();

            result = add_message(msg);

            if (result > 0) {
                counter_added = result;
                printf("%s produce msg: hash=%04X, counter_added=%d\n",
                       name, msg->hash, counter_added);
            }

            sleep(5);
        }
    }

    producers[prod_counter] = pid;
    producers_name[prod_counter] = (char *) malloc(16);
    sprintf(producers_name[prod_counter], "producer_%02d", prod_counter);
    prod_counter++;
}

void remove_producer() {
    if (prod_counter == 0) {
        fprintf(stderr, "Can't remove producer: no producers!\n");
        return;
    }

    --prod_counter;

    printf("%d \"%s\" is removed.\n",
           producers[prod_counter], producers_name[prod_counter]);

    free(producers_name[prod_counter]);
    kill(producers[prod_counter], SIGKILL);
}

void create_consumer() {
    if (cons_counter == MAX_CHILD) {
        fprintf(stderr, "Can't create consumer: too much consumers!\n");
        return;
    }
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork consumer");
    } else if (pid == 0) {
        char name[16];
        sprintf(name, "consumer_%02d", cons_counter);
        prctl(PR_SET_NAME, name, 0, 0, 0);

        message *msg;
        int counter_extracted;
        int result;
        while (1) {

            result = extract_message(&msg);

            if (result > 0) {
                counter_extracted = result;
                printf("%s consume msg: hash=%04X, counter_extracted=%d\n",
                       name, msg->hash, counter_extracted);
            }
            sleep(5);
        }
    }

    consumers[cons_counter] = pid;
    consumers_name[cons_counter] = (char *) malloc(16);
    sprintf(consumers_name[cons_counter], "consumer_%02d", cons_counter);
    cons_counter++;
}

void remove_consumer() {
    if (cons_counter == 0) {
        fprintf(stderr, "Can't remove consumer: no consumers!\n");
        return;
    }

    --cons_counter;

    printf("%d \"%s\" is removed.\n",
           consumers[cons_counter], consumers_name[cons_counter]);

    free(consumers_name[cons_counter]);
    kill(consumers[cons_counter], SIGKILL);
}

void prepare() {
    int fd = shm_open("/queue", (O_RDWR | O_CREAT | O_TRUNC), (S_IRUSR | S_IWUSR));
    if (fd < 0) {
        perror("shm_open");
        exit(1);
    }

    if (ftruncate(fd, sizeof(queue))) {
        perror("message_queue");
        exit(1);
    }

    void *ptr = mmap(NULL, sizeof(queue), PROT_READ | PROT_WRITE,
                     MAP_SHARED, fd, 0);
    if (ptr == MAP_FAILED) {
        perror("map");
        exit(1);
    }

    message_queue = (queue *) ptr;

    if (close(fd)) {
        perror("close");
        exit(1);
    }

    memset(message_queue, 0, sizeof(queue));

    mutex = sem_open("/mutex", (O_RDWR | O_CREAT | O_TRUNC), (S_IRUSR | S_IWUSR), 1);
    free_msgs = sem_open("/free_msgs_sem", O_CREAT, 0666, QUEUE_MAX_SIZE);
    taken_msgs = sem_open("/taken_msgs_sem", O_CREAT, 0666, 0);

    if (free_msgs == SEM_FAILED || taken_msgs == SEM_FAILED || mutex == SEM_FAILED) {
        perror("free_msgs/taken_msgs/mutex semaphore");
        exit(1);
    }
}

void print_processes() {
    if (prod_counter == 0 && cons_counter == 0) {
        printf("No processes.\n");
        return;
    }
    for (int i = 0; i < prod_counter; i++) {
        printf("%d \"%s\"\n", producers[i], producers_name[i]);
    }
    for (int i = 0; i < cons_counter; i++) {
        printf("%d \"%s\"\n", consumers[i], consumers_name[i]);
    }
}

void quit() {

    while (cons_counter) remove_consumer();
    while (prod_counter) remove_producer();

    munmap(message_queue, sizeof(queue));

    shm_unlink("/queue");

    sem_close(mutex);

    sem_close(free_msgs);

    sem_close(taken_msgs);

    sem_unlink("/mutex");

    sem_unlink("/free_msgs_sem");

    sem_unlink("/taken_msgs_sem");

    exit(0);
}

void test() {
    printf("PR-S: %d\n", prod_counter);
    printf("CN-S: %d\n", cons_counter);
    printf("Q_MS: %d Q_CS: %d\n", QUEUE_MAX_SIZE, message_queue->counter_added - message_queue->counter_extracted);
    printf("AD: %d EX: %d\n", message_queue->counter_added, message_queue->counter_extracted);
}

void print_usage() {
    char *menu = "Usage: write \n"
                 "'1' - to create producer.\n"
                 "'2' - to remove producer.\n"
                 "'3' - to create consumer.\n"
                 "'4' - to remove consumer.\n"
                 "'p' - to print processes.\n"
                 "'t' - to print test.\n"
                 "'q' - to quit.\n";
    printf("%s", menu);
}
