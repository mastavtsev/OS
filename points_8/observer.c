#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

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

int main(int argc, char **argv)
{
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

    clientSocket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (clientSocket < 0)
    {
        DieWithError("[-]Error in connection.\n");
    }
    // printf("[+]Client Socket is created.\n");

    memset(&serverAddr, '\0', sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(serverPort);
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    const int trueValue = 1;
    setsockopt(clientSocket, SOL_SOCKET, SO_REUSEADDR, &trueValue, sizeof(trueValue));

    /* Bind to the broadcast port */
    if (bind(clientSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
    {
        DieWithError("bind() failed");
    }

    printf("Observer started to observe.\n");

    socklen_t addr_size = sizeof(serverAddr);

    while (1)
    {
        if (recvfrom(clientSocket, buffer_recv, BUFFER_SIZE, 0, NULL, 0) < 0)
        {
            printf("[-]Error in receiving data.\n");
        }
        else
        {
            printf("%s\n", buffer_recv);
        }
        sleep(2);
    }

    close(clientSocket);

    return 0;
}