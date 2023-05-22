#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>

#define PORT 4444
#define BUFFER_SIZE 400

int clientSocket, ret;
struct sockaddr_in serverAddr;
long int student_id;

typedef struct
{
    int student_id;
    int message_type;
    char message[256];
} Message;

void DieWithError(char *errorMessage)
{
    perror(errorMessage);
    exit(1);
}

/*Обработчик сигналов*/
void my_handler(int nsig)
{
    close(clientSocket);

    exit(0);
}

int main(int argc, char **argv)
{
    (void)signal(SIGINT, my_handler);
    srandom(time(NULL));

    char buffer_recv[BUFFER_SIZE];
    student_id = random() % 250 + 1;

    unsigned short serverPort; /* Server port */

    if (argc != 2) /* Test for correct number of arguments */
    {
        fprintf(stderr, "Usage:  %s <Server Port>\n", argv[0]);
        exit(1);
    }

    serverPort = atoi(argv[1]); /* First arg:  local port */

    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket < 0)
    {
        DieWithError("[-]Error in connection.\n");
    }
    // printf("[+]Client Socket is created.\n");

    memset(&serverAddr, '\0', sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(serverPort);
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    ret = connect(clientSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr));
    if (ret < 0)
    {
        DieWithError("[-]Error in connection.\n");
        // exit(1);
    }
    printf("Observer started to observe.\n");

    while (1)
    {
        if (recv(clientSocket, buffer_recv, 1024, 0) < 0)
        {
            printf("[-]Error in receiving data.\n");
        }
        else
        {
            printf("%s\n", buffer_recv);
        }
        sleep(2);
    }

    return 0;
}