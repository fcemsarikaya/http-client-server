/**     
*@file server.c
*@author Fidel Cem Sarikaya <fcemsarikaya@gmail.com>
*@date 04.01.2022
*
*@brief Main program module.
*
* This server program implements HTTP 1.1 and waits for connections to transmit a file using sockets.
**/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <time.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>

static char *MYPROG;

volatile sig_atomic_t run = 1;
volatile sig_atomic_t waiting = 0;


/**
 * Signal handling function.
 * @brief This function handles the given signal by decrementing a global variable, and
 * terminating the program when necessary.
 * @details global variables: run, waiting.
 * @param signal The given signal.
 */
void handle_signal(int signal) {
    fprintf(stderr, "\nSignal detected: %d\n", signal);
    run = 0;

    if (waiting == 1) {
        exit(0);
        }
    }

/**
 * Usage function.
 * @brief This function writes helpful usage information to stderr about how program should be called.
 */
static void usage(char* message) {
    fprintf(stderr,"Usage Error! \tProper input: %s [-p PORT] [-i INDEX] DOC_ROOT\n%s\n", MYPROG, message);
    exit(1);}

/**
 * Program entry point.
 * @brief The program starts here, and takes a directory from the user to be shared through accepted connections.
 * @details The program receives a directory from the user and waits for socket connections to transmit requested files from
 * this server directory. Different headers are prepared and transitted by the program according to the received request message.
 * Process of waiting for connections can be ended by SIGINT and SIGTERM signals, while -p option can be used to specify a port 
 * number and -i option specifies a file in the directory to be transmitted. 
 * @param argc The argument counter.
 * @param argv The argument vector.
 * @return Returns EXIT_SUCCESS or EXIT_FAILURE.
 */
