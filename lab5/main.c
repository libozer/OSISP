#include "header.h"

void print_menu();                          // фукнция вывода меню
void input_option(char *option);            // функция ввода опции
void start();                               // функция для инициализации семафоров, мьютекса, очереди с сообщениями и общей памяти
void end();                                 // функция для завершения работы с семафорами, мьютексом, общей памятью
void list_threads();                        // функция для вывода дочерних процессов
void println();                             // функция для вывода линии, состоящей из '-'

void create_producer();                     // функция для создания производителя
void delete_producer();                     // фукнция для удаления производителя
void create_consumer();                     // функция для создания потребителя
void delete_consumer();                     // функция для удаления потребителя
void *consumer_func(void *arg);             // функция, вызываемая для созданного потребителя
void *producer_func(void *arg);             // функция, вызываемая для созданного производителя

uint16_t sdbm_hash(message *msg);           // хеш -функция для сверки контрольной суммы
void generate_message();                    // функция для создания сообщения
int queue_put_message();                    // функция для помещения сообщения в очередь
void print_producer_message(message *msg);  // функция для вывода сообщения
int queue_extract_message();                // функция для извлечения сообщения из очереди
void check_message(message *msg);           // функция для потребления сообщения
void print_consumer_message(message *msg);  // функция для вывода сообщения потребителя

queue q;                       

pthread_mutex_t mutex;                      // мьютекс для монопольного доступа к очереди и два семафора для заполнения и извлечения сообщений
sem_t *items;                               // семафоры для количества сообщений и свободных мест в очереди 
sem_t *free_space;

pthread_t producers[MAX_THREADS];           // id потоков производителей
pthread_t consumers[MAX_THREADS];           // id потоков потребителей

int producers_count;                        // количество производителей
int consumers_count;                        // количество потребителей

pthread_t tid;                              // id потока родительского процесса
int ring_size = 5;

int main()
{
    start();
    println();
    print_menu();
    char option;
    while(true)
    {
        input_option(&option);
        switch(option)
        {
            case '+':
            {
                ring_size++;
                sem_post(free_space);
                printf("size of ring buffer is %d now\n", ring_size);
                println();
                break;
            }
            case '-':
            {
                if(sem_trywait(free_space))
                {
                    if(!ring_size) fprintf(stderr, "size of ring buffer can't be less than 0\n");
                    else fprintf(stderr, "queue is full, consume messages from queue before decreasing its size\n");
                    errno = 0;
                }
                else printf("size of ring buffer is %d now\n", --ring_size);
                println();
                break;
            }
            case 'm':
            {
                print_menu();
                break;
            }
            case 'l':
            {
                list_threads();
                break;
            }
            case 's':
            {
                printf("ring buffer size = %d\n", ring_size);
                println();
                break;
            }
            case '1':
            {
                create_producer();
                break;
            }
            case '2':
            {
                delete_producer();
                break;
            }
            case '3':
            {
                create_consumer();
                break;
            }
            case '4':
            {
                delete_consumer();
                break;
            }
            case '0':
            {
                end();
            }
        }
    }
}

void println()
{
    printf("------------------------------------------------\n");
}

void list_threads()
{
    printf("main thread:\ntid - %ld\n", tid);
    printf("producers:\n");
    for(int i = 0; i < producers_count; i++)
    {
        printf("tid - %ld\n", producers[i]);
    }
    printf("consumers:\n");
    for(int i = 0; i < consumers_count; i++)
    {
        printf("tid - %ld\n", consumers[i]);
    }
    println();
}

void create_producer()
{
    if(producers_count == MAX_THREADS - 1)
    {
        fprintf(stderr, "max count of producers = %d was exceeded\n", MAX_THREADS);
        fprintf(stderr, "------------------------------------------------\n");
        return;
    }
    if(pthread_create(&producers[producers_count], NULL, producer_func, NULL))
    {
        perror("pthread_create");
        exit(errno);
    }
    printf("producer with id = %ld was created\n", producers[producers_count]);
    println();
    producers_count++;
}

void delete_producer()
{
    if(producers_count == 0)
    {
        fprintf(stderr, "there are no producers left\n");
        fprintf(stderr, "------------------------------------------------\n");
        return;
    }
    producers_count--;
    pthread_cancel(producers[producers_count]);
    if(pthread_join(producers[producers_count], NULL))
    {
        perror("pthread_join");
        exit(errno);
    }
    printf("producer with id = %ld was deleted\n", producers[producers_count]);
    println();
}

void create_consumer()
{
    if(consumers_count == MAX_THREADS - 1)
    {
        fprintf(stderr, "max count of consumers = %d was exceeded\n", MAX_THREADS);
        fprintf(stderr, "------------------------------------------------\n");
        return;
    }
    if(pthread_create(&consumers[consumers_count], NULL, consumer_func, NULL))
    {
        perror("pthread_create");
        exit(errno);
    }
    printf("consumer with id = %ld was created\n", consumers[consumers_count]);
    println();
    consumers_count++;
}

void delete_consumer()
{
    if(consumers_count == 0)
    {
        fprintf(stderr, "there are no consumers left\n");
        fprintf(stderr, "------------------------------------------------\n");
        return;
    }
    consumers_count--;
    pthread_cancel(consumers[consumers_count]);
    if(pthread_join(consumers[consumers_count], NULL))
    {
        perror("pthread_join");
        exit(errno);
    }
    printf("consumer with id = %ld was deleted\n", consumers[consumers_count]);
    println();
}

