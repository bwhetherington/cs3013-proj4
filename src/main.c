#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>
#include <pthread.h>

#include "message.h"

#define DEFAULT_CHUNK_SIZE 1024
#define MAX_CHUNK_SIZE 8192
#define DEFAULT_THREADS 1
#define MAX_THREADS 16

// This is horrible practice but for some reason it wasn't recognized?
extern int fileno(FILE *stream);

void printInfo(int numBytes, char *string, int count) {
    printf("File size: %d bytes.\n", numBytes);
    printf("Occurrences of the string \"%s\": %d\n", string, count);
}

void runSearchStep(int index, int *stringIndex, int *foundCount, char *buf, char *string, int len) {
    if (buf[index] == string[*stringIndex]) {
        (*stringIndex)++;
        if (*stringIndex == len) {
            // Reset search and increment found if match
            *stringIndex = 0;
            (*foundCount)++;
        }
    } else {
        // Reset search
        *stringIndex = 0;
    }
}

int MASTER;
int FILE_DESCRIPTOR;
char *SEARCH;
int SEARCH_LEN;
int NUM_BYTES;

void searchThread(void *input) {
    int threadIndex = (int) input;

    msg_t msg;
    recvMsg(threadIndex, &msg);

    int start = msg.start;
    int stop = msg.stop;

    int foundCount = 0;
    int stringIndex = 0;

    // We will need to search until (len + (stringlen - 1)) to ensure that we include strings over boundaries
    char *mapped = mmap(NULL, NUM_BYTES, PROT_READ, MAP_SHARED, FILE_DESCRIPTOR, 0);
    for (int i = start; i < stop; i++) {
        runSearchStep(i, &stringIndex, &foundCount, mapped, SEARCH, SEARCH_LEN);
    }

    // Cleanup
    munmap(mapped, NUM_BYTES);

    msg_t output;
    initMsg(&output, threadIndex, 0, foundCount, 0);
    sendMsg(MASTER, &output);
}

typedef struct {
    int start;
    int stop;
} section_t;

void initSections(section_t *sections, int numThreads, int numBytes, int searchLen) {
    int sectionLen = numBytes / numThreads;

    // Each section except for the last one will be (numBytes / numThreads + searchLen - 1)
    section_t *section;
    for (int i = 0; i < numThreads - 1; i++) {
        section = &sections[i];
        section->start = sectionLen * i;
        section->stop = section->start + sectionLen + searchLen - 1;
    }

    // Last section will be (numBytes / numThreads * (numThreads - 1) to the end)
    section_t *final = &sections[numThreads - 1];
    final->start = sectionLen * (numThreads - 1);
    final->stop = numBytes;
}

void countMmap(char *fileName, char *searchString, int numThreads) {
    int fileDescriptor = open(fileName, O_RDONLY);
    FILE_DESCRIPTOR = fileDescriptor;
    SEARCH = searchString;
    SEARCH_LEN = strlen(SEARCH);

    // Check to see that open worked
    if (fileDescriptor < 0) {
        fprintf(stderr, "open failed\n");
        exit(EXIT_FAILURE);
    }

    struct stat statInfo;
    fstat(fileDescriptor, &statInfo);
    int byteCount = statInfo.st_size;
    NUM_BYTES = byteCount;

    // Initialize threads
    // Create mailboxes
    initMailboxes(numThreads + 1);

    // Mailbox to coordinate child threads
    MASTER = numThreads;

    // Create sections
    section_t sections[numThreads];
    initSections(sections, numThreads, byteCount, strlen(searchString));

    // Send initial messages
    section_t section;
    msg_t send;
    for (int i = 0; i < numThreads; i++) {
        section = sections[i];
        initMsg(&send, MASTER, NULL, section.start, section.stop);
        sendMsg(i, &send);
    }

    // Create threads
    pthread_t threads[numThreads];
    for (int i = 0; i < numThreads; i++) {
        pthread_create(threads + i, NULL, searchThread, i);
    }

    // Count all subsections
    int foundCount = 0;
    msg_t recv;
    for (int i = 0; i < numThreads; i++) {
        recvMsg(MASTER, &recv);

        // Consider start to be the counted occurences in the section
        foundCount += recv.start;
    }

    // Cleanup
    close(fileDescriptor);
    cleanupMailboxes();

    // Display results
    printInfo(byteCount, searchString, foundCount);
}

