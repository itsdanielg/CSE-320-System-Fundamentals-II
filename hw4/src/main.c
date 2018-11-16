#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <getopt.h>

#include "imprimer.h"
#include "helpers.h"
#include <readline/readline.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>

/*
 * "Imprimer" printer spooler.
 */

// DECLARE FILE TYPE ARRAY
int fileTypeCount;
int printerCount;
int jobCount;
int conversionCount;
char** supportedFileTypes;
PRINTER** allPrinters;
JOB** allJobs;
char*** allConversions;

int main(int argc, char *argv[])
{
    int inputBatch = 0;
    int outputBatch = 0;
    char optval;
    while(optind < argc) {
    	if((optval = getopt(argc, argv, "")) != -1) {
    	    switch(optval) {
        	    case '?':
        		fprintf(stderr, "Usage: %s [-i <cmd_file>] [-o <out_file>]\n", argv[0]);
        		exit(EXIT_FAILURE);
        		break;
                case 'i':
                inputBatch = 1;
                // DO SOMETHING
                break;
                case 'o':
                outputBatch = 1;
                // DO SOMETHING
                break;
        	    default:
        		break;
    	    }
    	}
    }

    fileTypeCount = 0;
    supportedFileTypes = malloc(sizeof(char*));
    printerCount = 0;
    allPrinters = malloc(sizeof(PRINTER*) * MAX_PRINTERS);
    jobCount = 0;
    allJobs = malloc(sizeof(JOB*));
    conversionCount = 0;
    allConversions = malloc(sizeof(char**) * 2);
    // CONVERSION ARRAY CONSISTS OF {
    //      [FILETYPEONE, FILETYPE 2, ARGCOUNT],
    //      ARGV[] {CONVERSIONPROGRAM, arg1, arg2, etc..}
    // }

    if (inputBatch) {
        // File IO
    }
    else {
        if (outputBatch) {
            // Redirect Output to File
        }
        start_imp();
    }

    int i;
    // FREE ALL BLOCKS
    // FILE TYPES
    for (i = 0; i < fileTypeCount; i++) {
        free(supportedFileTypes[i]);
    }
    free(supportedFileTypes);
    supportedFileTypes = NULL;
    // PRINTERS
    for (i = 0; i < printerCount; i++) {
        free(allPrinters[i]->name);
        free(allPrinters[i]->type);
        free(allPrinters[i]);
    }
    free(allPrinters);
    allPrinters = NULL;
    // JOBS
    for (i = 0; i < jobCount; i++) {
        free(allJobs[i]->file_name);
        free(allJobs[i]->file_type);
    }
    free(allJobs);
    allJobs = NULL;
    // CONVERSIONS
    for (i = 0; i < conversionCount; i++) {
        int j;
        char argCount = allConversions[i * 2][2][0];
        for (j = 0; j < argCount + 1; j++) {
            free(allConversions[(i * 2) + 1][j]);
        }
        free(allConversions[(i * 2) + 1]);
        for (j = 0; j < 3; j++) {
            free(allConversions[i * 2][j]);
        }
        free(allConversions[i * 2]);
    }
    free(allConversions);
    allConversions = NULL;

    exit(EXIT_SUCCESS);
}