int main(int argc, char *argv[]) {

    struct sigaction sa;
    sa.sa_handler = &handle_signal;
    sa.sa_flags = SA_RESTART;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    MYPROG = argv[0];
    char port[7] = "8080";
    char defaultFileName[32] = "index.html";
    char* docRoot;

    int opt;
    while((opt = getopt(argc, argv, "p:i:")) != -1) 
    { 
        switch(opt) 
        { 
            case 'p':
                if (optarg == NULL) {
                    usage("Missing argument to the option 'p'\n");
                }

                char* endPointer;
                strtol(optarg, &endPointer, 10);
                if (endPointer == optarg || strlen(optarg) > 6) {
                    usage("Invalid argument to the option 'p'\n");
                }
                strcpy(port, optarg);
                break; 
            case 'i': 
                if (optarg == NULL) {
                    usage("Missing argument to the option 'i'\n");
                }
                if (strlen(optarg) > 31) {
                    usage("Invalid argument to the option 'i'\n");
                }
                strcpy(defaultFileName, optarg);
                break; 
            case '?': 
                usage("Unknown Option!");
                break; 
            default:
                assert(0);
                break;
        } 
    }
    
    docRoot = argv[optind];
    DIR* dir = opendir(docRoot);
    if (ENOENT == errno) {
        usage("Invalid directory");}
    closedir(dir);

    if (argc > 6 || argc < 2) {
        usage("Too many or lacking input arguments");}


    //socket struct setup
    struct addrinfo hints, *ai, *results;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    int res = getaddrinfo(NULL, port, &hints, &results);
    if (res != 0) {
        fprintf(stderr, "getaddrinfo() failed");
        freeaddrinfo(results);
        exit(EXIT_FAILURE);
    }
    
    int sockfd;
    for (ai = results; ai != NULL; ai = ai->ai_next) {
        sockfd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
        if (sockfd == -1) {
            continue;
        }

        int enable = 1;
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable)) == -1) {
            continue;
        }  
        if (bind(sockfd, ai->ai_addr, ai->ai_addrlen) != -1) {
            break;
        }
        
        close(sockfd);
    }

    if (ai == NULL) {
        perror("socket() or bind() failed");
        close(sockfd);
        freeaddrinfo(results);
        exit(EXIT_FAILURE);
    }
    freeaddrinfo(results);

    if (listen(sockfd, 1) < 0) {
        perror("listen() failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    fprintf(stdout, "Waiting for a connection...\n\n");

    while (run == 1) 
    { //inside of while-loop


    waiting = 1;
    int connfd = accept(sockfd, NULL, NULL);
    if (connfd < 0) {
        perror("accept() failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    waiting = 0;

    char buffer[1512];
    if (recv(connfd, buffer, sizeof(buffer), 0) < 0) {
        perror("recv() failed");
        close(connfd);
        exit(EXIT_FAILURE);
    }

    char buffer_backup[1512];
    strcpy(buffer_backup, buffer);
    char* checkLine = strtok(buffer_backup, "\r");

    char header[128];
    
    char* token;
    token = strtok(checkLine, " ");
    char* function = token;

    token = strtok(NULL, " ");
    char* requestedFileName = token;

    token = strtok(NULL, " ");
    char* version = token;

    token = strtok(NULL, " ");


    char requestedPath[strlen(docRoot) + strlen(requestedFileName) + strlen(defaultFileName) + 1];
    char* bodyContent;

    if (requestedFileName[strlen(requestedFileName) - 1] == '/') {
        sprintf(requestedPath, "%s%s%s", docRoot, requestedFileName, defaultFileName);
    }
    else {
        sprintf(requestedPath, "%s%s", docRoot, requestedFileName);
    }

    if (token != NULL || strcmp(version, "HTTP/1.1") != 0) {
        strcpy(header, "HTTP/1.1 400 Bad Request\r\nConnection: close\r\n");

        if (send(connfd, header, strlen(header), 0) < 0) {
        perror("send() failed");
        close(connfd);
        exit(EXIT_FAILURE);
        }   
    }
    else if (strcmp(function, "GET") != 0) {

        strcpy(header, "HTTP/1.1 501 Not Implemented\r\nConnection: close\r\n");

        if (send(connfd, header, strlen(header), 0) < 0) {
        perror("send() failed");
        close(connfd);
        exit(EXIT_FAILURE);
        }   
    }
    else if (access(requestedPath, F_OK) != 0) {
        strcpy(header, "HTTP/1.1 404 Not Found\r\nConnection: close\r\n");

        if (send(connfd, header, strlen(header), 0) < 0) {
        perror("send() failed");
        close(connfd);
        exit(EXIT_FAILURE);
        }   
    }
    else {
        FILE* filePtr;
        if ((filePtr = fopen(requestedPath,"r")) == NULL){
        perror("fopen() failed");
        close(connfd);
        return EXIT_FAILURE;
        }

        char *line = NULL;
        ssize_t read;
        size_t size = 0;
            
        int count = 0;
        while ((read = getline(&line, &size, filePtr)) != -1) {
            count++;
            if (count == 1) {
                bodyContent = (char *)malloc(read);
                strncpy(bodyContent, line, read);
                bodyContent[read] = '\0';
                continue;
                }
            char *tmp = realloc(bodyContent, strlen(bodyContent) + read + 1);
            if(!tmp) exit(1);
            bodyContent = tmp;

            sprintf(bodyContent + strlen(bodyContent), "%s", line);
            }
        free(line);
        fclose(filePtr);

        
        //time
        char timeString[48];
        time_t t;
        struct tm *tmp;

        t = time(NULL);
        tmp = localtime(&t);
        if (tmp == NULL) {
            perror("localtime");
            free(bodyContent);
            close(connfd);
            exit(EXIT_FAILURE);
           }
        if (strftime(timeString, sizeof(timeString), "%a, %d %b %y %T %Z", tmp) == 0) {
            free(bodyContent);
            close(connfd);
            fprintf(stderr, "strftime returned 0");
            exit(EXIT_FAILURE);
           }

        sprintf(header, "HTTP/1.1 200 OK\r\nDate: %s\r\nContent-Length: %d\r\nConnection: Close\r\n\r\n", timeString, 
                    (int)strlen(bodyContent));


        if (send(connfd, header, strlen(header), 0) < 0) {
        perror("send() failed");
        free(bodyContent);
        close(connfd);
        exit(EXIT_FAILURE);
        }   

        if (send(connfd, bodyContent, strlen(bodyContent), 0) < 0) {
        perror("send() failed");
        free(bodyContent);
        close(connfd);
        exit(EXIT_FAILURE);
        }
        free(bodyContent);
        }

    close(connfd);
    } //outside of while-loop
    close(sockfd);
    return EXIT_SUCCESS;
}