void countRead(FILE *file, int chunkSize, char *searchString) {
    int fileDescriptor = fileno(file);
    struct stat statInfo;
    fstat(fileDescriptor, &statInfo);
    int byteCount = statInfo.st_size;

    // Information for counting search string
    int foundCount = 0;
    int stringIndex = 0;
    int stringLen = strlen(searchString);

    // Keep track of the last buffer where 
    char buf[chunkSize];
    int bytesRead;

    // Read bytes in chunks
    while ((bytesRead = read(fileDescriptor, buf, chunkSize)) != 0) {
        // Check each byte
        for (int i = 0; i < bytesRead; i++) {
            runSearchStep(i, &stringIndex, &foundCount, buf, searchString, stringLen);
        }
    }
   
    printInfo(byteCount, searchString, foundCount);
}

int main(int argc, char **argv) {
    // Parse arguments
    // format: ./proj4 <file> <search> <size | [threads] mmap>
    char *fileName;
    char *searchString;
    int chunkSize = DEFAULT_CHUNK_SIZE;
    int numThreads = DEFAULT_THREADS;
    int mmap = 0;

    if (argc < 3 || argc > 5) {
        fprintf(stderr, "usage: %s <file> <search> <size | threads | threads mmap | mmap>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // File to read
    fileName = argv[1];
    FILE *file = fopen(fileName, "r");

    // String to search for
    searchString = argv[2];

    // Parse remainder

    // Check if chunked
    if (argc == 4) {
        if (strcmp(argv[3], "mmap") == 0) {
            mmap = 1;
        } else if (argv[3][0] == 'p') {
            mmap = 1;
            numThreads = atoi(argv[3] + 1);
            if (numThreads > MAX_THREADS) {
                fprintf(stderr, "warning: %d exceeds the maximum thread count of %d; the maximum thread count will be used instead\n", numThreads, MAX_THREADS);
                numThreads = MAX_THREADS;
            }
        } else {
            chunkSize = atoi(argv[3]);
            if (chunkSize > MAX_CHUNK_SIZE) {
                fprintf(stderr, "warning: %d bytes exceeds the maximum chunk size of %d bytes; the maximum chunk size will be used instead\n", chunkSize, MAX_CHUNK_SIZE);
                chunkSize = MAX_CHUNK_SIZE;
            }
        }
    } else if (argc == 5) {
        if (strcmp(argv[4], "mmap") == 0) {
            if (argv[3][0] == 'p') {
                numThreads = atoi(argv[3] + 1);
                if (numThreads > MAX_THREADS) {
                    fprintf(stderr, "warning: %d exceeds the maximum thread count of %d; the maximum thread count will be used instead\n", numThreads, MAX_THREADS);
                    numThreads = MAX_THREADS;
                }
            } else {
                fprintf(stderr, "thread count must be preceeded by 'p'\n");
                exit(EXIT_FAILURE);
            }
            mmap = 1;
        } else {
            fprintf(stderr, "final argument must be mmap\n");
            exit(EXIT_FAILURE);
        }
    }

    // // Print out arguments for debugging purposes
    // if (mmap) {
    //     printf("----- Arguments -----\n");
    //     printf("file    = %s\n", fileName);
    //     printf("search  = \"%s\"\n", searchString);
    //     printf("mode    = mmap\n");
    //     printf("threads = %d\n", numThreads);
    //     printf("---------------------\n");
    // } else {
    //     printf("----- Arguments -----\n");
    //     printf("file    = %s\n", fileName);
    //     printf("search  = \"%s\"\n", searchString);
    //     printf("mode    = read\n");
    //     printf("size    = %d\n", chunkSize);
    //     printf("---------------------\n");
    // }

    if (mmap) {
        // Do mmap version
        countMmap(fileName, searchString, numThreads);
    } else {
        // Do read version
        countRead(file, chunkSize, searchString);
    }
}

//read
//reading
//read