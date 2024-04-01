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


#define THREAD_NUM 16
#define MAX_ITEMS 100000 // Total items to produce and consume

sem_t semFull;
sem_t semEmpty;
pthread_mutex_t mutexBuffer; // To prevent race conditions when accessing the shared buffer.

int buffer[10];
int bufferSize = (sizeof(buffer) / sizeof(int));
int in = 0;
int out = 0;
int count = 0;

int producedItems[100] = {0};
int consumedItems[100] = {0};

void* producer(void* args) {

    for(int i = 0; i < MAX_ITEMS / (THREAD_NUM / 2); ++i) { // In order to equally divide the work among the threads
        int x = rand() % 100;

        sem_wait(&semEmpty); // Producers will wait for at least 1 open spot
        pthread_mutex_lock(&mutexBuffer);
        buffer[in] = x;
        in = (in + 1) % bufferSize;
        producedItems[x]++;
        pthread_mutex_unlock(&mutexBuffer);
        sem_post(&semFull);
    }
}

void* consumer(void* args) {
    int thread_id = *(int*)args;
    for(int i = 0; i < MAX_ITEMS / (THREAD_NUM / 2); ++i) { // In order to equally divide the work among the threads
        sem_wait(&semFull); // Consumers will wait for at least 1 open spot
        pthread_mutex_lock(&mutexBuffer);
        int y = buffer[out];
        out = (out+1) % bufferSize;
        consumedItems[y]++;
        pthread_mutex_unlock(&mutexBuffer);
        sem_post(&semEmpty);
        //sleep(1);
    }
    free(args);
}

int main(int argc, char* argv[]) {
    srand(time(NULL));
    pthread_t th[THREAD_NUM];
    pthread_mutex_init(&mutexBuffer, NULL);
    sem_init(&semFull, 0, 0);
    sem_init(&semEmpty, 0, bufferSize);


    struct timespec start, end;
    double elapsed_time;
    
    clock_gettime(CLOCK_MONOTONIC, &start); // Start timing

    int i;

    for(i = 0; i < THREAD_NUM; i++) {
        int *arg = malloc(sizeof(*arg)); // Dynamically allocate memory to store the thread ID.
        if (arg == NULL) {
            perror("Failed to allocate memory for thread arg.");
            exit(1);
        }
        *arg = i; // Store the loop index in the allocated memory.
        if(i % 2 == 0) {
            if(pthread_create(&th[i], NULL, &producer, NULL) != 0) {
                perror("Producer thread creation failed");
            }
        }
        else {
            if(pthread_create(&th[i], NULL, &consumer, arg)) {
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