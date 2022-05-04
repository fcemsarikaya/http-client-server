/**     
*@file client.c
*@author Fidel Cem Sarikaya <fcemsarikaya@gmail.com>
*@date 03.01.2022
*
*@brief Client program
*
* This client program implements HTTP 1.1 and connects to a server to obtain a file using sockets.
**/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>

static char *MYPROG;

/**
 * Usage function.
 * @brief This function writes helpful usage information to stderr about how program should be called.
 */
static void usage(char* message) {
    fprintf(stderr,"Usage Error! \tProper input: %s [-p PORT] [ -o FILE | -d DIR ] URL\n%s\n", MYPROG, message);
    exit(1);
    }

/**
 * Dynamic resource management function.
 * @brief This function frees some certain resources when it determines to be necessary according to certain conditions.
 * @details Specifically written for purposes of main(). If D is true, both resources are freed. If O, only 'oO' is freed.
 * @param D A boolean.
 * @param O A boolean.
 * @param oD A dynamic char array.
 * @param oO A dynamic char array.
*/
static void freeResources(bool D, bool O, char* oD, char* oO) {
    if (D) {
        free(oD);
        free(oO);
    }
    else if (O) {
        free(oO);
        }
    }

/**
 * Program entry point.
 * @brief The program starts here, and takes an URL from the user, which it will access and obtain the data file located there to transmit it to the user.
 * @details The program generates a request message to transmit to the relevant server by examining the URL and then creates relevant socket connection to send 
 * the request. After receiving the relevant file from server's end, 'client' saves the obtained data to a file with given name, if -o option is used. If -d is used,
 * the data is saved to the given directory. If neither are used, data is written to 'stdout'. There is also -p option, which runs the program with given port
 * number.
 * @param argc The argument counter.
 * @param argv The argument vector.
 * @return Returns EXIT_SUCCESS or EXIT_FAILURE.
 */