void start_imp() {
    char* command;
    command = readline("imp> ");
    // IF THERE ARGUMENTS ENTERED
    if (!(command[0] == '\0' || command == NULL)) {
        // INITIALIZE STRING ARRAY FOR COMMANDS
        char** commandArr;
        int commandCount = word_count(command);
        // IF READLINE DOESN'T ONLY DETECT WHITESPACE CHARACTERS
        if (commandCount) {
            commandArr = (char**)malloc(sizeof(char*) * commandCount);
            // SPlIT COMMAND LINE ARGUMENTS INTO STRING ARRAY
            split_string(command, commandArr, commandCount);
            // FREE THE STRING RETURNED BY READLINE (SINCE IT IS NOW SPLIT INTO AN ARRAY)
            free(command);
            // LOOK AT FIRST COMMANDS
            // HELP COMMAND
            if (strcmp(commandArr[0], "help") == 0) {
                if (commandCount != 1) {
                    char* errorMessage = "This command takes no arguments";
                    int size = sizeof(char) * 20 + strlen(errorMessage);
                    char* buffer = malloc(size);
                    printf("%s\n", imp_format_error_message(errorMessage, buffer, size));
                    free(buffer);
                }
                else {
                    list_help();
                }
            }
            // QUIT COMMAND
            else if (strcmp(commandArr[0], "quit") == 0) {
                if (commandCount != 1) {
                    char* errorMessage = "This command takes no arguments";
                    int size = sizeof(char) * 20 + strlen(errorMessage);
                    char* buffer = malloc(size);
                    printf("%s\n", imp_format_error_message(errorMessage, buffer, size));
                    free(buffer);
                }
                else {
                    free(commandArr[0]);
                    free(commandArr);
                    commandArr = NULL;
                    return;
                }
            }
            // TYPE COMMAND
            else if (strcmp(commandArr[0], "type") == 0) {
                if (commandCount != 2) {
                    char* errorMessage = "This command must take one argument";
                    int size = sizeof(char) * 20 + strlen(errorMessage);
                    char* buffer = malloc(size);
                    printf("%s\n", imp_format_error_message(errorMessage, buffer, size));
                    free(buffer);
                }
                else {
                    int fileTypeExists = 0;
                    for (int i = 0; i < fileTypeCount; i++) {
                        if (strcmp(supportedFileTypes[i], commandArr[1]) == 0) {
                            fileTypeExists = 1;
                            break;
                        }
                    }
                    if (fileTypeExists) {
                        char* errorMessage = "This file type is already supported";
                        int size = sizeof(char) * 20 + strlen(errorMessage);
                        char* buffer = malloc(size);
                        printf("%s\n", imp_format_error_message(errorMessage, buffer, size));
                        free(buffer);
                    }
                    else {
                        if (fileTypeCount == MAX_PRINTERS * 2) {
                            char* errorMessage = "Cannot add anymore file types";
                            int size = sizeof(char) * 20 + strlen(errorMessage);
                            char* buffer = malloc(size);
                            printf("%s\n", imp_format_error_message(errorMessage, buffer, size));
                            free(buffer);
                        }
                        else {
                            fileTypeCount++;
                            if (fileTypeCount != 1) {
                                supportedFileTypes = realloc(supportedFileTypes, sizeof(char*) * fileTypeCount);
                            }
                            char* fileType = malloc(sizeof(char) * strlen(commandArr[1]) + 1);
                            strcpy(fileType, commandArr[1]);
                            supportedFileTypes[fileTypeCount - 1] = fileType;
                        }

                    }
                }
            }
            // PRINTER COMMAND
            else if (strcmp(commandArr[0], "printer") == 0) {
                if (commandCount != 3) {
                    char* errorMessage = "This command must take two arguments";
                    int size = sizeof(char) * 20 + strlen(errorMessage);
                    char* buffer = malloc(size);
                    printf("%s\n", imp_format_error_message(errorMessage, buffer, size));
                    free(buffer);
                }
                else {
                    if (printerCount == MAX_PRINTERS) {
                        char* errorMessage = "Maximum number of printers reached";
                        int size = sizeof(char) * 20 + strlen(errorMessage);
                        char* buffer = malloc(size);
                        printf("%s\n", imp_format_error_message(errorMessage, buffer, size));
                        free(buffer);
                    }
                    else {
                        int printerExists= 0;
                        for (int i = 0; i < printerCount; i++) {
                            if (strcmp(allPrinters[i]->name, commandArr[1]) == 0) {
                                printerExists = 1;
                                break;
                            }
                        }
                        if (printerExists) {
                            char* errorMessage = "This printer name already exists";
                            int size = sizeof(char) * 20 + strlen(errorMessage);
                            char* buffer = malloc(size);
                            printf("%s\n", imp_format_error_message(errorMessage, buffer, size));
                            free(buffer);
                        }
                        else {
                            char* name = malloc(sizeof(char) * strlen(commandArr[1]) + 1);
                            char* type = malloc(sizeof(char) * strlen(commandArr[2]) + 1);
                            strcpy(name, commandArr[1]);
                            strcpy(type, commandArr[2]);
                            PRINTER* printer = malloc(sizeof(PRINTER));
                            *printer = (PRINTER) {
                                .id = printerCount,
                                .name = name,
                                .type = type,
                                .enabled = 0,
                                .busy = 0
                            };
                            allPrinters[printerCount] = printer;
                            printerCount++;
                        }
                    }
                }
            }
            // CONVERSION COMMAND
            else if (strcmp(commandArr[0], "conversion") == 0) {
                if (commandCount < 4) {
                    char* errorMessage = "This command must take at least three arguments";
                    int size = sizeof(char) * 20 + strlen(errorMessage);
                    char* buffer = malloc(size);
                    printf("%s\n", imp_format_error_message(errorMessage, buffer, size));
                    free(buffer);
                }
                else {
                    int fileTypeDeclared = 0;
                    for (int i = 0; i < fileTypeCount; i++) {
                        if (strcmp(supportedFileTypes[i], commandArr[1]) == 0) {
                            fileTypeDeclared = 1;
                            break;
                        }
                    }
                    if (!fileTypeDeclared) {
                        char* errorMessage = "The first file type is not declared";
                        int size = sizeof(char) * 20 + strlen(errorMessage);
                        char* buffer = malloc(size);
                        printf("%s\n", imp_format_error_message(errorMessage, buffer, size));
                        free(buffer);
                    }
                    else {
                        fileTypeDeclared = 0;
                        for (int i = 0; i < fileTypeCount; i++) {
                            if (strcmp(supportedFileTypes[i], commandArr[2]) == 0) {
                                fileTypeDeclared = 1;
                                break;
                            }
                        }
                        if (!fileTypeDeclared) {
                            char* errorMessage = "The second file type is not declared";
                            int size = sizeof(char) * 20 + strlen(errorMessage);
                            char* buffer = malloc(size);
                            printf("%s\n", imp_format_error_message(errorMessage, buffer, size));
                            free(buffer);
                        }
                        else {
                            // IF THERE ARE CONVERSIONS DECLARED ALREADY
                            if (conversionCount) {
                                // ERROR CHECK: CHECK IF NAME OF CONVERSION PROGRAM ALREADY EXISTS
                                int nameExists = 0;
                                for (int i = 0; i < conversionCount; i++) {
                                    if (strcmp(allConversions[i*2][2], commandArr[2]) == 0) {
                                        nameExists = 1;
                                        break;
                                    }
                                }
                                if (nameExists) {
                                    char* errorMessage = "This conversion program has already been declared";
                                    int size = sizeof(char) * 20 + strlen(errorMessage);
                                    char* buffer = malloc(size);
                                    printf("%s\n", imp_format_error_message(errorMessage, buffer, size));
                                    free(buffer);
                                }
                                else {
                                    // ERROR CHECK: CHECK IF PAIR OF FILE TYPES ALREADY IN CONVERSION
                                    int pairExists = 0;
                                    for (int i = 0; i < conversionCount; i++) {
                                        if ((strcmp(allConversions[i*2][0], commandArr[0]) == 0) && (strcmp(allConversions[i*2][1], commandArr[2]) == 1)) {
                                            pairExists = 1;
                                            break;
                                        }
                                    }
                                    if (pairExists) {
                                        char* errorMessage = "This pair of file types to be converted has already been declared";
                                        int size = sizeof(char) * 20 + strlen(errorMessage);
                                        char* buffer = malloc(size);
                                        printf("%s\n", imp_format_error_message(errorMessage, buffer, size));
                                        free(buffer);
                                    }
                                    else {
                                        conversionCount++;
                                        allConversions = realloc(allConversions, sizeof(char**) * conversionCount * 2);
                                        char** conversionList = malloc(sizeof(char*) * 3);
                                        char* fileTypeOne = malloc(sizeof(char) * strlen(commandArr[0]) + 1);
                                        char* fileTypeTwo = malloc(sizeof(char) * strlen(commandArr[1]) + 1);
                                        char* argCount = malloc(sizeof(char));
                                        strcpy(fileTypeOne, commandArr[0]);
                                        strcpy(fileTypeTwo, commandArr[1]);
                                        char additionalArgs = commandCount - 3;
                                        argCount[0] = additionalArgs;
                                        conversionList[0] = fileTypeOne;
                                        conversionList[1] = fileTypeTwo;
                                        conversionList[2] = argCount;
                                        char** argVArray = malloc(sizeof(char*) * (additionalArgs + 1));
                                        char* program = malloc(sizeof(char) * strlen(commandArr[2]) + 1);
                                        strcpy(program, commandArr[2]);
                                        argVArray[0] = program;
                                        argVArray[1] = NULL;
                                        for (int i = 0; i < additionalArgs; i++) {
                                            char* argument = malloc(sizeof(char) * strlen(commandArr[i + 3]) + 1);
                                            strcpy(argument, commandArr[i + 3]);
                                            argVArray[i + 1] = argument;
                                        }
                                        allConversions[(conversionCount - 1) * 2] = conversionList;
                                        allConversions[((conversionCount - 1) * 2) + 1] = argVArray;
                                    }
                                }
                            }
                            // ELSE IF THERE ARE NO CONVERSIONS DECLARED YET
                            else {
                                conversionCount++;
                                char** conversionList = malloc(sizeof(char*) * 3);
                                char* fileTypeOne = malloc(sizeof(char) * strlen(commandArr[0]) + 1);
                                char* fileTypeTwo = malloc(sizeof(char) * strlen(commandArr[1]) + 1);
                                char* argCount = malloc(sizeof(char));
                                strcpy(fileTypeOne, commandArr[0]);
                                strcpy(fileTypeTwo, commandArr[1]);
                                char additionalArgs = commandCount - 3;
                                argCount[0] = additionalArgs;
                                conversionList[0] = fileTypeOne;
                                conversionList[1] = fileTypeTwo;
                                conversionList[2] = argCount;
                                char** argVArray = malloc(sizeof(char*) * (additionalArgs + 1));
                                char* program = malloc(sizeof(char) * strlen(commandArr[2]) + 1);
                                strcpy(program, commandArr[2]);
                                argVArray[0] = program;
                                argVArray[1] = NULL;
                                for (int i = 0; i < additionalArgs; i++) {
                                    char* argument = malloc(sizeof(char) * strlen(commandArr[i + 3]) + 1);
                                    strcpy(argument, commandArr[i + 3]);
                                    argVArray[i + 1] = argument;
                                }
                                allConversions[0] = conversionList;
                                allConversions[1] = argVArray;
                            }
                        }
                    }
                }
            }
            // PRINTERS COMMAND
            else if (strcmp(commandArr[0], "printers") == 0) {
                if (commandCount != 1) {
                    char* errorMessage = "This command takes no arguments";
                    int size = sizeof(char) * 20 + strlen(errorMessage);
                    char* buffer = malloc(size);
                    printf("%s\n", imp_format_error_message(errorMessage, buffer, size));
                    free(buffer);
                }
                else {
                    for (int i = 0; i < printerCount; i++) {
                        int size = sizeof(char) * 20 + sizeof(PRINTER);
                        char* buffer = malloc(size);
                        printf("%s\n", imp_format_printer_status(allPrinters[i], buffer, size));
                        free(buffer);
                    }
                }
            }
            // JOBS COMMAND
            else if (strcmp(commandArr[0], "jobs") == 0) {
                if (commandCount != 1) {
                    char* errorMessage = "This command takes no arguments";
                    int size = sizeof(char) * 20 + strlen(errorMessage);
                    char* buffer = malloc(size);
                    printf("%s\n", imp_format_error_message(errorMessage, buffer, size));
                    free(buffer);
                }
                else {
                    for (int i = 0; i < jobCount; i++) {
                        int size = sizeof(char) * 20 + sizeof(JOB);
                        char* buffer = malloc(size);
                        printf("%s\n", imp_format_job_status(allJobs[i], buffer, size));
                        free(buffer);
                    }
                }
            }
            // PRINT COMMAND
            else if (strcmp(commandArr[0], "print") == 0) {
                if (commandCount < 2) {
                    char* errorMessage = "This command must take at least one argument";
                    int size = sizeof(char) * 20 + strlen(errorMessage);
                    char* buffer = malloc(size);
                    printf("%s\n", imp_format_error_message(errorMessage, buffer, size));
                    free(buffer);
                }
                else {
                    // GET EXTENSION OF FILE
                    char* extension = get_extension(commandArr[1]);
                    if (strcmp(extension, "") == 0) {
                        char* errorMessage = "Invalid file entered";
                        int size = sizeof(char) * 20 + strlen(errorMessage);
                        char* buffer = malloc(size);
                        printf("%s\n", imp_format_error_message(errorMessage, buffer, size));
                        free(buffer);
                    }
                    else {
                        int fileSupported = 0;
                        for (int i = 0; i < fileTypeCount; i++) {
                            if (strcmp(supportedFileTypes[i], extension) == 0) {
                                fileSupported = 1;
                                break;
                            }
                        }
                        if (!fileSupported) {
                            char* errorMessage = "This file type is not supported";
                            int size = sizeof(char) * 20 + strlen(errorMessage);
                            char* buffer = malloc(size);
                            printf("%s\n", imp_format_error_message(errorMessage, buffer, size));
                            free(buffer);
                        }
                        else {
                            // IF THERE ADDITIONAL PRINTER ARGUMENTS
                            if (commandCount > 2) {
                                int printersNotDeclared = commandCount - 2;
                                for (int i = 2; i < commandCount; i++) {
                                    for (int j = 0; j < printerCount; j++) {
                                        char* printerName = allPrinters[j]->name;
                                        if (strcmp(printerName, commandArr[i]) == 0) {
                                            printersNotDeclared -= 1;
                                            break;
                                        }
                                    }
                                }
                                if (printersNotDeclared == commandCount - 2) {
                                    char* errorMessage = "ALl printers not been declared";
                                    int size = sizeof(char) * 20 + strlen(errorMessage);
                                    char* buffer = malloc(size);
                                    printf("%s\n", imp_format_error_message(errorMessage, buffer, size));
                                    free(buffer);
                                }
                                else if (printersNotDeclared) {
                                    char* errorMessage = "Some printers not been declared";
                                    int size = sizeof(char) * 20 + strlen(errorMessage);
                                    char* buffer = malloc(size);
                                    printf("%s\n", imp_format_error_message(errorMessage, buffer, size));
                                    free(buffer);
                                }
                                else {
                                    PRINTER_SET eligible_printers = 0;
                                    for (int i = 2; i < commandCount; i++) {
                                        for (int j = 0; j < printerCount; j++) {
                                            char* printerName = allPrinters[j]->name;
                                            if (strcmp(printerName, commandArr[i]) == 0) {
                                                eligible_printers |= 1 << allPrinters[j]->id;
                                            }
                                        }
                                    }
                                    int printerId = -1;
                                    for (int i = 2; i < commandCount; i++) {
                                        for (int j = 0; j < printerCount; j++) {
                                            char* printerName = allPrinters[j]->name;
                                            if (strcmp(printerName, commandArr[i]) == 0) {
                                                if (allPrinters[j]->enabled) {
                                                    printerId = j;
                                                }
                                                break;
                                            }
                                        }
                                    }
                                    if (printerId == -1) {
                                        printf("Job is queued. No printers have been enabled\n");
                                    }
                                    else {
                                        printerId = -1;
                                        for (int i = 2; i < commandCount; i++) {
                                            for (int j = 0; j < printerCount; j++) {
                                                char* printerName = allPrinters[j]->name;
                                                if (strcmp(printerName, commandArr[i]) == 0) {
                                                    if (!(allPrinters[j]->busy)) {
                                                        printerId = j;
                                                    }
                                                    break;
                                                }
                                            }
                                        }
                                        if (printerId == -1) {
                                            printf("Job is queued. All printers are busy\n");
                                        }
                                        else {
                                            int stopLoop = 0;
                                            printerId = -1;
                                            for (int i = 2; i < commandCount; i++) {
                                                for (int j = 0; j < printerCount; j++) {
                                                    char* printerName = allPrinters[j]->name;
                                                    if (strcmp(printerName, commandArr[i]) == 0) {
                                                        if (strcmp(allPrinters[j]->type, extension) == 0) {
                                                            printerId = j;
                                                            stopLoop = 1;
                                                            break;
                                                        }
                                                        // CHECK IF CONVERSION IS AVAILABLE FOR PRINTER
                                                        else {
                                                            char* currentFileType = extension;
                                                            char* endFileType = allPrinters[j]->type;
                                                            int printerFound = conversion_search(currentFileType, endFileType, conversionCount, allConversions);
                                                            if (printerFound) {
                                                                printerId = j;
                                                                stopLoop = 1;
                                                                break;
                                                            }
                                                        }
                                                    }
                                                }
                                                if (stopLoop) {
                                                    break;
                                                }
                                            }
                                            if (printerId == -1) {
                                                printf("Job is queued. Printers cannot support file type\n");
                                            }
                                        }
                                    }
                                    jobCount++;
                                    if (jobCount != 1) {
                                        supportedFileTypes = realloc(supportedFileTypes, sizeof(JOB*) * jobCount);
                                    }
                                    PRINTER* printerToUse = NULL;
                                    if (printerId != -1) {
                                        printerToUse = allPrinters[printerId];
                                    }
                                    JOB* job = malloc(sizeof(JOB));
                                    char* file_name = malloc(sizeof(char) * strlen(commandArr[1]) + 1);
                                    char* file_type = malloc(sizeof(char) * strlen(extension) + 1);
                                    strcpy(file_name, commandArr[1]);
                                    strcpy(file_type, extension);
                                    struct timeval creation_time = {
                                        .tv_sec = time(0)
                                    };
                                    struct timeval change_time = {
                                        .tv_sec = time(0)
                                    };
                                    *job = (JOB) {
                                        .jobid = jobCount - 1,
                                        .status = QUEUED,
                                        .pgid = 0,
                                        .file_name = file_name,
                                        .file_type = file_type,
                                        .eligible_printers = eligible_printers,
                                        .chosen_printer = printerToUse,
                                        .creation_time = creation_time,
                                        .change_time = change_time
                                    };
                                    allJobs[jobCount - 1] = job;
                                    // CONNECT PRINTER HERE?
                                    check_jobs();
                                }
                            }
                            // ELSE IF THERE ARE NO ADDITIONAL PRINTER ARGUMENTS
                            else {
                                PRINTER_SET eligible_printers = ANY_PRINTER;
                                int printerId = -1;
                                if (printerCount == 0) {
                                    printf("Job is queued. No printers have been declared\n");
                                }
                                else {
                                    for (int i = 0; i < printerCount; i++) {
                                        if (allPrinters[i]->enabled) {
                                            printerId = i;
                                        }
                                        break;
                                    }
                                    if (printerId == -1) {
                                        printf("Job is queued. No printers have been enabled\n");
                                    }
                                    else {
                                        printerId = -1;
                                        for (int i = 0; i < printerCount; i++) {
                                            if (!(allPrinters[i]->busy)) {
                                                printerId = i;
                                            }
                                            break;
                                        }
                                        if (printerId == -1) {
                                            printf("Job is queued. All printers are busy\n");
                                        }
                                        else {
                                            printerId = -1;
                                            for (int i = 0; i < printerCount; i++) {
                                                if (strcmp(allPrinters[i]->type, extension) == 0) {
                                                    printerId = i;
                                                    break;
                                                }
                                                // CHECK IF CONVERSION IS AVAILABLE FOR PRINTER
                                                else {
                                                    char* currentFileType = extension;
                                                    char* endFileType = allPrinters[i]->type;
                                                    int printerFound = conversion_search(currentFileType, endFileType, conversionCount, allConversions);
                                                    if (printerFound) {
                                                        printerId = i;
                                                        break;
                                                    }
                                                }
                                            }
                                            if (printerId == -1) {
                                                printf("Job is queued. Printers cannot support file type\n");
                                            }
                                        }
                                    }
                                }
                                jobCount++;
                                if (jobCount != 1) {
                                    supportedFileTypes = realloc(supportedFileTypes, sizeof(JOB*) * jobCount);
                                }
                                PRINTER* printerToUse = NULL;
                                if (printerId != -1) {
                                    printerToUse = allPrinters[printerId];
                                }
                                JOB* job = malloc(sizeof(JOB));
                                char* file_name = malloc(sizeof(char) * strlen(commandArr[1]) + 1);
                                char* file_type = malloc(sizeof(char) * strlen(extension) + 1);
                                strcpy(file_name, commandArr[1]);
                                strcpy(file_type, extension);
                                struct timeval creation_time = {
                                    .tv_sec = time(0)
                                };
                                struct timeval change_time = {
                                    .tv_sec = time(0)
                                };
                                *job = (JOB) {
                                    .jobid = jobCount,
                                    .status = QUEUED,
                                    .pgid = 0,
                                    .file_name = file_name,
                                    .file_type = file_type,
                                    .eligible_printers = eligible_printers,
                                    .chosen_printer = printerToUse,
                                    .creation_time = creation_time,
                                    .change_time = change_time
                                };
                                allJobs[jobCount - 1] = job;
                                // CONNECT PRINTER HERE?
                                check_jobs();
                            }
                        }
                    }
                }
            }
            // CANCEL COMMAND
            else if (strcmp(commandArr[0], "cancel") == 0) {
                if (commandCount != 2) {
                    char* errorMessage = "This command must take one argument";
                    int size = sizeof(char) * 20 + strlen(errorMessage);
                    char* buffer = malloc(size);
                    printf("%s\n", imp_format_error_message(errorMessage, buffer, size));
                    free(buffer);
                }
                else {
                    JOB* currentJob = NULL;
                    for (int i = 0; i < jobCount; i++) {
                        if(allJobs[i]->jobid == *commandArr[1]) {
                            currentJob = allJobs[i];
                            break;
                        }
                    }
                    if (currentJob == NULL) {
                        char* errorMessage = "This job number does not exist";
                        int size = sizeof(char) * 20 + strlen(errorMessage);
                        char* buffer = malloc(size);
                        printf("%s\n", imp_format_error_message(errorMessage, buffer, size));
                        free(buffer);
                    }
                    else {
                        JOB_STATUS status = currentJob->status;
                        if (status == RUNNING || status == QUEUED) {
                            // IF JOB IS BEING PROCESSED IN CONVERSION PIPELINE
                            // THEN TERMINATE PROCESSES BY SENDING SIGTERM TO PROCESS GROUP
                            if (status == RUNNING) {
                                killpg(currentJob->pgid, SIGTERM);
                            }
                            // ELSE IF QUEUED, THEN JUST CHANGE STATE
                            currentJob->status = ABORTED;
                            int size = sizeof(char) * 20 + sizeof(JOB);
                            char* buffer = malloc(size);
                            printf("%s\n", imp_format_job_status(currentJob, buffer, size));
                            free(buffer);
                            int success = check_jobs();
                            if (success) {
                                printf("Pending job has been processed\n");
                            }
                        }
                        else {
                            char* errorMessage = "This job is not currently running";
                            int size = sizeof(char) * 20 + strlen(errorMessage);
                            char* buffer = malloc(size);
                            printf("%s\n", imp_format_error_message(errorMessage, buffer, size));
                            free(buffer);
                        }
                    }
                }
            }
            // PAUSE COMMAND
            else if (strcmp(commandArr[0], "pause") == 0) {
                if (commandCount != 2) {
                    char* errorMessage = "This command must take one argument";
                    int size = sizeof(char) * 20 + strlen(errorMessage);
                    char* buffer = malloc(size);
                    printf("%s\n", imp_format_error_message(errorMessage, buffer, size));
                    free(buffer);
                }
                else {
                    JOB* currentJob = NULL;
                    for (int i = 0; i < jobCount; i++) {
                        if(allJobs[i]->jobid == *commandArr[1]) {
                            currentJob = allJobs[i];
                            break;
                        }
                    }
                    if (currentJob == NULL) {
                        char* errorMessage = "This job number does not exist";
                        int size = sizeof(char) * 20 + strlen(errorMessage);
                        char* buffer = malloc(size);
                        printf("%s\n", imp_format_error_message(errorMessage, buffer, size));
                        free(buffer);
                    }
                    else {
                        JOB_STATUS status = currentJob->status;
                        if (status == RUNNING) {
                            // IF JOB IS BEING PROCESSED IN CONVERSION PIPELINE
                            // THEN PAUSE PROCESSES BY SENDING SIGSTOP TO PROCESS GROUP
                            killpg(currentJob->pgid, SIGSTOP);
                            currentJob->status = PAUSED;
                            int size = sizeof(char) * 20 + sizeof(JOB);
                            char* buffer = malloc(size);
                            printf("%s\n", imp_format_job_status(currentJob, buffer, size));
                            free(buffer);
                            int success = check_jobs();
                            if (success) {
                                printf("Pending job has been processed\n");
                            }
                        }
                        else {
                            char* errorMessage = "This job is not currently running";
                            int size = sizeof(char) * 20 + strlen(errorMessage);
                            char* buffer = malloc(size);
                            printf("%s\n", imp_format_error_message(errorMessage, buffer, size));
                            free(buffer);
                        }
                    }
                }
            }
            // RESUME COMMAND
            else if (strcmp(commandArr[0], "resume") == 0) {
                if (commandCount != 2) {
                    char* errorMessage = "This command must take one argument";
                    int size = sizeof(char) * 20 + strlen(errorMessage);
                    char* buffer = malloc(size);
                    printf("%s\n", imp_format_error_message(errorMessage, buffer, size));
                    free(buffer);
                }
                else {
                    JOB* currentJob = NULL;
                    for (int i = 0; i < jobCount; i++) {
                        if(allJobs[i]->jobid == *commandArr[1]) {
                            currentJob = allJobs[i];
                            break;
                        }
                    }
                    if (currentJob == NULL) {
                        char* errorMessage = "This job number does not exist";
                        int size = sizeof(char) * 20 + strlen(errorMessage);
                        char* buffer = malloc(size);
                        printf("%s\n", imp_format_error_message(errorMessage, buffer, size));
                        free(buffer);
                    }
                    else {
                        JOB_STATUS status = currentJob->status;
                        if (status == PAUSED) {
                            // IF JOB IS BEING PROCESSED IN CONVERSION PIPELINE
                            // THEN PAUSE PROCESSES BY SENDING SIGCONT TO PROCESS GROUP
                            killpg(currentJob->pgid, SIGCONT);
                            currentJob->status = RUNNING;
                            int size = sizeof(char) * 20 + sizeof(JOB);
                            char* buffer = malloc(size);
                            printf("%s\n", imp_format_job_status(currentJob, buffer, size));
                            free(buffer);
                            int success = check_jobs();
                            if (success) {
                                printf("Pending job has been processed\n");
                            }
                        }
                        else {
                            char* errorMessage = "This job is not currently paused";
                            int size = sizeof(char) * 20 + strlen(errorMessage);
                            char* buffer = malloc(size);
                            printf("%s\n", imp_format_error_message(errorMessage, buffer, size));
                            free(buffer);
                        }
                    }
                }
            }
            // DISABLE COMMAND
            else if (strcmp(commandArr[0], "disable") == 0) {
                if (commandCount != 2) {
                    char* errorMessage = "This command must take one argument";
                    int size = sizeof(char) * 20 + strlen(errorMessage);
                    char* buffer = malloc(size);
                    printf("%s\n", imp_format_error_message(errorMessage, buffer, size));
                    free(buffer);
                }
                else {
                    int printerDeclared = 0;
                    for (int i = 0; i < printerCount; i++) {
                        if (strcmp(allPrinters[i]->name, commandArr[1]) == 0) {
                            printerDeclared = i + 1;
                            break;
                        }
                    }
                    if (!printerDeclared) {
                        char* errorMessage = "This printer is not declared";
                        int size = sizeof(char) * 20 + strlen(errorMessage);
                        char* buffer = malloc(size);
                        printf("%s\n", imp_format_error_message(errorMessage, buffer, size));
                        free(buffer);
                    }
                    else {
                        printerDeclared--;
                        if (allPrinters[printerDeclared]->enabled == 0) {
                            char* errorMessage = "This printer is already disabled";
                            int size = sizeof(char) * 20 + strlen(errorMessage);
                            char* buffer = malloc(size);
                            printf("%s\n", imp_format_error_message(errorMessage, buffer, size));
                            free(buffer);
                        }
                        else {
                            allPrinters[printerDeclared]->enabled = 0;
                            int size = sizeof(char) * 20 + sizeof(PRINTER);
                            char* buffer = malloc(size);
                            printf("%s\n", imp_format_printer_status(allPrinters[printerDeclared], buffer, size));
                            free(buffer);
                        }
                    }
                }
            }
            // ENABLE COMMAND
            else if (strcmp(commandArr[0], "enable") == 0) {
                if (commandCount != 2) {
                    char* errorMessage = "This command must take one argument";
                    int size = sizeof(char) * 20 + strlen(errorMessage);
                    char* buffer = malloc(size);
                    printf("%s\n", imp_format_error_message(errorMessage, buffer, size));
                    free(buffer);
                }
                else {
                    int printerDeclared = 0;
                    for (int i = 0; i < printerCount; i++) {
                        if (strcmp(allPrinters[i]->name, commandArr[1]) == 0) {
                            printerDeclared = i + 1;
                            break;
                        }
                    }
                    if (!printerDeclared) {
                        char* errorMessage = "This printer is not declared";
                        int size = sizeof(char) * 20 + strlen(errorMessage);
                        char* buffer = malloc(size);
                        printf("%s\n", imp_format_error_message(errorMessage, buffer, size));
                        free(buffer);
                    }
                    else {
                        printerDeclared--;
                        if (allPrinters[printerDeclared]->enabled == 1) {
                            char* errorMessage = "This printer is already enabled";
                            int size = sizeof(char) * 20 + strlen(errorMessage);
                            char* buffer = malloc(size);
                            printf("%s\n", imp_format_error_message(errorMessage, buffer, size));
                            free(buffer);
                        }
                        else {
                            allPrinters[printerDeclared]->enabled = 1;
                            int size = sizeof(char) * 20 + sizeof(PRINTER);
                            char* buffer = malloc(size);
                            printf("%s\n", imp_format_printer_status(allPrinters[printerDeclared], buffer, size));
                            free(buffer);
                            int success = check_jobs();
                            if (success) {
                                printf("Pending job/jobs have been processed\n");
                            }
                        }
                    }
                }
            }
            // INVALID COMMAND ENTERED
            else {
                char* errorMessage = "This command must take one argument";
                int size = sizeof(char) * 20 + strlen(errorMessage);
                char* buffer = malloc(size);
                printf("%s\n", imp_format_error_message(errorMessage, buffer, size));
                free(buffer);
            }
            // FREE EVERY POINTER ALLOCATED FOR A COMMAND AND THE COMMAND ARRAY OVERALL
            for (int commandIndex = 0; commandIndex < commandCount; commandIndex++) {
                free(commandArr[commandIndex]);
            }
            free(commandArr);
            commandArr = NULL;
            return start_imp();
        }
        else {
            free(command);
            return start_imp();
        }
    }
    else {
        free(command);
        return start_imp();
    }
}



