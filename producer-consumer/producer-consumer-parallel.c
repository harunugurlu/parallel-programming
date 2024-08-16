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
#define MAX_ITEMS 100 // Total items to produce and consume

sem_t semFull[4];
sem_t semEmpty[4];
pthread_mutex_t mutexBuffer[4]; // To prevent race conditions when accessing the shared buffer.

int buffer[8];
int bufferSize = (sizeof(buffer) / sizeof(int));


// Different in and out indexes for different producer consumer pairs
typedef struct {
    int in;
    int out;
    int thread_id;
} thread_args;

int producedItems[100] = {0};
int consumedItems[100] = {0};

void* producer(void* arg) {
    thread_args *args = (thread_args*)arg;
    for(int i = 0; i < MAX_ITEMS / (THREAD_NUM / 2); ++i) { // In order to equally divide the work among the threads
        int x = rand() % 100;
        int index = args->in;
        // printf("Thread_id: %d, index in producer: %d\n", args->thread_id, index);
        int sem_val = 0;
        // sem_getvalue(&semEmpty[index], &sem_val);
        // printf("Thread_id: %d, sem_value before wait is %d\n", args->thread_id, sem_val);
        sem_wait(&semEmpty[index]); // Producers will wait for at least 1 open spot
        // sem_getvalue(&semEmpty[index], &sem_val);
        // printf("Thread_id: %d, sem_value after wait is %d\n", args->thread_id, sem_val);
        pthread_mutex_lock(&mutexBuffer[index]);
        buffer[args->in] = x;
        printf("Producer Thread_id: %d, sent: %d\n",args->thread_id, x);
        args->in = (args->in + 1) % (THREAD_NUM / 4);
        producedItems[x]++;
        pthread_mutex_unlock(&mutexBuffer[index]);
        sem_post(&semFull[index]);
    }
    printf("Producer %d got out!!!\n", args->thread_id);
    free(args);
}

void* consumer(void* arg) {
    thread_args *args = (thread_args*)arg;
    
    for(int i = 0; i < MAX_ITEMS / (THREAD_NUM / 2); ++i) { // In order to equally divide the work among the threads
        int index = args->out;
        sem_wait(&semFull[index]); // Consumers will wait for at least 1 open spot
        pthread_mutex_lock(&mutexBuffer[index]);
        int y = buffer[args->out];
        printf("Consumer Thread id: %d, thread got: %d\n", args->thread_id, y);
        args->out = (args->out+1) % (THREAD_NUM / 4);
        consumedItems[y]++;
        pthread_mutex_unlock(&mutexBuffer[index]);
        sem_post(&semEmpty[index]);
    }
    printf("Consumer %d got out!!!\n", args->thread_id);
    free(args);
}

int main(int argc, char* argv[]) {
    srand(time(NULL));
    pthread_t th[THREAD_NUM];
    thread_args args;

    struct timespec start, end;
    double elapsed_time;
    
    clock_gettime(CLOCK_MONOTONIC, &start); // Start timing

    int i;
    int j;
    int n;
    int cue = 0;

    for(i = 0, j = 0, n = 0; i < THREAD_NUM; i+=2, j+=2, n++) {
        thread_args *args1 = (thread_args*)malloc(sizeof(thread_args));
        thread_args *args2 = (thread_args*)malloc(sizeof(thread_args));
        if (args1 == NULL || args2 == NULL) {
            perror("Failed to allocate memory for thread arg.");
            exit(1);
        }

        if(n < 4) {
            if(pthread_mutex_init(&mutexBuffer[n], NULL) != 0) {
                perror("Failed to allocate memory for thread arg.");
                exit(1);
            }
            if(sem_init(&semFull[n], 0, 0) != 0) {
                perror("Failed to allocate memory for thread arg.");
                exit(1);
            }
            if(sem_init(&semEmpty[n], 0, 2) != 0) {
                perror("Failed to allocate memory for thread arg.");
                exit(1);
            }
        }

        args1->in = j;
        args1->out = j;
        args1->thread_id = i;

        args2->in = j;
        args2->out = j;
        args2->thread_id = i+1;

        if(i % 2 == 0) {
            if(pthread_create(&th[i], NULL, &producer, (void*)args1) != 0) {
                perror("Producer thread creation failed");
            }
            if(pthread_create(&th[i+1], NULL, &consumer, (void*)args2) != 0) {
                perror("Consumer thread creation failed");
            }
        }
    }
    // for(i = 0; i < THREAD_NUM; i++) {
    //     printf("buraya girdi\n");
    //     if(pthread_join(th[i], NULL) != 0) {
    //         perror("Thread join failed");
    //     }
    // }
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

    for(int i = 0; i < 4; i++) {
        sem_destroy(&semEmpty[n]);
        sem_destroy(&semFull[n]);
        pthread_mutex_destroy(&mutexBuffer[i]);
    }
    
    return 0;
}