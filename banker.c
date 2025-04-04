#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>
#include <unistd.h>
#include <time.h>

// Global variables
#define NUMBER_OF_CUSTOMERS 5
#define NUMBER_OF_RESOURCES 3

int available[NUMBER_OF_RESOURCES];
int maximum[NUMBER_OF_CUSTOMERS][NUMBER_OF_RESOURCES];
int allocation[NUMBER_OF_CUSTOMERS][NUMBER_OF_RESOURCES];
int need[NUMBER_OF_CUSTOMERS][NUMBER_OF_RESOURCES];

// Mutex for thread safety
pthread_mutex_t mutex;

// Function prototypes
void initialize_data(int argc, char *argv[]);
bool is_safe_state();
int request_resources(int customer_num, int request[]);
int release_resources(int customer_num, int release[]);
void* customer_behavior(void* customer_num_ptr);
void print_state();

int main(int argc, char *argv[]) {
    // Check command line arguments
    if (argc != NUMBER_OF_RESOURCES + 1) {
        fprintf(stderr, "Usage: %s <resource 1> <resource 2> <resource 3>\n", argv[0]);
        return -1;
    }

    // Initialize data structures
    initialize_data(argc, argv);
    
    // Initialize mutex
    pthread_mutex_init(&mutex, NULL);
    
    // Create customer threads
    pthread_t customers[NUMBER_OF_CUSTOMERS];
    int customer_nums[NUMBER_OF_CUSTOMERS];
    
    for (int i = 0; i < NUMBER_OF_CUSTOMERS; i++) {
        customer_nums[i] = i;
        pthread_create(&customers[i], NULL, customer_behavior, &customer_nums[i]);
    }
    
    // Wait for all customer threads to finish (though they run indefinitely)
    for (int i = 0; i < NUMBER_OF_CUSTOMERS; i++) {
        pthread_join(customers[i], NULL);
    }
    
    // Clean up mutex
    pthread_mutex_destroy(&mutex);
    
    return 0;
}

void initialize_data(int argc, char *argv[]) {
    // Initialize available resources from command line
    for (int i = 0; i < NUMBER_OF_RESOURCES; i++) {
        available[i] = atoi(argv[i + 1]);
    }
    
    // Initialize maximum demand randomly (for demo purposes)
    srand(time(NULL));
    for (int i = 0; i < NUMBER_OF_CUSTOMERS; i++) {
        for (int j = 0; j < NUMBER_OF_RESOURCES; j++) {
            maximum[i][j] = rand() % (available[j] + 1); // Max demand <= available
            allocation[i][j] = 0;
            need[i][j] = maximum[i][j];
        }
    }
    
    printf("Initial state:\n");
    print_state();
}

bool is_safe_state() {
    int work[NUMBER_OF_RESOURCES];
    bool finish[NUMBER_OF_CUSTOMERS] = {false};
    
    // Initialize work = available
    for (int i = 0; i < NUMBER_OF_RESOURCES; i++) {
        work[i] = available[i];
    }
    
    // Find a customer that can finish
    bool found;
    for (int i = 0; i < NUMBER_OF_CUSTOMERS; i++) {
        found = false;
        for (int j = 0; j < NUMBER_OF_CUSTOMERS; j++) {
            if (!finish[j]) {
                bool can_finish = true;
                for (int k = 0; k < NUMBER_OF_RESOURCES; k++) {
                    if (need[j][k] > work[k]) {
                        can_finish = false;
                        break;
                    }
                }
                
                if (can_finish) {
                    // Customer can finish - release its resources
                    for (int k = 0; k < NUMBER_OF_RESOURCES; k++) {
                        work[k] += allocation[j][k];
                    }
                    finish[j] = true;
                    found = true;
                }
            }
        }
        
        if (!found) {
            break; // No customer found that can finish
        }
    }
    
    // Check if all customers finished
    for (int i = 0; i < NUMBER_OF_CUSTOMERS; i++) {
        if (!finish[i]) {
            return false; // Unsafe state
        }
    }
    
    return true; // Safe state
}

