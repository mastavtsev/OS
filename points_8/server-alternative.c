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
#include <errno.h>

#define BUFFER_SIZE 400
#define MAX_OBSERVERS 100

/*Имена для семафоров*/
char sem_name_1[] = "/posix-semaphore-1"; // Профессор (родитель :) ) ожидает ответ от студентов (дети :) )
char sem_name_2[] = "/posix-semaphore-2"; // Студенты ожидают получение оценки от преподователя
char sem_name_3[] = "/posix-semaphore-3"; // Студенты ожидают для отправки номера варианта и ответа профессору

char memn_logs[] = "/shared-memory-logs";           // адрес разделяемой памяти логов
char memn_observers[] = "/shared-memory-observers"; // адрес разделяемой памяти логов

sem_t *sem1;
sem_t *sem2;
sem_t *sem3;

int shm;
int shm_observers;

int clients_main_sock;
int observers_main_sock;

typedef struct
{
    int student_id;
    int message_type;
    char message[256];
} Message;

typedef struct
{
    int count;
    int observers[MAX_OBSERVERS];
} ObserverCont;

/*Обработчик сигналов*/
void my_handler(int nsig)
{
    if (sem_unlink(sem_name_1) == -1)
    {
        perror("sem_unlink: Incorrect unlink of posix semaphore\n");
        exit(-1);
    };

    if (sem_close(sem1) == -1)
    {
        perror("sem_close: Incorrect close of posix semaphore\n");
        exit(-1);
    };

    if (sem_unlink(sem_name_2) == -1)
    {
        perror("sem_unlink: Incorrect unlink of posix semaphore\n");
        exit(-1);
    };

    if (sem_close(sem2) == -1)
    {
        perror("sem_close: Incorrect close of posix semaphore\n");
        exit(-1);
    };

    if (sem_unlink(sem_name_3) == -1)
    {
        perror("sem_unlink: Incorrect unlink of posix semaphore\n");
        exit(-1);
    };

    if (sem_close(sem3) == -1)
    {
        perror("sem_close: Incorrect close of posix semaphore\n");
        exit(-1);
    };

    close(shm);
    shm_unlink(memn_logs);

    close(clients_main_sock);
    close(observers_main_sock);

    ObserverCont *obs_tmp = mmap(0, sizeof(ObserverCont), PROT_WRITE | PROT_READ, MAP_SHARED, shm_observers, 0);

    for (int i = 0; i < obs_tmp->count; i++)
    {
        int observer_socket = obs_tmp->observers[i];
        close(observer_socket);
    }

    close(shm_observers);
    shm_unlink(memn_observers);

    exit(0);
}

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

int send_data(int socket, void *data, size_t size)
{
    // char *buffer = (char *)data;
    size_t total_bytes_sent = 0;
    ssize_t bytes_sent;

    while (total_bytes_sent < size)
    {
        bytes_sent = send(socket, data + total_bytes_sent, size - total_bytes_sent, 0);
        if (bytes_sent < 0)
        {
            perror("send failed");
            return -1;
        }
        total_bytes_sent += bytes_sent;
    }

    return 0;
}

int create_soket(int port, int non_blocking)
{
    int sockfd, ret;

    struct sockaddr_in serverAddr;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        DieWithError("Unable to create sever socket");
    }

    if (non_blocking)
    {
        // Set socket as non-blocking
        int flags = fcntl(sockfd, F_GETFL, 0);
        if (flags == -1)
        {
            perror("fcntl(F_GETFL) failed");
            return 1;
        }
        if (fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) == -1)
        {
            perror("fcntl(F_SETFL) failed");
            return 1;
        }
    }

    memset(&serverAddr, '\0', sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    ret = bind(sockfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr));

    if (ret < 0)
    {
        DieWithError("Error in binding.\n");
        // exit(1);
    }

    return sockfd;
}

