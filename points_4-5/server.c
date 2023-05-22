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

int receive_data(int socket, void *data, size_t size)
{
    char *buffer = (char *)data;
    size_t total_bytes_received = 0;
    ssize_t bytes_received;

    while (total_bytes_received < size)
    {
        bytes_received = recv(socket, buffer + total_bytes_received, size - total_bytes_received, 0);
        if (bytes_received < 0)
        {
            perror("receive failed");
            return -1;
        }
        else if (bytes_received == 0)
        {
            return 1;
        }
        total_bytes_received += bytes_received;
    }

    return 0;
}

int main(int argc, char **argv)
{

    int sockfd, ret;
    struct sockaddr_in serverAddr;

    int client_socket;
    struct sockaddr_in newAddr;

    socklen_t addr_size;

    Message message;
    pid_t childpid;

    char buffer[BUFFER_SIZE];

    srandom(time(NULL));

    unsigned short serverPort; /* Server port */

    if (argc < 2)     /* Test for correct number of arguments */
    {
        fprintf(stderr, "Usage:  %s <Server Port>\n", argv[0]);
        exit(1);
    }

    serverPort = atoi(argv[1]);  /* First arg:  local port */

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
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

    if (listen(sockfd, 25) == 0)
    {
        printf("Professor is waiting for students...\n");
    }
    else
    {
        printf("[-]Error in binding.\n");
    }


    while (1)
    {
        client_socket = accept(sockfd, (struct sockaddr *)&newAddr, &addr_size);

        if (client_socket < 0)
        {
            exit(1);
        }

        // printf("Connection accepted from %s:%d\n", inet_ntoa(newAddr.sin_addr), ntohs(newAddr.sin_port));

        printf("New student has arrived for passing an exam!\n");

        if ((childpid = fork()) == 0)
        {
            close(sockfd);
            int messages_count = 0;
            while (1)
            {

                // Получить сериализованые данные от клиента
                size_t data_size = sizeof(Message);
                int return_val;
                if ((return_val = receive_data(client_socket, buffer, data_size)) < 0)
                {
                    printf("Failed to receive data.\n");
                }
                else if (return_val == 0)
                {
                    // Deserialize the received data into the original data structure
                    memcpy(&message, buffer, data_size);

                    if (message.message_type == 1)
                    {
                    }

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
                        if (send(client_socket, buffer, 25, 0) != 25)
                        {
                            DieWithError("send() failed");
                        }
                    }

                    // Process the deserialized data
                    // printf("Received message:       %s\n", message.message);
                }
                messages_count++;
                if (messages_count == 2) {
                    exit(0);
                }
            }
        }
        else if (childpid < 0)
        {
            DieWithError("fork() failed");
        }
        
    }

    close(client_socket);

    return 0;
}