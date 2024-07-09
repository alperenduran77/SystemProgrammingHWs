#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h> // For open() constants
#include <string.h> // For strlen()
#include <time.h>
#define MAX_STUDENTS 250

// Function prototypes
void addStudentGrade(const char* nameSurname, const char* grade, const char* fileName);
void searchStudent(const char* nameSurname, const char* fileName);
void sortAll(const char* filePath, int sortMode);
void showAll(const char* filePath);
void listGrades(const char* filePath);
void listSome(int numEntries, int pageNumber, const char* filePath);
void printUsage();
void logMessage(const char* logFilePath, const char* operation, const char* message);
void ensureFileExists(const char* filePath);

typedef struct {
    char name[100]; // Adjust size as necessary
    char grade[3];  // Assuming grade format is "AA", "BB", etc.
} StudentInfo;


int compareByNameAsc(const void* a, const void* b) {
    const StudentInfo* studentA = (const StudentInfo*)a;
    const StudentInfo* studentB = (const StudentInfo*)b;
    return strcmp(studentA->name, studentB->name);
}

// Compare by name descending
int compareByNameDesc(const void* a, const void* b) {
    return compareByNameAsc(b, a); // Simply reverse the arguments for descending order
}

// Compare by grade descending
int compareByGradeDesc(const void* a, const void* b) {
    const StudentInfo* studentA = (const StudentInfo*)a;
    const StudentInfo* studentB = (const StudentInfo*)b;
    return strcmp(studentA->grade, studentB->grade);
}

// Compare by grade asscending
int compareByGradeAsc(const void* a, const void* b) {
    return compareByGradeDesc(b, a); // Reverse arguments for ascending order
}


int main() {
    

    while (1) {
        fflush(stdout); // Make sure the Enter command: is displayed immediately

        char command[512];
        if (fgets(command, sizeof(command), stdin) == NULL) {
            continue; // If no input is detected, continue to the next iteration of the loop
        }
        command[strcspn(command, "\n")] = 0; // Remove trailing newline

        // Using dynamic memory allocation to store tokens
        char* args[10]; // Assuming a maximum of 10 arguments for simplicity
        int argCount = 0;
        char* token = strtok(command, " ");
        /*no need to free this elements because i did not use malloc calloc */
        while (token != NULL && argCount < 10) {
            args[argCount++] = token;
            token = strtok(NULL, " ");
        }

        if (argCount == 0) continue; // If no command is entered, prompt again

        // Process commands
       if (strcmp(args[0], "addStudentGrade") == 0 && argCount >= 5) {
    char fullName[256] = ""; // Initialize fullName with an empty string

    // Start at index 1 to skip the command itself and end before the last two arguments (grade and fileName)
    // This accounts for names with multiple parts by concatenating everything before the grade and fileName
    for (int i = 1; i < argCount - 2; ++i) {
        strcat(fullName, args[i]); // Add the current name part
        if (i < argCount - 2) strcat(fullName, " "); // Add space if this is not the last part of the name
    }

    addStudentGrade(fullName, args[argCount - 2], args[argCount - 1]);
}

 
        else if (strcmp(args[0], "searchStudent") == 0 && argCount >= 4) {
    char fullName[256] = ""; // Initialize fullName with an empty string
    
    // Start at index 1 to skip the command itself and end before the last argument (fileName)
    // This loop concatenates all parts of the name, correctly handling spaces
    for (int i = 1; i < argCount - 2; ++i) {
        strcat(fullName, args[i]); // Add the current name part
        if (i < argCount - 2) strcat(fullName, " "); // Add space if this is not the last part of the name
    }
    
    // Ensure to add the last part of the name without an extra space
    strcat(fullName, args[argCount - 2]);

    searchStudent(fullName, args[argCount - 1]);
}

        else if (strcmp(args[0], "sortAll") == 0 && argCount >= 2) {
            int sortMode = 1;
            sortMode = atoi(args[2]);
            sortAll(args[1], sortMode);
        }
        else if (strcmp(args[0], "showAll") == 0 && argCount == 2) {
            showAll(args[1]);
        }
        else if (strcmp(args[0], "listGrades") == 0 && argCount == 2) {
            listGrades(args[1]);
        }
        else if (strcmp(args[0], "listSome") == 0 && argCount == 4) {
            int numOfEntries = atoi(args[1]);
            int pageNumber = atoi(args[2]);
            listSome(numOfEntries, pageNumber, args[3]);
        }
        else if(strcmp(args[0], "gtuStudentGrades") == 0 && argCount >= 2)
        {
            ensureFileExists(args[1]);
        }

        else if (strcmp(args[0], "gtuStudentGrades") == 0) {
            printUsage();
        }
        else if (strcmp(args[0], "exit") == 0) {
            printf("Exiting program...\n");
            break; // Exit the loop and end the program
        }
        else {
            printf("Invalid command!\n");
            printUsage(); // Show usage help
        }
    }

    return 0;
}


