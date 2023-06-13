#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <semaphore.h>
#include <signal.h>

#define BUFFER_SIZE 400

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

int send_data(int socket, void *data, size_t size)
{
    char *buffer = (char *)data;
    ssize_t bytes_sent = send(socket, buffer, size, 0);
    if (bytes_sent < 0)
    {
        perror("send failed");
        return -1;
    }
    return 0;
}

int create_socket(int port)
{
    int sockfd;
    struct sockaddr_in serverAddr;

    if ((sockfd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
    {
        DieWithError("Unable to create server socket");
    }

    memset(&serverAddr, '\0', sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sockfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
    {
        DieWithError("Error in binding.\n");
    }

    return sockfd;
}

int main(int argc, char **argv)
{
    int client_socket;
    struct sockaddr_in newAddr, observerAddr;
    socklen_t addr_size, obser_addr_size;

    Message message;
    pid_t childpid;
    pid_t childpid_clients;

    int res_mem_size = BUFFER_SIZE;
    int shm_res;

    char buffer[BUFFER_SIZE];

    char *addr;

    srandom(time(NULL));

    unsigned short clientsPort, observerPort;

    if (argc < 3)
    {
        fprintf(stderr, "Usage:  %s <Server Port> <Observer Port>\n", argv[0]);
        exit(1);
    }

    clientsPort = atoi(argv[1]);
    observerPort = atoi(argv[2]);

    int clients_main_sock = create_socket(clientsPort);

    int observers_main_sock;

    if ((observers_main_sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
    {
        DieWithError("Unable to create server to observer socket");
    }

    /* Set socket to allow broadcast */
    int broadcastPermission = 1;
    if (setsockopt(observers_main_sock, SOL_SOCKET, SO_BROADCAST, (void *)&broadcastPermission,
                   sizeof(broadcastPermission)) < 0)
        DieWithError("setsockopt() failed");

    /* Construct local address structure */
    memset(&observerAddr, 0, sizeof(observerAddr)); /* Zero out structure */
    observerAddr.sin_family = AF_INET;              /* Internet address family */
    observerAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    observerAddr.sin_port = htons(observerPort); /* Broadcast port */

    printf("Professor is waiting for students...\n");

    while (1)
    {
        addr_size = sizeof(newAddr);
        obser_addr_size = sizeof(observerAddr);

        ssize_t bytes_received = recvfrom(clients_main_sock, buffer, sizeof(buffer), 0, (struct sockaddr *)&newAddr, &addr_size);
        if (bytes_received < 0)
        {
            perror("receive failed");
            exit(1);
        }

        memcpy(&message, buffer, sizeof(Message));

        char str_buffer[BUFFER_SIZE];

        if (message.message_type == 0)
        {
            printf("New student has arrived for passing an exam!\n");

            snprintf(str_buffer, BUFFER_SIZE, "Student %d sent a variant number.\nProfessor received variant from Student %d\n",
                     message.student_id, message.student_id);

            printf("Professor received a variant from Student %d:\n      %s\n", message.student_id, message.message);
        }
        else
        {
            printf("Professor received an answer from Student %d:\n      %s\n", message.student_id, message.message);

            int time_for_task_checking = random() % 5 + 1;
            sleep(time_for_task_checking);
            printf("Professor rated an answer from Student %d.\n", message.student_id);

            long mark = (random() + message.student_id) % 10 + 1;

            snprintf(buffer, 25, "Your mark is: %ld!", mark);

            if (sendto(clients_main_sock, buffer, 25, 0, (struct sockaddr *)&newAddr, addr_size) != 25)
            {
                DieWithError("sendto() failed");
            }

            snprintf(str_buffer, BUFFER_SIZE, "Student %d sent an answer.\nProfessor received an answer from Student %d\nProfessor rated an answer from Student %d with %ld.\n",
                     message.student_id, message.student_id, message.student_id, mark);
        }

        if (sendto(observers_main_sock, str_buffer, BUFFER_SIZE, 0, (struct sockaddr *)&observerAddr, obser_addr_size) != BUFFER_SIZE)
        {
            DieWithError("sendto() for observer failed");
        }
    }

    close(clients_main_sock);
    close(observers_main_sock);

    return 0;
}