int request_resources(int customer_num, int request[]) {
    pthread_mutex_lock(&mutex);
    int return_code = -1; // Default to failure
    
    // Check if request exceeds need
    for (int i = 0; i < NUMBER_OF_RESOURCES; i++) {
        if (request[i] > need[customer_num][i]) {
            printf("Customer %d: Request exceeds need. Denied.\n", customer_num);
            pthread_mutex_unlock(&mutex);
            return -1;
        }
    }
    
    // Check if request exceeds available
    for (int i = 0; i < NUMBER_OF_RESOURCES; i++) {
        if (request[i] > available[i]) {
            printf("Customer %d: Not enough resources available. Denied.\n", customer_num);
            pthread_mutex_unlock(&mutex);
            return -1;
        }
    }
    
    // Try to allocate resources temporarily
    for (int i = 0; i < NUMBER_OF_RESOURCES; i++) {
        available[i] -= request[i];
        allocation[customer_num][i] += request[i];
        need[customer_num][i] -= request[i];
    }
    
    // Check if the new state is safe
    if (is_safe_state()) {
        printf("Customer %d: Request granted.\n", customer_num);
        return_code = 0; // Success
    } else {
        // Revert the allocation
        for (int i = 0; i < NUMBER_OF_RESOURCES; i++) {
            available[i] += request[i];
            allocation[customer_num][i] -= request[i];
            need[customer_num][i] += request[i];
        }
        printf("Customer %d: Request leads to unsafe state. Denied.\n", customer_num);
    }
    
    print_state();
    pthread_mutex_unlock(&mutex);
    return return_code;
}

int release_resources(int customer_num, int release[]) {
    pthread_mutex_lock(&mutex);
    
    // Check if release exceeds allocation
    for (int i = 0; i < NUMBER_OF_RESOURCES; i++) {
        if (release[i] > allocation[customer_num][i]) {
            printf("Customer %d: Trying to release more than allocated. Denied.\n", customer_num);
            pthread_mutex_unlock(&mutex);
            return -1;
        }
    }
    
    // Release the resources
    for (int i = 0; i < NUMBER_OF_RESOURCES; i++) {
        available[i] += release[i];
        allocation[customer_num][i] -= release[i];
        need[customer_num][i] += release[i];
    }
    
    printf("Customer %d: Resources released.\n", customer_num);
    print_state();
    pthread_mutex_unlock(&mutex);
    return 0;
}

void* customer_behavior(void* customer_num_ptr) {
    int customer_num = *((int*)customer_num_ptr);
    
    while (1) {
        // Sleep for random time between requests
        sleep(rand() % 3 + 1);
        
        // Decide randomly whether to request or release (more requests than releases)
        if (rand() % 4 != 0) { // 75% chance to request
            // Create a random request that doesn't exceed need
            int request[NUMBER_OF_RESOURCES];
            for (int i = 0; i < NUMBER_OF_RESOURCES; i++) {
                if (need[customer_num][i] > 0) {
                    request[i] = rand() % (need[customer_num][i] + 1);
                } else {
                    request[i] = 0;
                }
            }
            
            // Make the request
            request_resources(customer_num, request);
        } else {
            // Create a random release that doesn't exceed allocation
            int release[NUMBER_OF_RESOURCES];
            for (int i = 0; i < NUMBER_OF_RESOURCES; i++) {
                if (allocation[customer_num][i] > 0) {
                    release[i] = rand() % (allocation[customer_num][i] + 1);
                } else {
                    release[i] = 0;
                }
            }
            
            // Release the resources
            release_resources(customer_num, release);
        }
    }
    
    return NULL;
}

void print_state() {
    printf("\nCurrent system state:\n");
    
    printf("Available resources: ");
    for (int i = 0; i < NUMBER_OF_RESOURCES; i++) {
        printf("%d ", available[i]);
    }
    printf("\n");
    
    printf("Maximum demand:\n");
    for (int i = 0; i < NUMBER_OF_CUSTOMERS; i++) {
        printf("Customer %d: ", i);
        for (int j = 0; j < NUMBER_OF_RESOURCES; j++) {
            printf("%d ", maximum[i][j]);
        }
        printf("\n");
    }
    
    printf("Allocation:\n");
    for (int i = 0; i < NUMBER_OF_CUSTOMERS; i++) {
        printf("Customer %d: ", i);
        for (int j = 0; j < NUMBER_OF_RESOURCES; j++) {
            printf("%d ", allocation[i][j]);
        }
        printf("\n");
    }
    
    printf("Need:\n");
    for (int i = 0; i < NUMBER_OF_CUSTOMERS; i++) {
        printf("Customer %d: ", i);
        for (int j = 0; j < NUMBER_OF_RESOURCES; j++) {
            printf("%d ", need[i][j]);
        }
        printf("\n");
    }
    printf("\n");
}