void printUsage() {
    printf("\tUsage:\n");
    printf("1) To add a student grade:addStudentGrade Name Surname Grade grades.txt\n");
    printf("2) To search for a student grade:searchStudent Name Surname grades.txt\n");
    printf("3) To sort all grades by student name:sortAll grades.txt\n");
    printf(" -----------------Sorting Options--------------\n");
    printf("sortAll grades.txt or sortAll grades.txt 1 =>Sorted by Name ascending\n");
    printf("sortAll grades.txt 2 => Sorted by Name descending\n");
    printf("sortAll grades.txt 3 => Sorted by Grades descending\n");
    printf("sortAll grades.txt 4 => Sorted by Grades ascending\n");
    printf("-----------------------------------------------------------\n");
    printf("4) To display all student grades:showAll grades.txt\n");
    printf("5) To display the first 5 student grades:listGrades grades.txt\n");
    printf("6) To display a specific number of grades from a specific page:listSome <numOfEntries> <pageNumber> grades.txt\n");
    printf("7) Example:listSome 5 2 grades.txt (displays entries from the 6th to the 10th)\n\n");
    printf("8) To display usage:gtuStudentGrades\n");
    printf("9) To creata a file:gtuStudentGrades grades.txt\n");
    printf("10) Write exit to quit the program\n");
   
}

void addStudentGrade(const char* nameSurname, const char* grade, const char* fileName) {
    pid_t pid = fork();

    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        logMessage("operations.log", "addStudentGrade", "Operation started.");
        
        // Child process
        int file = open(fileName, O_WRONLY | O_CREAT | O_APPEND, 0666);
        if (file == -1) {
            perror("open");
            exit(EXIT_FAILURE);
        }
        
        char buffer[256];
        int length = snprintf(buffer, sizeof(buffer), "%s, %s\n", nameSurname, grade);
        
        if (write(file, buffer, length) != length) {
            perror("write");
            close(file);
            exit(EXIT_FAILURE);
        }
        
        char logBuffer[256];
        snprintf(logBuffer, sizeof(logBuffer), "Successfully added grade for %s.", nameSurname);
        logMessage("operations.log", "addStudentGrade", logBuffer);

        close(file);
        exit(EXIT_SUCCESS);
    } else {
        int status;
        wait(&status);
        if (status == 0) {
            printf("Student grade added successfully.\n");
        } else {
            logMessage("operations.log", "addStudentGrade", "Failed to add student grade.");
            printf("Failed to add student grade.\n");
        }
    }
}