int main(int argc, char *argv[]) {

    MYPROG = argv[0];
    char port[7] = "80";

    char* outputFileName;
    char* outputDirectory;
    char* url;

    bool O_isUsed = false;
    bool D_isUsed = false;

    int opt;
    while((opt = getopt(argc, argv, "p:o:d:")) != -1) 
    { 
        switch(opt) 
        { 
            case 'p':
                if (optarg == NULL) {
                    freeResources(D_isUsed, O_isUsed, outputDirectory, outputFileName);
                    usage("Missing argument to the option 'p'\n");
                }

                char* endPointer;
                strtol(optarg, &endPointer, 10);
                if (endPointer == optarg || strlen(optarg) > 6) {
                    freeResources(D_isUsed, O_isUsed, outputDirectory, outputFileName);
                    usage("Invalid argument to the option 'p'\n");
                }
                strcpy(port, optarg);
                break; 
            case 'o': 
                if (D_isUsed) {
                    freeResources(D_isUsed, O_isUsed, outputDirectory, outputFileName);
                    usage("Options 'o' and 'd' can't be used together");
                }
                else if (optarg == NULL) {
                    freeResources(D_isUsed, O_isUsed, outputDirectory, outputFileName);
                    usage("Missing argument to the option 'o'\n");
                }
                outputFileName = malloc(sizeof(optarg));
                strcpy(outputFileName, optarg);
                O_isUsed = true;
                break;
            case 'd': 
                if (O_isUsed) {
                    freeResources(D_isUsed, O_isUsed, outputDirectory, outputFileName);
                    usage("Options 'o' and 'd' can't be used together");
                }
                else if (optarg == NULL) {
                    freeResources(D_isUsed, O_isUsed, outputDirectory, outputFileName);
                    usage("Missing argument to the option 'o'");
                }

                url = argv[argc - 1];
                if (url == NULL) {
                    freeResources(D_isUsed, O_isUsed, outputDirectory, outputFileName);
                    usage("Missing URL");
                }

                outputDirectory = malloc(sizeof(optarg));
                strcpy(outputDirectory, optarg);

                DIR* dir = opendir(outputDirectory);
                if (ENOENT == errno) {
                    freeResources(D_isUsed, O_isUsed, outputDirectory, outputFileName);
                    usage("Invalid directory");
                    }
                closedir(dir);
                
                int index;
                for (int i = 0; i < strlen(url); i++){
                    if (url[i] == '/') {index = i;}
                }
                
                if (index == strlen(url) - 1) {
                    outputFileName = malloc(sizeof(char) * 11);
                    strcpy(outputFileName, "index.html");
                }
                else {
                    outputFileName = malloc(sizeof(char) * (strlen(url) - index + 2));
                    strcpy(outputFileName, &url[index + 1]);
                }
                
                D_isUsed = true;
                break;     
            case '?':
                freeResources(D_isUsed, O_isUsed, outputDirectory, outputFileName);
                usage("Unknown Option!");
                break; 
            default:
                assert(0);
                break;
        } 
    }
  

    url = argv[optind];
    if (url == NULL || strlen(url) < 8) {
        freeResources(D_isUsed, O_isUsed, outputDirectory, outputFileName);
        usage("Invalid URL");
    }

    if (argc > 6 || argc < 2) {
        freeResources(D_isUsed, O_isUsed, outputDirectory, outputFileName);
        usage("Too many or lacking input arguments");
    }


    char* checkList = ";/:@=&";
    char* tempCheck = strpbrk(&url[7], checkList);

    int hostLength = strlen(url) - strlen(tempCheck) - 7;
    char hostName[hostLength + 1];
    strncpy(hostName, &url[7], hostLength);
    hostName[hostLength] = '\0';



    char* requestedFileName = strchr(&url[7], '/');
    char requestMessage[39 + strlen(requestedFileName) + strlen(hostName)];
    sprintf(requestMessage, "GET %s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n", requestedFileName, hostName);


    //socket struct setup
    struct addrinfo hints, *ai, *results;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;


    int res = getaddrinfo(hostName, port, &hints, &results);
    if (res != 0) {
        fprintf(stderr, "getaddrinfo() failed");
        freeResources(D_isUsed, O_isUsed, outputDirectory, outputFileName);
        freeaddrinfo(results);
        exit(EXIT_FAILURE);
    }
    fprintf(stdout, "Connecting to the host...\n\n");
    int sockfd;
    for (ai = results; ai != NULL; ai = ai->ai_next) {
        sockfd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
        if (sockfd == -1) {
            continue;
        }
        
        if (connect(sockfd, ai->ai_addr, ai->ai_addrlen) != -1) {
            break;
        }
        close(sockfd);
    }
    freeaddrinfo(results);

    if (ai == NULL) {
        freeResources(D_isUsed, O_isUsed, outputDirectory, outputFileName);
        perror("socket() or connect() failed");
        exit(EXIT_FAILURE);
    }
    
    if (send(sockfd, requestMessage, strlen(requestMessage), 0) < 0) {
        freeResources(D_isUsed, O_isUsed, outputDirectory, outputFileName);
        perror("send() failed");
        exit(EXIT_FAILURE);
    }
    
    char buffer[1512] = {0};
    if (recv(sockfd, buffer, sizeof(buffer), MSG_WAITALL) < 0) {
        freeResources(D_isUsed, O_isUsed, outputDirectory, outputFileName);
        perror("recv() failed");
        exit(EXIT_FAILURE);
    }
    close(sockfd);

    char buff1[1512];
    strcpy(buff1, buffer);

    char buff2[1512];
    strcpy(buff2, buffer);

    char buff3[1512];
    strcpy(buff3, buffer);

    char* token;
    token = strtok(buff1, " ");
    char* firstWord = token;

    token = strtok(NULL, " ");
    char* secondWord = token;

    char* endPointer2;
    int responseStatus = strtol(secondWord, &endPointer2, 10);

    if (strcmp(endPointer2, secondWord) == 0 || strcmp(firstWord, "HTTP/1.1") != 0) {
        fprintf(stderr, "Protocol error!");
        freeResources(D_isUsed, O_isUsed, outputDirectory, outputFileName);
        exit(2);
    }
    if (responseStatus != 200) {
        token = strtok(buff2, "\n");
        char* status = strchr(&token[1], ' ');
        fprintf(stderr, status);
        freeResources(D_isUsed, O_isUsed, outputDirectory, outputFileName);
        exit(3);
    }
    char *tempPtr = strstr(buffer, "\r\n\r\n");

    if (D_isUsed) {
        char pathFile[strlen(outputDirectory) + strlen(outputFileName) + 3];

        if (outputDirectory[strlen(outputDirectory) - 1] == '/') {
            sprintf(pathFile, "%s%s", outputDirectory, outputFileName);
        }
        else {
            sprintf(pathFile, "%s/%s", outputDirectory, outputFileName);
        }

        FILE *outputFile = fopen(pathFile, "w");
        fputs(&tempPtr[4], outputFile);
        fclose(outputFile);

        free(outputDirectory);
        free(outputFileName);
    }
    else {
        if (O_isUsed) {
            FILE *outputFile = fopen(outputFileName, "w");
            fputs(&tempPtr[4], outputFile);
            fclose(outputFile);
            
            free(outputFileName);
        }
        else {
            fprintf(stdout, "%s\n", &tempPtr[4]);
        }
    }
    return EXIT_SUCCESS;
}