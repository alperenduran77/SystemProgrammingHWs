#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

#define MAX_AUTOMOBILES 8
#define MAX_PICKUPS 4
#define NUM_CAR_OWNERS 45 // Adjust this to simulate more arrivals

sem_t newAutomobile, inChargeforAutomobile;
sem_t newPickup, inChargeforPickup;
sem_t entryMutex;
pthread_mutex_t mutexParking;

int free_auto = 1; // Temporary spots for automobiles
int free_pickup = 1;     // Temporary spots for pickups
int mFree_automobile = 0;
int mFree_pickup = 0;
volatile int terminate = 0; // Using volatile for visibility across threads

void handle_alarm(int sig) {
    terminate = 1;
    sem_post(&newAutomobile); // Unblock any waiting threads
    sem_post(&newPickup); // Unblock any waiting threads
}

void* carOwner(void* param) {
    int vehicleType = *(int*)param;
    free(param); // Free the allocated memory for vehicleType

    while (!terminate) {
        usleep(rand() % 1000000); // Random delay to simulate arrival times

        sem_wait(&entryMutex); // Only one vehicle can enter the attempt to park
        pthread_mutex_lock(&mutexParking);

        if (terminate) {
            pthread_mutex_unlock(&mutexParking);
            sem_post(&entryMutex);
            break;
        }

        // Check if all permanent spots are filled
        if ((vehicleType == 0 && mFree_automobile >= MAX_AUTOMOBILES) ||
            (vehicleType == 1 && mFree_pickup >= MAX_PICKUPS)) {
               
            if ((vehicleType == 0 && free_auto > 0) ||
                (vehicleType == 1 && free_pickup > 0)) {
                if (vehicleType == 0) {
                    
                    free_auto--;
                    printf("Automobile owner parked in temporary lot but no permanent spots left.\n");
                    free_auto++;
                } else {
                   
                    free_pickup--;
                    printf("Pickup owner parked in temporary lot but no permanent spots left.\n");
                    free_pickup++;
                }
            }
            pthread_mutex_unlock(&mutexParking);
            sem_post(&entryMutex);
            break;
        }

        // Check for available temporary spot
        if ((vehicleType == 0 && free_auto > 0) ||
            (vehicleType == 1 && free_pickup > 0)) {
            if (vehicleType == 0) {
                free_auto--;
            } else {
                free_pickup--;
            }
            pthread_mutex_unlock(&mutexParking);
            printf("%s owner parked in temporary lot. Vale looks for an empty spot for %s.\n", vehicleType == 0 ? "Automobile" : "Pickup", vehicleType == 0 ? "Automobile" : "Pickup");
            sem_post(vehicleType == 0 ? &newAutomobile : &newPickup);
            sem_post(&entryMutex);
            sem_wait(vehicleType == 0 ? &inChargeforAutomobile : &inChargeforPickup); // Wait for attendant to park in permanent spot
        } else {
            pthread_mutex_unlock(&mutexParking);
            printf("No temporary spots available. %s owner left.\n", vehicleType == 0 ? "Automobile" : "Pickup");
            sem_post(&entryMutex);
            break;
        }
    }
    return NULL;
}

void* carAttendant(void* param) {
    int vehicleType = *(int*)param;
    sem_t* myNewSem = vehicleType == 0 ? &newAutomobile : &newPickup;
    sem_t* myInChargeSem = vehicleType == 0 ? &inChargeforAutomobile : &inChargeforPickup;
    int* myPermanentCounter = vehicleType == 0 ? &mFree_automobile : &mFree_pickup;
    int maxSpots = vehicleType == 0 ? MAX_AUTOMOBILES : MAX_PICKUPS;

    while (1) {
        sem_wait(myNewSem); // Wait for a vehicle to be ready to move to permanent

        pthread_mutex_lock(&mutexParking);

        if (terminate) {
            pthread_mutex_unlock(&mutexParking);
            break;
        }

        if (*myPermanentCounter < maxSpots) {
            (*myPermanentCounter)++;
            printf("%s moved to permanent lot by vale. Remaining permanent %s spots: %d.\n",
                   vehicleType == 0 ? "Automobile" : "Pickup",
                   vehicleType == 0 ? "automobile" : "pickup",
                   maxSpots - *myPermanentCounter);
            if (vehicleType == 0) {
                free_auto++;
            } else {
                free_pickup++;
            }
            sem_post(myInChargeSem); // Notify the car owner that the vehicle is parked
        }

        pthread_mutex_unlock(&mutexParking);
    }
    return NULL;
}

