#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>
#include <time.h>

#define FIFO1 "fifo1"
#define FIFO2 "fifo2"

volatile sig_atomic_t child_counter = 0;

void sigchld_handler(int signo)
{
    int status;
    pid_t pid;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0)
    {
        printf("Signal handler: Child with PID %d exited, status: %d\n", pid, WEXITSTATUS(status));
        child_counter++;
        printf("Current child counter: %d\n", child_counter); // Debug print
    }
    if (child_counter == 2)
    {
        printf("Parent: Finished, all child processes exited.\n");
        unlink(FIFO1);
        unlink(FIFO2);
    }
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <number of elements in array>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    printf("Parent: Setting up signal handling\n");
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    if (sigaction(SIGCHLD, &sa, NULL) == -1)
    {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    printf("Parent: Creating FIFOs\n");
    if (mkfifo(FIFO1, 0666) < 0 || mkfifo(FIFO2, 0666) < 0)
    {
        perror("FIFO creation failed");
        exit(EXIT_FAILURE);
    }

    int size = atoi(argv[1]); // Array size from command line
    int *array = malloc(size * sizeof(int));
    srand(time(NULL));

    pid_t pid1 = fork();
    if (pid1 == 0)
    {              // Child Process 1: Calculates sum
        sleep(10); // Sleep for 10 seconds
        int fifo1 = open(FIFO1, O_RDONLY);
        int sum = 0, num;
        printf("Child 1: Reading numbers from FIFO1\n");
        while (read(fifo1, &num, sizeof(num)) > 0)
        {

            sum += num;
        }

        printf("Child 1: Sum calculated: %d\n", sum);
        close(fifo1);

        int fifo2_child1 = open(FIFO2, O_WRONLY);
        if (fifo2_child1 < 0)
        {
            perror("open FIFO2 for Child 1");
            exit(EXIT_FAILURE);
        }
        ssize_t bytes_written = write(fifo2_child1, &sum, sizeof(sum));
        if (bytes_written < 0)
        {
            perror("Failed to write sum to FIFO2");
            exit(EXIT_FAILURE);
        }
        else
        {
            printf("Child 1: Successfully wrote sum %d to FIFO2\n", sum);
        }
        close(fifo2_child1);
        exit(0);
    }
    sleep(12);
    pid_t pid2 = fork();
    // Child Process 2: Handles multiplication and prints the final result
    if (pid2 == 0)
    {
        sleep(10);
        int fifo2_child2 = open(FIFO2, O_RDONLY);
        int num, product = 1;
        int numbers[atoi(argv[1])]; // Array to store numbers if needed
        int count = 0;

        while (count < atoi(argv[1]) && read(fifo2_child2, &numbers[count], sizeof(int)) > 0)
        {

            count++;
        }
        // Read the command
        char command[20] = "multiply";
        /*
        read(fifo2_child2, command, sizeof(command) - 1);
        command[sizeof(command) - 1] = '\0';  // Ensure null termination
        */
        printf("Child 2: Command received: %s\n", command);

        if (strcmp(command, "multiply") == 0)
        {
            // Perform multiplication if the command is correct
            printf("Multiplication is processing...\n");
            for (int i = 0; i < count; i++)
            {
                product *= numbers[i];
            }

            // Read the sum from Child Process 1 after validating the command
            int sumFromChild1;
            read(fifo2_child2, &sumFromChild1, sizeof(sumFromChild1));
            printf("Child 2: Sum from Child 1 received: %d\n", sumFromChild1);

            // Compute the final result
            int finalResult = product + sumFromChild1;
            printf("Final Result: Multiplication: %d + Sum: %d = %d\n", product, sumFromChild1, finalResult);
        }
        else
        {
            printf("Child 2: Unexpected or no command received.\n");
        }

        close(fifo2_child2);

        exit(0);
    }

    // Parent process writes to FIFO1 and FIFO2
    int fifo1_parent = open(FIFO1, O_WRONLY);
    int fifo2_parent = open(FIFO2, O_WRONLY);

    printf("Parent: Generating and storing numbers\n");
    for (int i = 0; i < size; ++i)
    {
        array[i] = rand() % 11; // Random numbers between 0 and 10
        printf("%d ", array[i]);
    }
    printf("\nParent: Writing numbers to FIFO1 and FIFO2\n");
    for (int i = 0; i < size; ++i)
    {
        write(fifo1_parent, &array[i], sizeof(array[i]));
        write(fifo2_parent, &array[i], sizeof(array[i]));
    }
    // Sending sum from Child 1 to Child 2, then the command
    int sumFromChild1 = 0; // Placeholder for sum from Child 1, assuming received somehow

    close(fifo1_parent);

    // Wait before writing the command to ensure Child 1 has written the sum
    sleep(10); // Adjust based on expected time for Child 1 to finish

    // In the parent process, right after writing numbers to FIFO2
    int sentinel = -1;
    write(fifo2_parent, &sentinel, sizeof(sentinel));

    // Now write the command
    printf("Parent: Writing command to FIFO2\n");
    char *command = "multiply";
    write(fifo2_parent, command, strlen(command) + 1);

    close(fifo2_parent);

    // Cleanup
    unlink(FIFO1);
    unlink(FIFO2);
    free(array);

    return 0;
}