void searchStudent(const char* nameSurname, const char* fileName) {
    pid_t pid = fork();

    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        logMessage("operations.log", "searchStudent", "Operation started.");
        int file = open(fileName, O_RDONLY);
        if (file == -1) {
            perror("open");
            exit(EXIT_FAILURE);
        }
       
        
        if (file == -1) {
            perror("open");
            exit(EXIT_FAILURE);
        }

        char buffer[1024]; // Increased buffer size for larger reads
        ssize_t bytes_read;
        int found = 0;

        // Temporary buffer to hold one line at a time
        char line[256];
        int lineIndex = 0;

        while ((bytes_read = read(file, buffer, sizeof(buffer) - 1)) > 0) {
            for (ssize_t i = 0; i < bytes_read; ++i) {
                if (buffer[i] == '\n' || lineIndex == sizeof(line) - 1) {
                    // End of line or maximum line length reached, null-terminate and process
                    line[lineIndex] = '\0';
                    if (strstr(line, nameSurname) != NULL) {
                        printf("%s\n", line);
                        found = 1;
                        break;
                    }
                    lineIndex = 0; // Reset line index for the next line
                } else {
                    // Accumulate characters into the line buffer
                    line[lineIndex++] = buffer[i];
                }
            }
            if (found) 
            {
                char logBuffer[256];
    snprintf(logBuffer, sizeof(logBuffer), "Student found: %s.", nameSurname);
    logMessage("operations.log", "searchStudent", logBuffer);
                break; // Break if the student is found
        
            }
            }
        


        close(file);

        
        if (!found) {
            char logBuffer[256];
    snprintf(logBuffer, sizeof(logBuffer), "Student not found: %s.", nameSurname);
            logMessage("operations.log", "searchStudent", logBuffer);
            printf("Student '%s' not found.\n", nameSurname);
        }
        
        exit(EXIT_SUCCESS);
    } else {
        int status;
        wait(&status);
        if (status != 0) {
            printf("Search operation failed.\n");
        
    }
}
}
void sortAll(const char* filePath, int sortMode) {
    pid_t pid = fork();
   
    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        // Child process handles the sorting
        logMessage("operations.log", "sortAll", "Operation started.");
        int file = open(filePath, O_RDONLY);
        if (file == -1) {
            perror("Failed to open file for reading");
            exit(EXIT_FAILURE);
        }

        int (*compareFunc)(const void*, const void*);
        switch (sortMode) {
            case 1: compareFunc = compareByNameAsc; break;
            case 2: compareFunc = compareByNameDesc; break;
            case 3: compareFunc = compareByGradeDesc; break;
            case 4: compareFunc = compareByGradeAsc; break;
            default: compareFunc = compareByNameAsc; // Default to name ascending
        }

        StudentInfo students[MAX_STUDENTS];
        int studentCount = 0;
        char buffer[256];
        ssize_t bytes_read;
        while ((bytes_read = read(file, buffer, sizeof(buffer) - 1)) > 0 && studentCount < MAX_STUDENTS) {
            buffer[bytes_read] = '\0';
            char* line = strtok(buffer, "\n");
            while (line != NULL && studentCount < MAX_STUDENTS) {
                sscanf(line, "%99[^,], %2s", students[studentCount].name, students[studentCount].grade);
                studentCount++;
                line = strtok(NULL, "\n");
            }
        }
        close(file);

        // Sort the students array based on the specified mode
        qsort(students, studentCount, sizeof(StudentInfo), compareFunc);

        switch(sortMode)
    {
        case 1: 
            logMessage("operations.log", "sortAll", "Displayed students sorted ascending ordered by name.");
            printf("Students sorted ascending ordered by name.\n");
            break;
        case 2: 
            logMessage("operations.log", "sortAll", "Displayed students sorted descending ordered by name.");
            printf("Students sorted descending ordered by name.\n");
            break;
        case 3: 
            logMessage("operations.log", "sortAll", "Displayed students sorted descending ordered by grade.");
            printf("Students sorted descending ordered by grades.\n");
            break;
        case 4: 
            logMessage("operations.log", "sortAll", "Displayed students sorted asscending ordered by grade.");
            printf("Students sorted asscending ordered by grades.\n");
            break;            
    }

        // Instead of writing back to the file, print sorted students to the terminal
        for (int i = 0; i < studentCount; i++) {
            printf("%s, %s\n", students[i].name, students[i].grade);
        }

       

        exit(EXIT_SUCCESS); // Child process exits successfully
    } else {
        // Parent process waits for the child to finish
        int status;
        wait(&status); // Wait for the child process to complete
        if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
            logMessage("operations.log", "sortAll", "Sorting failed.");
        }
    }
}
void showAll(const char* filePath) {
    // Log the start of the operation
    logMessage("operations.log", "showAll", "Operation started.");

    pid_t pid = fork();

    if (pid == -1) {
        // Fork failed
        perror("fork");
        logMessage("operations.log", "showAll", "Fork failed for displaying all grades.");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        // Child process
        int file = open(filePath, O_RDONLY);
        if (file == -1) {
            perror("Failed to open file");
            logMessage("operations.log", "showAll", "Failed to open grades file in child process.");
            exit(EXIT_FAILURE);
        }

        char buffer[256];
        ssize_t bytes_read;
        while ((bytes_read = read(file, buffer, sizeof(buffer) - 1)) > 0) {
            buffer[bytes_read] = '\0';
            write(STDOUT_FILENO, buffer, bytes_read); // Use write() to display the content
        }

        close(file);
        exit(EXIT_SUCCESS);
    } else {
        // Parent process waits for the child to complete
        int status;
        wait(&status);
        if (status != 0) {
            printf("Failed to display file content.\n");
            logMessage("operations.log", "showAll", "Failed to display all grades.");
        } else {
            logMessage("operations.log", "showAll", "Displayed all student grades successfully.");
        }
    }
}
void listGrades(const char* filePath) {
    pid_t pid = fork();
    
    
    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        logMessage("operations.log", "listGrades", "Operation started.");
        // Child process for listing grades
        int file = open(filePath, O_RDONLY);
        if (file == -1) {
            perror("open");
            exit(EXIT_FAILURE);
        }

        char buffer[256];
        ssize_t bytes_read;
        int lineCount = 0, i = 0;
        while ((bytes_read = read(file, buffer + i, 1)) > 0 && lineCount < 5) {
            if (buffer[i] == '\n') {
                buffer[i+1] = '\0'; // Null-terminate the string
                write(STDOUT_FILENO, buffer, i+1);
                lineCount++;
                i = 0; // Reset buffer index for the next line
                continue;
            }
            i++;
            if (i >= (int)(sizeof(buffer) - 2)) {
                buffer[i] = '\0';
                write(STDOUT_FILENO, buffer, i); // Print what's been read so far
                i = 0; // Reset buffer index
            }
        }

        close(file);
        exit(EXIT_SUCCESS);
    } else {
        // Parent process waits for the child to finish
        int status;
        wait(&status);
        if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
            logMessage("operations.log", "listGrades", "Listing grades completed successfully.");
        } else {
            logMessage("operations.log", "listGrades", "Listing grades failed.");
        }
    }
}

