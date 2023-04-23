// 02-parent-child.c
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <semaphore.h>
#include <time.h>
#include <signal.h>

char sem_name_1[] = "/posix-semaphore-1"; // имя семафора для варианта

char sem_name_3[] = "/posix-semaphore-3"; // имя семафора для варианта

void my_handler(int nsig){
	if (nsig == 2) {
		printf("Receive signal %d, CTRL-C pressed\n", nsig);
	} else {
		printf("Receive signal %d, CTRL-4 pressed\n", nsig);
	}
   exit(0);
}

typedef struct
{
    int student_id;
    int message_type;
    char message[256];
} Message;

void SendMessageStudent(sem_t *sem1, sem_t *sem3, int i, int message_type)
{
    sem_wait(sem3);
    char memn[] = "shared-memory"; // адрес разделяемой памяти
    Message *addr;
    int shm;

    // открыть объект
    if ((shm = shm_open(memn, O_RDWR, 0666)) == -1)
    {
        printf("Opening error\n");
        perror("child shm_open");
        exit(-1);
    }
    // else
    // {
    //     printf("child Object is open: name = %s, id = 0x%x\n", memn, shm);
    // }

    ftruncate(shm, sizeof(Message));

    // Записываем сообщение в разделяемую память
    Message new_message;
    new_message.student_id = i;
    new_message.message_type = message_type;

    if (message_type == 0)
    {
        snprintf(new_message.message, 100, "Student %d: my variant is %d", i, rand() % 25 + i + 1);
    }
    else
    {
        snprintf(new_message.message, 100, "Student %d: my answer is ANSWER %d", i, rand() % 25 + i + 1);
    }

    // strcpy(new_message.message, "Hello from parent process");
    

    // получить доступ к памяти
    addr = mmap(0, sizeof(Message), PROT_WRITE | PROT_READ, MAP_SHARED, shm, 0);
    if (addr == (Message *)-1)
    {
        printf("Error getting pointer to shared memory\n");
        perror("mmap");
        exit(-1);
    }

    // осуществить запись в память
    memcpy(addr, &new_message, sizeof(new_message));
    // printf("Recorded:\n     %s\n", addr);
    //  закрыть открытый объект
    
    sem_post(sem1); // Release the semaphore lock
}

// void WriteAnAnswer(sem_t *sem1, sem_t *sem3, int i)
// {
//     sem_wait(sem3);
//     char memn[] = "shared-memory"; // адрес разделяемой памяти
//     char *addr;
//     int shm;
//     Message *shm_ptr;

//     // открыть объект
//     if ((shm = shm_open(memn, O_RDWR, 0666)) == -1)
//     {
//         printf("Opening error\n");
//         perror("child shm_open");
//         exit(-1);
//     }
//     // else
//     // {
//     //     printf("child Object is open: name = %s, id = 0x%x\n", memn, shm);
//     // }

//     ftruncate(shm, sizeof(Message));

//     // Записываем сообщение в разделяемую память
//     Message new_message;
//     new_message.student_id = i;
//     new_message.message_type = 1;
//     snprintf(new_message.message, 100, "Student %d: my answer is (ANSWER_%d)", i, rand() % 25 + i + 7);
//     // strcpy(new_message.message, "Hello from parent process");
//     *shm_ptr = new_message;

//     // получить доступ к памяти
//     addr = mmap(0, sizeof(Message), PROT_WRITE | PROT_READ, MAP_SHARED, shm, 0);
//     if (addr == (char *)-1)
//     {
//         printf("Error getting pointer to shared memory\n");
//         perror("mmap");
//         exit(-1);
//     }

//     // осуществить запись в память
//     memcpy(addr, shm_ptr, sizeof(shm_ptr));
//     // printf("Recorded:\n     %s\n", addr);
//     //  закрыть открытый объект
//     close(shm);

//     sem_post(sem1); // Release the semaphore lock
// }

int main(int argc, char **argv)
{
    (void)signal(SIGINT, my_handler);
    int n;
    char memn[] = "shared-memory"; // адрес разделяемой памяти
    Message *message;
    int shm;
    sem_t *sem1; // Первый семофор
    sem_t *sem3;
    time_t t;

    /* Intializes random number generator */
    srand((unsigned)time(&t));

    // create, initialize semaphores
    sem1 = sem_open(sem_name_1, O_CREAT, 0666, 0);

    sem3 = sem_open(sem_name_3, O_CREAT, 0666, 1);

    if (argc < 2)
    {
        printf("Wrong number of arguments\n");
        exit(-1);
    }

    n = atoi(argv[1]);

    pid_t pids[n];
    int i;

    // открыть объект
    if ((shm = shm_open(memn, O_CREAT | O_EXCL | O_RDWR, 0666)) == -1)
    {
        printf("Opening error\n");
        perror("parent shm_open");
        return 1;
    }
    // else
    // {
    //     printf("parent Object is open: name = %s, id = 0x%x\n", memn, shm);
    // }

    ftruncate(shm, sizeof(Message));


    /* Start children. */
    for (i = 0; i < n; ++i)
    {
        if ((pids[i] = fork()) < 0)
        {
            perror("fork");
            abort();
        }
        else if (pids[i] == 0) // child
        {
            // printf("%d\n", i);
            // DoWorkInChild();
            SendMessageStudent(sem1, sem3, i + 1, 0);
            SendMessageStudent(sem1, sem3, i + 1, 1);
            exit(0);
        }
    }

    n = n * 2;

    while (n > 0)
    {
        // sem_getvalue(sem1, &value1);
        // printf("The initial value of the semaphore 1 is %d\n", value1);
        sem_wait(sem1); // Lock the semaphore

        // получить доступ к памяти
        message = mmap(0, sizeof(Message), PROT_WRITE | PROT_READ, MAP_SHARED, shm, 0);

        if (message == (Message *)-1)
        {
            printf("Error getting pointer to shared memory\n");
            exit(-1);
        }

        if (message->message_type == 0)
        {
            printf("Professor received a variant from Student %d:\n      %s\n", message->student_id, message->message);
        }
        else
        {
            printf("Professor received an answer from Student %d:\n      %s\n", message->student_id, message->message);
        }
        // осуществить вывод содержимого разделяемой памяти

        // sem_getvalue(sem1, &value1);
        // printf("The value of the semaphore _1_ after the wait is %d\n", value1);

        n--;

        sem_post(sem3);
    }

    // закрыть открытый объект
    close(shm);

    if (sem_unlink(sem_name_1) == -1)
    {
        perror("sem_unlink: Incorrect unlink of posix semaphore");
        exit(-1);
    };

    if (sem_close(sem1) == -1)
    {
        perror("sem_close: Incorrect close of posix semaphore");
        exit(-1);
    };

    if (sem_unlink(sem_name_3) == -1)
    {
        perror("sem_unlink: Incorrect unlink of posix semaphore");
        exit(-1);
    };

    if (sem_close(sem3) == -1)
    {
        perror("sem_close: Incorrect close of posix semaphore");
        exit(-1);
    };

    shm_unlink(memn);
}
