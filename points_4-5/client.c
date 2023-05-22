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
#define BUFFER_SIZE 300

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

int send_data(int socket, void *data, size_t size)
{
    char *buffer = (char *)data;
    size_t total_bytes_sent = 0;
    ssize_t bytes_sent;

    while (total_bytes_sent < size)
    {
        bytes_sent = send(socket, buffer + total_bytes_sent, size - total_bytes_sent, 0);
        if (bytes_sent < 0)
        {
            perror("send failed");
            return -1;
        }
        total_bytes_sent += bytes_sent;
    }

    return 0;
}

void SendMessageStudent(int message_type)
{
    char buffer[BUFFER_SIZE];


    // Создаём сообщение
    Message new_message;
    new_message.message_type = message_type;
    new_message.student_id = student_id;

    if (message_type == 0)
    {
        snprintf(new_message.message, 100, "Student %ld: my variant is %ld", student_id, random() % 25 + 1);
    }
    else
    {
        snprintf(new_message.message, 100, "Student %ld: my answer is ANSWER %ld", student_id, random() % 25 + 1);
    }

    // strcpy(new_message.message, "Hello from parent process");

    size_t data_size = sizeof(Message);
    memcpy(buffer, &new_message, data_size);

    // Отправить сериализованные данные через сервер
    if (send_data(clientSocket, buffer, data_size) < 0)
    {
        printf("Failed to send data.\n");
    }

    // send(clientSocket, &new_message, sizeof(new_message), 0);
}

int main(int argc, char **argv)
{
    srandom(time(NULL));
    
    char buffer_recv[BUFFER_SIZE];
    student_id = random() % 250 + 1;

    unsigned short serverPort; /* Server port */

    if (argc != 2)     /* Test for correct number of arguments */
    {
        fprintf(stderr, "Usage:  %s <Server Port>\n", argv[0]);
        exit(1);
    }

    serverPort = atoi(argv[1]);  /* First arg:  local port */

    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket < 0)
    {
        DieWithError("[-]Error in connection.\n");
    }
    //printf("[+]Client Socket is created.\n");

    memset(&serverAddr, '\0', sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(serverPort);
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    ret = connect(clientSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr));
    if (ret < 0)
    {
        DieWithError("[-]Error in connection.\n");
        //exit(1);
    }
    printf("Student %ld entered class.\n", student_id);

    int time_for_task_solving = rand() % 5 + 1;

    SendMessageStudent(0);
    sleep(time_for_task_solving);
    SendMessageStudent(1);

    if (recv(clientSocket, buffer_recv, 1024, 0) < 0)
    {
        printf("[-]Error in receiving data.\n");
    }
    else
    {
        printf("Mark from professor: \t%s\n", buffer_recv);
    }

    return 0;
}