void* vehicleDeparture(void* param) {
    while (!terminate) {
        sleep(2); // Wait for 5 seconds before removing a vehicle

        pthread_mutex_lock(&mutexParking);
        if (mFree_automobile > 0 && (rand() % 2 == 0 || mFree_pickup == 0)) {
            mFree_automobile--;
            printf("Automobile left the permanent lot. Remaining permanent automobile spots: %d.\n", MAX_AUTOMOBILES - mFree_automobile);
            free_auto++;
            sem_post(&newAutomobile);
            printf("Automobile owner parked in temporary lot. Vale looks for an empty spot for Automobile.\n"); // New vehicle parks in the temporary lot
        } else if (mFree_pickup > 0) {
            mFree_pickup--;
            printf("Pickup left the permanent lot. Remaining permanent pickup spots: %d.\n", MAX_PICKUPS - mFree_pickup);
            free_pickup++;
            sem_post(&newPickup);
            printf("Pickup owner parked in temporary lot. Vale looks for an empty spot for Pickup.\n"); // New vehicle parks in the temporary lot
        }
        pthread_mutex_unlock(&mutexParking);
    }
    return NULL;
}

int main() {
    signal(SIGALRM, handle_alarm);
    alarm(20); // Set an alarm for 20 seconds

    srand(time(NULL));
    pthread_t carOwnerThreads[NUM_CAR_OWNERS];
    pthread_t autoAttendantThread, pickupAttendantThread, departureThread;

    sem_init(&newAutomobile, 0, 0);
    sem_init(&inChargeforAutomobile, 0, 0);
    sem_init(&newPickup, 0, 0);
    sem_init(&inChargeforPickup, 0, 0);
    sem_init(&entryMutex, 0, 1);
    pthread_mutex_init(&mutexParking, NULL);

    // Create car owner threads with random vehicle types
    for (int i = 0; i < NUM_CAR_OWNERS; i++) {
        int* vehicleType = malloc(sizeof(int));
        *vehicleType = rand() % 2; // Randomly choose between 0 (automobile) and 1 (pickup)
        pthread_create(&carOwnerThreads[i], NULL, carOwner, vehicleType);
    }

    // Create attendant threads
    int autoType = 0, pickupType = 1;
    pthread_create(&autoAttendantThread, NULL, carAttendant, &autoType);
    pthread_create(&pickupAttendantThread, NULL, carAttendant, &pickupType);

    // Create vehicle departure thread
    pthread_create(&departureThread, NULL, vehicleDeparture, NULL);

    // Sleep for 20 seconds before termination
    sleep(20);
    terminate = 1; // Signal termination

    // Wait for car owner threads to complete
    for (int i = 0; i < NUM_CAR_OWNERS; i++) {
        pthread_join(carOwnerThreads[i], NULL);
    }

    // Wait for departure and attendant threads to complete
    pthread_join(departureThread, NULL);
    pthread_join(autoAttendantThread, NULL);
    pthread_join(pickupAttendantThread, NULL);

    sem_destroy(&newAutomobile);
    sem_destroy(&inChargeforAutomobile);
    sem_destroy(&newPickup);
    sem_destroy(&inChargeforPickup);
    sem_destroy(&entryMutex);
    pthread_mutex_destroy(&mutexParking);

    printf("Simulation complete. All vehicles processed.\n");
    return 0;
}