void listSome(int numEntries, int pageNumber, const char* filePath) {
    logMessage("operations.log", "listSome", "Operation started.");

    pid_t pid = fork();

    if (pid == -1) {
        perror("fork");
        logMessage("operations.log", "listSome", "Fork failed.");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        // Child process
        int file = open(filePath, O_RDONLY);
        if (file == -1) {
            perror("Failed to open file");
            logMessage("operations.log", "listSome", "Failed to open file in child process.");
            exit(EXIT_FAILURE);
        }

        int startLine = (pageNumber - 1) * numEntries + 1;
        int endLine = startLine + numEntries - 1;
        int currentLine = 0;
        char buffer[256 + 1]; // +1 for null terminator
        ssize_t bytesRead;
        int lineIndex = 0;

        while ((bytesRead = read(file, buffer + lineIndex, 1)) > 0) {
            if (buffer[lineIndex] == '\n' || lineIndex == 256) {
                buffer[lineIndex + 1] = '\0'; // Null terminate the string
                currentLine++;

                if (currentLine >= startLine && currentLine <= endLine) {
                    write(STDOUT_FILENO, buffer, lineIndex + 1);
                } else if (currentLine > endLine) {
                    break; // Stop reading the file if end of page is reached
                }

                lineIndex = -1; // Reset line index for the next line
            }

            lineIndex++;
            if (lineIndex == 256) { // Prevent buffer overflow
                lineIndex = 0; // Reset buffer index
            }
        }

        close(file);
        exit(EXIT_SUCCESS);
    } else {
        // Parent process waits for the child to complete
        int status;
        wait(&status);
        if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
            logMessage("operations.log", "listSome", "Listing completed successfully.");
        } else {
            logMessage("operations.log", "listSome", "Listing failed.");
        }
    }
}

void logMessage(const char* logFilePath, const char* operation, const char* message) {
    pid_t pid = fork();
    
    if (pid == -1) {
        // Fork failed
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        // Child process
        int fd = open(logFilePath, O_WRONLY | O_CREAT | O_APPEND, 0666);
        if (fd == -1) {
            perror("Failed to open log file");
            exit(EXIT_FAILURE);
        }

        // Get current time
        time_t now = time(NULL);
        char* timeStr = ctime(&now);
        timeStr[24] = '\0'; // Remove the newline at the end of the time string

        // Construct the log message
        char logEntry[1024]; // Ensure this buffer is large enough for your log message
        int len = snprintf(logEntry, sizeof(logEntry), "[%s] %s: %s\n", timeStr, operation, message);
        
        // Write the log message
        if (write(fd, logEntry, len) != len) {
            perror("Failed to write log entry");
        }

        // Close the file
        close(fd);
        
        exit(EXIT_SUCCESS); // Ensure child exits successfully
    } else {
        // Parent process
        int status;
        wait(&status); // Wait for the child process to complete
    }
}

void ensureFileExists(const char* filePath) {
    pid_t pid = fork();

    if (pid == -1) {
        // Fork failed
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        // Child process
        int fd = open(filePath, O_WRONLY | O_CREAT, 0666); // Open in write-only mode, create file if it doesn't exist
        if (fd == -1) {
            perror("open");
            exit(EXIT_FAILURE);
        }
        close(fd);
        exit(EXIT_SUCCESS); // Child exits after ensuring the file exists
    } else {
        // Parent process
        int status;
        wait(&status); // Wait for the child process to complete
        if (status != 0) {
            printf("Child process failed to ensure file existence.\n");
            exit(EXIT_FAILURE);
        }
    }
}