void list_help() {
    printf(
        "\n"
        "MISCALLANEOUS COMMANDS:\n"
        "help\n"
        "quit\n"
        "\n"
        "CONFIGURATION COMMANDS:\n"
        "type (file_type)\n"
        "printer (printer_name) (file_type)\n"
        "conversion (file_type1) (file_type2) (conversion_program) [arg1 arg2 ...]\n"
        "\n"
        "INFORMATIONAL COMMANDS:\n"
        "printers\n"
        "jobs\n"
        "\n"
        "SPOOLING COMMANDS:\n"
        "print (file_name) [printer1 printer2 ...]\n"
        "cancel (job_number)\n"
        "pause (job_number)\n"
        "resume (job_number)\n"
        "disable (printer_name)\n"
        "enable (printer_name)\n"
        "\n"
    );
}

void split_string(char* inputString, char** arrayList, int wordCount) {
    char c = *inputString;
    c = *inputString;
    for (int i = 0; i < wordCount; i++) {
        while (c != '\0') {
            if (isspace(c)) {
                inputString++;
                c = *inputString;
                continue;
            }
            char* tempString = inputString;
            int charCount = 0;
            while (c != ' ' && c != '\0') {
                charCount++;
                tempString++;
                c = *tempString;
            }
            charCount++;
            *arrayList = (char*)malloc(sizeof(char) * charCount);
            memcpy(*arrayList, inputString, (charCount - 1));
            (*arrayList)[charCount - 1] = '\0';
            arrayList++;
            inputString += charCount - 1;
            c = *inputString;
            break;
        }
    }
}

