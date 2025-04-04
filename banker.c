#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>
#include <unistd.h>
#include <time.h>

#define NUM_CUSTOMERS 5
#define NUM_RESOURCES 3

int available[NUM_RESOURCES];
int maximum[NUM_CUSTOMERS][NUM_RESOURCES];
int allocation[NUM_CUSTOMERS][NUM_RESOURCES];
int need[NUM_CUSTOMERS][NUM_RESOURCES];

pthread_mutex_t lock;

void init_banker(int resources[]) {
    for (int i = 0; i < NUM_RESOURCES; i++) {
        available[i] = resources[i];
    }

    srand(time(NULL));
    for (int i = 0; i < NUM_CUSTOMERS; i++) {
        for (int j = 0; j < NUM_RESOURCES; j++) {
            maximum[i][j] = rand() % (available[j] + 1);
            allocation[i][j] = 0;
            need[i][j] = maximum[i][j];
        }
    }
}

bool check_safety() {
    int work[NUM_RESOURCES];
    bool finish[NUM_CUSTOMERS] = {false};

    for (int i = 0; i < NUM_RESOURCES; i++) {
        work[i] = available[i];
    }

    int count = 0;
    while (count < NUM_CUSTOMERS) {
        bool found = false;
        for (int i = 0; i < NUM_CUSTOMERS; i++) {
            if (!finish[i]) {
                bool possible = true;
                for (int j = 0; j < NUM_RESOURCES; j++) {
                    if (need[i][j] > work[j]) {
                        possible = false;
                        break;
                    }
                }

                if (possible) {
                    for (int j = 0; j < NUM_RESOURCES; j++) {
                        work[j] += allocation[i][j];
                    }
                    finish[i] = true;
                    found = true;
                    count++;
                }
            }
        }
        if (!found) break;
    }
    return (count == NUM_CUSTOMERS);
}

int request_resources(int customer, int request[]) {
    pthread_mutex_lock(&lock);
    
    for (int i = 0; i < NUM_RESOURCES; i++) {
        if (request[i] > need[customer][i]) {
            pthread_mutex_unlock(&lock);
            return -1;
        }
        if (request[i] > available[i]) {
            pthread_mutex_unlock(&lock);
            return -1;
        }
    }

    for (int i = 0; i < NUM_RESOURCES; i++) {
        available[i] -= request[i];
        allocation[customer][i] += request[i];
        need[customer][i] -= request[i];
    }

    if (check_safety()) {
        pthread_mutex_unlock(&lock);
        return 0;
    } else {
        for (int i = 0; i < NUM_RESOURCES; i++) {
            available[i] += request[i];
            allocation[customer][i] -= request[i];
            need[customer][i] += request[i];
        }
        pthread_mutex_unlock(&lock);
        return -1;
    }
}

int release_resources(int customer, int release[]) {
    pthread_mutex_lock(&lock);
    
    for (int i = 0; i < NUM_RESOURCES; i++) {
        if (release[i] > allocation[customer][i]) {
            pthread_mutex_unlock(&lock);
            return -1;
        }
    }

    for (int i = 0; i < NUM_RESOURCES; i++) {
        available[i] += release[i];
        allocation[customer][i] -= release[i];
        need[customer][i] += release[i];
    }

    pthread_mutex_unlock(&lock);
    return 0;
}

void* customer(void* arg) {
    int id = *(int*)arg;
    
    while (1) {
        sleep(rand() % 3 + 1);
        
        if (rand() % 4 != 0) {
            int request[NUM_RESOURCES];
            for (int i = 0; i < NUM_RESOURCES; i++) {
                request[i] = need[id][i] > 0 ? rand() % (need[id][i] + 1) : 0;
            }
            
            if (request_resources(id, request) == 0) {
                printf("Customer %d: Request granted\n", id);
            } else {
                printf("Customer %d: Request denied\n", id);
            }
        } else {
            int release[NUM_RESOURCES];
            for (int i = 0; i < NUM_RESOURCES; i++) {
                release[i] = allocation[id][i] > 0 ? rand() % (allocation[id][i] + 1) : 0;
            }
            
            if (release_resources(id, release) == 0) {
                printf("Customer %d: Resources released\n", id);
            } else {
                printf("Customer %d: Release failed\n", id);
            }
        }
    }
    return NULL;
}

void print_state() {
    printf("\nSystem State:\nAvailable: ");
    for (int i = 0; i < NUM_RESOURCES; i++) {
        printf("%d ", available[i]);
    }
    
    printf("\nMaximum:\n");
    for (int i = 0; i < NUM_CUSTOMERS; i++) {
        printf("C%d: ", i);
        for (int j = 0; j < NUM_RESOURCES; j++) {
            printf("%d ", maximum[i][j]);
        }
        printf("\n");
    }
    
    printf("Allocation:\n");
    for (int i = 0; i < NUM_CUSTOMERS; i++) {
        printf("C%d: ", i);
        for (int j = 0; j < NUM_RESOURCES; j++) {
            printf("%d ", allocation[i][j]);
        }
        printf("\n");
    }
    
    printf("Need:\n");
    for (int i = 0; i < NUM_CUSTOMERS; i++) {
        printf("C%d: ", i);
        for (int j = 0; j < NUM_RESOURCES; j++) {
            printf("%d ", need[i][j]);
        }
        printf("\n");
    }
    printf("\n");
}

int main(int argc, char *argv[]) {
    if (argc != NUM_RESOURCES + 1) {
        fprintf(stderr, "Usage: %s <res1> <res2> <res3>\n", argv[0]);
        return 1;
    }

    int resources[NUM_RESOURCES];
    for (int i = 0; i < NUM_RESOURCES; i++) {
        resources[i] = atoi(argv[i + 1]);
    }

    init_banker(resources);
    pthread_mutex_init(&lock, NULL);

    printf("Initial state:");
    print_state();

    pthread_t threads[NUM_CUSTOMERS];
    int ids[NUM_CUSTOMERS];

    for (int i = 0; i < NUM_CUSTOMERS; i++) {
        ids[i] = i;
        if (pthread_create(&threads[i], NULL, customer, &ids[i]) != 0) {
            perror("Failed to create thread");
            return 1;
        }
    }

    while (1) {
        sleep(5);
        print_state();
    }

    pthread_mutex_destroy(&lock);
    return 0;
}