void *producer_func(void *arg)
{
    message msg;
    int count;
    while (true) 
    {
        sleep(3);
        generate_message(&msg);
        sem_getvalue(free_space, &count);
        if(!count)
        {
            fprintf(stderr, "queue is full, producer with tid = %ld is waiting\n", pthread_self());
        }
        sem_wait(free_space);       // уменьшаем значение (блокируем) кол-ва свободных мест в очереди
        pthread_mutex_lock(&mutex);  // блокируем доступ к очереди

        queue_put_message(&msg);
        print_producer_message(&msg);

        pthread_mutex_unlock(&mutex);// разблокируем доступ к очереди
        sem_post(items);            // увеличиваем значение (разблокируем) кол-ва сообщений в очереди 
    }
}

void *consumer_func(void *arg) 
{
    message msg;
    int count;
    while(true)
    {
        sleep(3);
        sem_getvalue(items, &count);
        if(!count)
        {
            fprintf(stderr, "queue is empty, consumer with tid = %ld is waiting\n", pthread_self());
        }
        sem_wait(items);             // уменьшаем значение (блокируем) кол-во сообщений в очереди
        pthread_mutex_lock(&mutex);  // блокируем доступ к очереди

        queue_extract_message(&msg);
        check_message(&msg);
        print_consumer_message(&msg);

        pthread_mutex_unlock(&mutex);// разблокируем доступ к очереди
        sem_post(free_space);        // увеличиваем значение (разблокируем) кол-во свободных мест в очереди
    }
}

void start()
{
    tid = pthread_self();   
    memset(&q, 0, sizeof(queue));
    if(pthread_mutex_init(&mutex, NULL))
    {
        perror("pthread_mutex_init");
        exit(errno);
    }
    if ((free_space = sem_open("free_space", (O_RDWR | O_CREAT | O_TRUNC), (S_IRUSR | S_IWUSR), ring_size)) == SEM_FAILED || // для кол-ва свободных мест
        (items = sem_open("items", (O_RDWR | O_CREAT | O_TRUNC ), (S_IRUSR | S_IWUSR), 0)) == SEM_FAILED)  // не было произведено сообщений
    {
        perror("sem_open");
        exit(errno);
    }   
}

void end()
{
    if(pthread_mutex_destroy(&mutex))
    {
        perror("pthread_mutex_destroy");
        exit(errno);
    }
    if(sem_close(free_space) || sem_close(items))                    // закрываем и удаляем семафоры
    {
        perror("sem_close");
        exit(errno);
    }
    if(sem_unlink("free_space") || sem_unlink("items"))
    {
        perror("sem_unlink");
        exit(errno);
    }
    exit(0);
}

void print_producer_message(message *msg)
{
    printf("thread with id = %ld produced msg:\ntype = %d\nhash = %04X\nsize = %d\n",
    pthread_self(), msg->type, msg->hash, msg->size);
    printf("added messages count: %d\n", q.added_count);
    println();
}

void print_consumer_message(message *msg)
{
    printf("thread with id = %ld consumed msg:\ntype = %d\nhash = %04X\nsize = %d\n",
    pthread_self(), msg->type, msg->hash, msg->size);
    printf("extracted messages count: %d\n", q.extracted_count);
    println();
}

int queue_put_message(message *msg)
{
    if (q.tail == ring_size) q.tail = 0;
    q.buffer[q.tail] = *msg;
    q.tail++;
    return ++q.added_count;
}

int queue_extract_message(message *msg)
{
    if (q.head == ring_size) q.head = 0;
    *msg = q.buffer[q.head];
    q.head++;
    return ++q.extracted_count;
}

uint16_t sdbm_hash(message *msg)           // хеш-функция для проверки контрольной суммы
{
	uint16_t hash = 0;
    int len = sizeof(*msg);                // длина всей структуры message в байтах
    char *ptr = (char*) msg;               // указатель на байт стуктуры message
	for (int i = 0; i < len; ptr++, i++)
	{
		hash = (*ptr) + (hash << 6) + (hash << 16) - hash;
	}
	return hash;
}

void generate_message(message *msg)
{
    int tmp_size = 0;
    msg->type = 0;
    while(!tmp_size)
    {
        tmp_size = rand() % 257;
        if(tmp_size == 256) 
        {
            msg->type = 1;
            msg->size = 0;
            break;
        }
        msg->size = tmp_size;
    }
    uint8_t tmp_data;
    for(int i = 0; i < tmp_size; i++)
    {
        tmp_data = ((rand() % (127 - 32)) + 32);
        msg->data[i] = tmp_data;           // заполняем msg->data случайным символов аски с кодом от 32 ' ' до 126 '~' включительно 
    }
    msg->hash = 0;
    msg->hash = sdbm_hash(msg);
}

void check_message(message *msg)           // при потреблении сообщения надо сравнить исходный хеш с хешем data
{
    uint16_t msg_hash = msg->hash;         // сохраняем исходное значение, высчитываем значение для пришедшего сообщения, обнулив при этом его хеш
    msg->hash = 0;
    uint16_t check_sum = sdbm_hash(msg);
    if(check_sum != msg_hash)
    {
        fprintf(stderr, "check_sum = %04X != msg_hash = %04X\n", check_sum, msg_hash);
    }
    msg->hash = msg_hash;  
}

void print_menu()
{
    printf("'0' - exit\n");
    printf("'1' - create producer\n'2'- remove producer\n");
    printf("'3' - create consumer\n'4' - remove consumer\n");
    printf("'m' - print menu\n'l' - list threads\n's' - print ring buffer size\n");
    printf("'+' - increment queue size by 1\n'-' - decrement queue size by 1\n");
    println();
}

void input_option(char *option)
{
    char tmp; 
    while(true)                                            
    {
        scanf("%c", &tmp);
        if((tmp >= '0' && tmp <= '4') || tmp == 'm' ||
            tmp == 'l' || tmp == '+' || tmp == '-' || tmp == 's') break;
    }
    *option = tmp;
    println();
}