int word_count(char* inputString) {
    int wordCount = 0;
    int wordCheck = 0;
    char c = *inputString;
    while (c != '\0') {
        if (isspace(c)) {
            if (wordCheck) {
                wordCheck = 0;
            }
        }
        else {
            if (!wordCheck) {
                wordCheck = 1;
                wordCount++;
            }
        }
        inputString++;
        c = *inputString;
    }
    return wordCount;
}

char* get_extension(char* fileName) {
    char* dot = strrchr(fileName, '.');
    if (dot == NULL || dot == fileName) {
        return "";
    }
    return ++dot;
}

int conversion_search(char* currentFileType, char* endFileType, int conversionCount, char*** allConversions) {
    for (int i = 0; i < conversionCount; i++) {
        char* fileTypeOne = allConversions[i * 2][0];
        char* fileTypeTwo = allConversions[i * 2][1];
        if (strcmp(fileTypeTwo, endFileType)) {
            return 1;
        }
        else if (strcmp(currentFileType, fileTypeOne) == 0) {
            return conversion_search(fileTypeTwo, endFileType, conversionCount, allConversions);
        }
    }
    return 0;
}

int check_jobs() {
    int returnStatus = 0;
    for (int i = 0; i < jobCount; i++) {
        JOB* currentJob = allJobs[i];
        if (currentJob->status == COMPLETED || currentJob->status == ABORTED) {
            PRINTER* currentPrinter = currentJob->chosen_printer;
            currentPrinter->busy = 0;
            currentJob->chosen_printer = NULL;
        }
        else if (currentJob->status == QUEUED) {
            PRINTER_SET eligible_printers = currentJob->eligible_printers;
            for (int j = 0; j < printerCount; j++) {
                PRINTER* currentPrinter = allPrinters[j];
                char bitIsEligible = eligible_printers & 1;
                if (bitIsEligible) {
                    int printerIsEnabled = currentPrinter->enabled;
                    int printerIsBusy = currentPrinter->busy;
                    if (printerIsEnabled && !printerIsBusy) {
                        for (int i = 0; i < printerCount; i++) {
                            if (strcmp(currentPrinter->type, currentJob->file_type) == 0) {
                                currentPrinter->busy = 1;
                                currentJob->chosen_printer = currentPrinter;
                                currentJob->status = RUNNING;
                                returnStatus = 1;
                                // INITIATE CONVERSION PIPELINE
                                char* currentFileType = currentJob->file_type;
                                char* endFileType = allPrinters[j]->type;
                                int printerFound = conversion_pipeline(currentFileType, endFileType, conversionCount, allConversions, currentJob);
                                if (printerFound) {
                                    // CONNECT TO PRINTER
                                    int fileDescriptor = imp_connect_to_printer(currentPrinter, PRINTER_NORMAL);
                                    if (fileDescriptor == -1) {
                                        char* errorMessage = "Failed to connect to printer";
                                        int size = sizeof(char) * 20 + strlen(errorMessage);
                                        char* buffer = malloc(size);
                                        printf("%s\n", imp_format_error_message(errorMessage, buffer, size));
                                        free(buffer);
                                    }
                                    break;
                                }
                                else {
                                    char* errorMessage = "Conversion pipeline failed";
                                    int size = sizeof(char) * 20 + strlen(errorMessage);
                                    char* buffer = malloc(size);
                                    printf("%s\n", imp_format_error_message(errorMessage, buffer, size));
                                    free(buffer);
                                    break;
                                }
                            }
                            // CHECK IF CONVERSION IS AVAILABLE FOR PRINTER
                            else {
                                char* currentFileType = currentJob->file_type;
                                char* endFileType = currentPrinter->type;
                                int printerFound = conversion_pipeline(currentFileType, endFileType, conversionCount, allConversions, currentJob);
                                if (printerFound) {
                                    currentPrinter->busy = 1;
                                    currentJob->chosen_printer = currentPrinter;
                                    currentJob->status = RUNNING;
                                    // INITIATE CONVERSION PIPELINE
                                    char* currentFileType = currentJob->file_type;
                                    char* endFileType = allPrinters[j]->type;
                                    int printerFound = conversion_pipeline(currentFileType, endFileType, conversionCount, allConversions, currentJob);
                                    if (printerFound) {
                                        // CONNECT TO PRINTER
                                        int fileDescriptor = imp_connect_to_printer(currentPrinter, PRINTER_NORMAL);
                                        if (fileDescriptor == -1) {
                                            char* errorMessage = "Failed to connect to printer";
                                            int size = sizeof(char) * 20 + strlen(errorMessage);
                                            char* buffer = malloc(size);
                                            printf("%s\n", imp_format_error_message(errorMessage, buffer, size));
                                            free(buffer);
                                        }
                                        break;
                                    }
                                    else {
                                        char* errorMessage = "Conversion pipeline failed";
                                        int size = sizeof(char) * 20 + strlen(errorMessage);
                                        char* buffer = malloc(size);
                                        printf("%s\n", imp_format_error_message(errorMessage, buffer, size));
                                        free(buffer);
                                        break;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    return returnStatus;
}

int conversion_pipeline(char* currentFileType, char* endFileType, int conversionCount, char*** allConversions, JOB* currentJob) {
    if (strcmp(currentFileType, endFileType) == 0) {
        setpgid(getpid(), getpid());
        if (fork() == 0) {
            char* argv[] = {"/bin/cat", NULL};
            execve("/bin/cat", argv, NULL);
            currentJob->pgid = getpgid(getpid());
            return 1;
        }
    }
    else {
        for (int i = 0; i < conversionCount; i++) {
            char* fileTypeOne = allConversions[i * 2][0];
            char* fileTypeTwo = allConversions[i * 2][1];
            if (strcmp(fileTypeTwo, endFileType)) {
                setpgid(getpid(), getpid());
                if (fork() == 0) {
                    char** argv = allConversions[(i * 2) + 1];
                    execve(allConversions[(i * 2) + 1][0], argv, NULL);
                    currentJob->pgid = getpgid(getpid());
                    return 1;
                }
            }
            else if (strcmp(currentFileType, fileTypeOne) == 0) {
                setpgid(getpid(), getpid());
                if (fork() == 0) {
                    char** argv = allConversions[(i * 2) + 1];
                    execve(allConversions[(i * 2) + 1][0], argv, NULL);
                    currentJob->pgid = getpgid(getpid());
                    return conversion_search(fileTypeTwo, endFileType, conversionCount, allConversions);;
                }
            }
        }
    }
    return 1;
}