int main(int argc, char **argv)
{
    (void)signal(SIGINT, my_handler);

    int client_socket;
    struct sockaddr_in newAddr;

    socklen_t addr_size;

    Message message;
    pid_t childpid;
    pid_t childpid_clients;
    pid_t childpid_observers;

    int res_mem_size = BUFFER_SIZE; // размер области
    int shm_res;

    char buffer[BUFFER_SIZE];

    char *addr;

    srandom(time(NULL));

    unsigned short clientsPort, observerPort; /* Server port */

    if (argc < 3) /* Test for correct number of arguments */
    {
        fprintf(stderr, "Usage:  %s <Server Port> <Observer Port>\n", argv[0]);
        exit(1);
    }

    clientsPort = atoi(argv[1]);  /* First arg:  local port */
    observerPort = atoi(argv[2]); /* Second arg:  local port */

    // создаём и инициализируем семофоры
    sem1 = sem_open(sem_name_1, O_CREAT, 0666, 1);
    sem2 = sem_open(sem_name_2, O_CREAT, 0666, 0);
    sem3 = sem_open(sem_name_3, O_CREAT, 0666, 0);

    // открыть объект
    if ((shm = shm_open(memn_logs, O_CREAT | O_RDWR, 0666)) == -1)
    {
        DieWithError("Opening error\n");
    }

    ftruncate(shm, res_mem_size);

    // открыть объект
    if ((shm_observers = shm_open(memn_observers, O_CREAT | O_RDWR, 0666)) == -1)
    {
        DieWithError("Opening error\n");
    }

    ftruncate(shm_observers, sizeof(ObserverCont));

    ObserverCont observerCont;
    observerCont.count = 0;
    memset(&observerCont.observers, '\0', sizeof(observerCont.observers));

    // получить доступ к памяти
    ObserverCont *addr_obs;
    addr_obs = mmap(0, sizeof(ObserverCont), PROT_WRITE | PROT_READ, MAP_SHARED, shm_observers, 0);

    if (addr_obs == (ObserverCont *)-1)
    {
        printf("Error getting pointer to shared memory\n");
        perror("mmap");
        exit(-1);
    }

    // осуществить запись в память
    memcpy(addr_obs, &observerCont, sizeof(ObserverCont));

    if ((childpid = fork()) > 0)
    {
        // close(observers_main_sock);
        clients_main_sock = create_soket(clientsPort, 0);

        if (listen(clients_main_sock, 25) == 0)
        {
            printf("Professor is waiting for students...\n");
        }
        else
        {
            DieWithError("[-]Error in listening.\n");
        }

        while (1)
        {

            client_socket = accept(clients_main_sock, (struct sockaddr *)&newAddr, &addr_size);

            if (client_socket < 0)
            {
                exit(1);
            }

            // printf("Connection accepted from %s:%d\n", inet_ntoa(newAddr.sin_addr), ntohs(newAddr.sin_port));

            printf("New student has arrived for passing an exam!\n");
            if ((childpid_clients = fork()) == 0)
            {
                close(clients_main_sock);

                int messages_count = 0;
                while (1)
                {
                    sem_wait(sem1);

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

                        char str_buffer[BUFFER_SIZE];

                        if (message.message_type == 0)
                        {
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
                            if (send(client_socket, buffer, 25, 0) != 25)
                            {
                                DieWithError("send() failed");
                            }

                            snprintf(str_buffer, BUFFER_SIZE, "Student %d sent an answer.\nProfessor received an answer from Student %d\nProfessor rated an answer from Student %d with %ld.\n",
                                     message.student_id, message.student_id, message.student_id, mark);
                        }

                        // открыть объект
                        if ((shm = shm_open(memn_logs, O_CREAT | O_RDWR, 0666)) == -1)
                        {
                            printf("Opening error\n");
                            perror("parent shm_open");
                            return 1;
                        }

                        // получить доступ к памяти
                        addr = mmap(0, res_mem_size, PROT_WRITE | PROT_READ, MAP_SHARED, shm, 0);

                        if (addr == (char *)-1)
                        {
                            DieWithError("Error getting pointer to shared memory\n");
                        }

                        memcpy(addr, str_buffer, sizeof(str_buffer));

                        close(shm);
                    }

                    messages_count++;

                    sem_post(sem2);

                    if (messages_count == 2)
                    {
                        close(client_socket);
                        exit(0);
                    }
                }
            }
            else if (childpid < 0)
            {
                DieWithError("Error in fork!\n");
            }
        }

        close(clients_main_sock);
    }
    else
    {
        int childpid_obs;
        observers_main_sock = create_soket(observerPort, 1);

        if (listen(observers_main_sock, 5) != 0)
        {
            DieWithError("[-]Error in listening.\n");
        }

        if ((childpid_obs = fork()) == 0)
        {
            // ожиданание на семафоре для наблюдения
            sem_wait(sem3);
            while (1)
            {
                int sem_val;
                sem_getvalue(sem3, &sem_val);
                printf("Sem 3 value: %d\n", sem_val);
                sem_wait(sem2);

                // получить доступ к памяти
                addr = mmap(0, res_mem_size, PROT_WRITE | PROT_READ, MAP_SHARED, shm, 0);
                if (addr == (char *)-1)
                {
                    printf("Error getting pointer to shared memory\n");
                    exit(-1);
                }

                ObserverCont *obs_tmp = mmap(0, sizeof(ObserverCont), PROT_WRITE | PROT_READ, MAP_SHARED, shm_observers, 0);

                for (int i = 0; i < obs_tmp->count; i++)
                {
                    printf("Number of observers in cycle:\t%d\n", obs_tmp->count);
                    printf("OBSERVER Socket:\t%d\n", obs_tmp->observers[i]);
                    int observer_socket = obs_tmp->observers[i];
                    // Отправить сериализованные данные через сервер
                    if (send(observer_socket, addr, sizeof(addr), 0) != sizeof(addr))
                    {
                        DieWithError("Observer Failed to send data.\n");
                    }
                }
                sleep(5);
                sem_post(sem1);
            }
        }
        else if (childpid_obs > 0)
        {
            int observers_count = 0;

            while (1)
            {
                int observer_socket = accept(observers_main_sock, (struct sockaddr *)&newAddr, &addr_size);

                if (observer_socket == -1)
                {

                    // Handle non-blocking accept
                    if ((errno == EAGAIN) || (errno == EWOULDBLOCK))
                    {
                        continue;
                    }
                    else
                    {
                        DieWithError("observer accept failed\n");
                    }
                }
                else
                {
                    observers_count++;
                    ObserverCont *obs_tmp = mmap(0, sizeof(ObserverCont), PROT_WRITE | PROT_READ, MAP_SHARED, shm_observers, 0);
                    printf("Number of observers:\t%d\n", obs_tmp->count);
                    printf("OBSERVER Socket:\t%d\n", observer_socket);
                    send(observer_socket, "addr", sizeof("addr"), 0);
                    obs_tmp->observers[obs_tmp->count] = observer_socket;
                    obs_tmp->count++;
                    printf("Number of observers:\t%d\n", obs_tmp->count);
                }

                if (observers_count > 0)
                {
                    // поднять семафор для рассылки наблюдателям
                    sem_post(sem3);
                }
            }
        }
    }

    return 0;
}