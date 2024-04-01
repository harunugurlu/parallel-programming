// Producer only produces when the buffer is not full
// Consumer only consumes when the buffer is not empty
// Both can't access the buffer at the same time

// in --> next empty buffer
// out --> first full buffer

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <semaphore.h>


#define THREAD_NUM 8
#define MAX_ITEMS 80 // Total items to produce and consume

sem_t semFull;
sem_t semEmpty;
pthread_mutex_t mutexBuffer[4]; // To prevent race conditions when accessing the shared buffer.

int buffer[8];
int bufferSize = (sizeof(buffer) / sizeof(int));

// Different in and out indexes for different producer consumer pairs
int in1 = 0;
int out1 = 0;
int in2 = 0;
int out2 = 0;
int in3 = 0;
int out3 = 0;
int in4 = 0;
int out4 = 0;

typedef struct {
    int in;
    int out;
} thread_args;

int producedItems[100] = {0};
int consumedItems[100] = {0};

void* producer(void* arg) {
    thread_args *args = (thread_args*)arg;

    for(int i = 0; i < MAX_ITEMS / (THREAD_NUM / 2); ++i) { // In order to equally divide the work among the threads
        int x = rand() % 100;

        sem_wait(&semEmpty); // Producers will wait for at least 1 open spot
        pthread_mutex_lock(&mutexBuffer);
        buffer[args->in] = x;
        printf("Producer sent: %d\n", x);
        args->in = (args->in + 1) % (THREAD_NUM / 4);
        producedItems[x]++;
        pthread_mutex_unlock(&mutexBuffer);
        sem_post(&semFull);
    }
}

void* consumer(void* arg) {
    thread_args *args = (thread_args*)arg;
    
    for(int i = 0; i < MAX_ITEMS / (THREAD_NUM / 2); ++i) { // In order to equally divide the work among the threads
        sem_wait(&semFull); // Consumers will wait for at least 1 open spot
        pthread_mutex_lock(&mutexBuffer);
        int y = buffer[args->out];
        printf("thread got: %d\n", y);
        args->out = (args->out+1) % (THREAD_NUM / 4);
        consumedItems[y]++;
        pthread_mutex_unlock(&mutexBuffer);
        sem_post(&semEmpty);
    }
    free(args);
}

int main(int argc, char* argv[]) {
    srand(time(NULL));
    pthread_t th[THREAD_NUM];
    pthread_mutex_init(&mutexBuffer, NULL);
    sem_init(&semFull, 0, 0);
    sem_init(&semEmpty, 0, 2);


    thread_args args;

    struct timespec start, end;
    double elapsed_time;
    
    clock_gettime(CLOCK_MONOTONIC, &start); // Start timing

    int i;
    int j;
    int n;
    int cue = 0;
    for(i = 0, j = 0, n = 0; i < THREAD_NUM; i+=2, j+=2, n++) {
        thread_args *args = (thread_args*)malloc(sizeof(thread_args));
        if (args == NULL) {
            perror("Failed to allocate memory for thread arg.");
            exit(1);
        }

        pthread_mutex_init(&mutexBuffer[i], NULL);

        args->in = j;
        args->out = j;

        if(i % 2 == 0) {
            if(pthread_create(&th[i], NULL, &producer, (void*)args) != 0) {
                perror("Producer thread creation failed");
            }
            if(pthread_create(&th[i+1], NULL, &consumer, (void*)args) != 0) {
                perror("Consumer thread creation failed");
            }
        }
    }
    for(i = 0; i < THREAD_NUM; i++) {
        if(pthread_join(th[i], NULL) != 0) {
            perror("Thread join failed");
        }
    }
    for(int i = 0; i < 100; i++) {
        if(producedItems[i] != consumedItems[i]) {
            printf("Mismatch found for item %d: Produced %d, Consumed %d\n", i, producedItems[i], consumedItems[i]);
            // Handle the error appropriately
    }
}

    clock_gettime(CLOCK_MONOTONIC, &end); // End timing

    elapsed_time = end.tv_sec - start.tv_sec;
    elapsed_time += (end.tv_nsec - start.tv_nsec) / 1000000000.0;
    
    printf("Total time taken: %f seconds\n", elapsed_time);

    sem_destroy(&semEmpty);
    sem_destroy(&semFull);
    pthread_mutex_destroy(&mutexBuffer);
    
    return 0;
}