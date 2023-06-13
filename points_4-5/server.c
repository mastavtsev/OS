#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 4444
#define BUFFER_SIZE 300

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

    int sockfd, ret;
    struct sockaddr_in serverAddr, clientAddr;

    Message message;

    char buffer[BUFFER_SIZE];

    srandom(time(NULL));

    unsigned short serverPort; /* Server port */

    if (argc < 2)     /* Test for correct number of arguments */
    {
        fprintf(stderr, "Usage:  %s <Server Port>\n", argv[0]);
        exit(1);
    }

    serverPort = atoi(argv[1]);  /* First arg:  local port */

    sockfd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sockfd < 0)
    {
        DieWithError("[-]Error in connection.\n");
        // exit(1);
    }
    // printf("[+]Server Socket is created.\n");

    memset(&serverAddr, '\0', sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(serverPort);
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    ret = bind(sockfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr));
    if (ret < 0)
    {
        DieWithError("Error in binding.\n");
        // exit(1);
    }
    // printf("[+]Bind to port %d\n", 4444);

    printf("Professor is waiting for students...\n");

    socklen_t addr_size;
    int messages_count = 0;

    while (1)
    {
        addr_size = sizeof(clientAddr);

        // Receive data from the client
        int recv_len = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&clientAddr, &addr_size);
        if (recv_len < 0)
        {
            DieWithError("recvfrom failed");
            break;
        }

        printf("New student has arrived for passing an exam!\n");

        // Deserialize the received data into the original data structure
        memcpy(&message, buffer, sizeof(Message));

        if (message.message_type == 0)
        {
            printf("Professor received a variant from Student %d:\n      %s\n", message.student_id, message.message);
        }
        else
        {
            printf("Professor received an answer from Student %d:\n      %s\n", message.student_id, message.message);

            int time_for_task_checking = random() % 5 + 1;
            sleep(time_for_task_checking);
            printf("Professor rated an answer from Student %d.\n", message.student_id);
            snprintf(buffer, 25, "Your mark is: %ld!", (random() + message.student_id) % 10 + 1);

            // Send the response back to the client
            if (sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr *)&clientAddr, sizeof(clientAddr)) == -1)
            {
                perror("sendto failed");
                break;
            }
        }
    }

    close(sockfd);
}

