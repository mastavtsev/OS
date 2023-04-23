// 02-parent-child.c
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <time.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/sem.h>

#define SHM_ID 0x1111 // ключ разделяемой памяти
#define SHM_ID_1 2002 // ключ разделяемой памяти
#define PERMS 0666    // права доступа

typedef struct
{
    int student_id;
    int message_type;
    char message[256];
} Message;

/*Имена для семафоров*/
char pathname[] = "program_6_poins-ans.c"; // Профессор (родитель :) ) ожидает ответ от студентов (дети :) )


int semid;
int shmid;   // идентификатор разделяемой памяти
Message *msg_p; // адрес сообщения в разделяемой памяти

int semid1;
int shmid1;   // идентификатор разделяемой памяти
char *msg_p1; // адрес сообщения в разделяемой памяти

struct sembuf mybuf;

/*Обработчик сигналов сигналов*/
void my_handler(int nsig)
{
    if (nsig == 2)
    {
        printf("Receive signal %d, CTRL-C pressed\n", nsig);
    }
    else
    {
        printf("Receive signal %d, CTRL-4 pressed\n", nsig);
    }

    // if (sem_unlink(sem_name_1) == -1)
    // {
    //     perror("sem_unlink: Incorrect unlink of posix semaphore");
    //     exit(-1);
    // };

    // if (sem_close(sem1) == -1)
    // {
    //     perror("sem_close: Incorrect close of posix semaphore");
    //     exit(-1);
    // };

    // if (sem_unlink(sem_name_3) == -1)
    // {
    //     perror("sem_unlink: Incorrect unlink of posix semaphore");
    //     exit(-1);
    // };

    // if (sem_close(sem3) == -1)
    // {
    //     perror("sem_close: Incorrect close of posix semaphore");
    //     exit(-1);
    // };

    shmdt (msg_p);
    if (shmctl (shmid, IPC_RMID, (struct shmid_ds *) 0) < 0) {
        perror("shared memory remove error");
    }

    shmdt (msg_p1);
    if (shmctl (shmid1, IPC_RMID, (struct shmid_ds *) 0) < 0) {
        perror("shared memory remove error");
    }

    if (semctl (semid, 0, IPC_RMID, (struct semid_ds *) 0) < 0)
        perror ("server: semaphore remove error");
    
    exit(0);
}

/* Структура сообщения - передача информации от студента к профессору
    student_id - уникальный номер студента, используется для различия студентов
    message_type - тип сообщения:
                    message_type == 0 - сообщение о номере варианта
                    message_type == 1 - ответ на вариант
    message - само сообщение
*/


/*Метод для отправки ответа от студента*/
void SendMessageStudent(int i, int message_type)
{
    mybuf.sem_op = -1;
    mybuf.sem_num = 2;
    mybuf.sem_flg = 0;
    if (semop(semid, &mybuf, 1) < 0)
    {
        printf("Can\'t wait for condition");
        exit(-1);
    }
    // sem_wait(sem3); // Ожидаем возможности отослать ответ
    
    int shm;

    // Записываем сообщение в разделяемую память
    // Message new_message;
    // new_message.student_id = i;
    // new_message.message_type = message_type;

    

    msg_p->student_id = i;
    msg_p->message_type = message_type;
    // Формируем текст ответа
    if (message_type == 0)
    {
        snprintf(msg_p->message, 256, "Student %d: my variant is %d", i, rand() % 25 + i + 1);
    }
    else
    {
        snprintf(msg_p->message, 256, "Student %d: my answer is ANSWER %d", i, rand() % 25 + i + 1);
    }
    


    mybuf.sem_op = 1;
    mybuf.sem_num = 0;
    mybuf.sem_flg = 0;
    if (semop(semid, &mybuf, 1) < 0)
    {
        printf("Can\'t post for condition");
        exit(-1);
    }
    // sem_post(sem1); // Открываем семофор профессора, теперь он будет получать наше сообщение
}

/*Метод для получение студентом сообщения от профессора*/
void ReceiveMessageStudent(int i)
{
    mybuf.sem_op = -1;
    mybuf.sem_num = 1;
    mybuf.sem_flg = 0;
    if (semop(semid, &mybuf, 1) < 0)
    {
        printf("Can\'t wait for condition");
        exit(-1);
    }
    // sem_wait(sem2);
    

    // осуществить вывод сообщение от профессора
    printf("Student %d received a mark from professor:\n      %s\n", i, (char*)msg_p1);
}

int main(int argc, char **argv)
{
    (void)signal(SIGINT, my_handler);

    if (argc < 2)
    {
        printf("Wrong number of arguments\n");
        exit(-1);
    }

    int n;
    n = atoi(argv[1]);

    char *message;
    int shm;
    int shm_res;
    time_t t;

    char res[100];
    char *addr;
    int res_mem_size = 100; // размер области

    /* Инициализируем генератор случайных чисел */
    srand((unsigned)time(&t));

    pid_t pids[n];
    int i;

    key_t key1;

    if ((key1 = ftok(pathname, 0)) < 0)
    {
        printf("Can\'t generate key\n");
        perror("ket error");
        exit(-1);
    }

    if ((semid = semget(key1, 3, 0666 | IPC_CREAT)) < 0)
    {
        printf("Can\'t get semid\n");
        exit(-1);
    }

    // получение доступа к сегменту разделяемой памяти
    if ((shmid = shmget(SHM_ID, sizeof(Message), 0666 | IPC_CREAT)) < 0)
        perror("writer 1: can not get shared memory segment");

    // получение адреса сегмента
    if ((msg_p = (Message*)shmat(shmid, 0, 0)) == NULL)
    {
        perror("writer: shared memory attach error");
    }

    // получение доступа к сегменту разделяемой памяти
    if ((shmid1 = shmget(SHM_ID_1, 256, 0666 | IPC_CREAT)) < 0)
        perror("writer 2: can not get shared memory segment");

    // получение адреса сегмента
    if ((msg_p1 = (char*)shmat(shmid1, 0, 0)) == NULL)
    {
        perror("writer: shared memory attach error");
    }

    /* Запускаем дочерние процессы (процессы студентов). */
    for (i = 0; i < n; ++i)
    {
        if ((pids[i] = fork()) < 0)
        {
            perror("fork");
            abort();
        }
        else if (pids[i] == 0) // child
        {
            SendMessageStudent(i + 1, 0);
            SendMessageStudent(i + 1, 1);
            ReceiveMessageStudent(i + 1);
            exit(0);
        }
    }

    n = n * 2;

    mybuf.sem_op = 1;
    mybuf.sem_num = 2;
    mybuf.sem_flg = 0;
    if (semop(semid, &mybuf, 1) < 0)
    {
        printf("Can\'t post for condition");
        exit(-1);
    }

    /*Получем 2*n сообщение - по два сообщение (вариант и ответ) от каждого из
            n студентов*/
    while (n > 0)
    {
        mybuf.sem_op = -1;
        mybuf.sem_num = 0;
        mybuf.sem_flg = 0;
        if (semop(semid, &mybuf, 1) < 0)
        {
            printf("Can\'t wait for condition");
            exit(-1);
        }
        // sem_wait(sem1); // Ожидаем на семафоре

        // получить доступ к памяти
        

        if (msg_p == (Message *)-1)
        {
            printf("Error getting pointer to shared memory\n");
            exit(-1);
        }


        if (msg_p->message_type == 0)
        {
            printf("Professor received a variant from Student %d:\n      %s\n", msg_p->student_id, msg_p->message);
        }
        else
        {
            printf("Professor received an answer from Student %d:\n      %s\n", msg_p->student_id, msg_p->message);

            sleep(1); // Профессор решает, какую оценку поставить.
            
            snprintf(res, 100, "Your mark is %d!", rand() % 11);

            // осуществить запись в память
            strcpy (msg_p1, res);

            mybuf.sem_op = 1;
            mybuf.sem_num = 1;
            mybuf.sem_flg = 0;
            if (semop(semid, &mybuf, 1) < 0)
            {
                printf("Can\'t post for condition");
                exit(-1);
            }
            // sem_post(sem2);
        }

        n--;

        mybuf.sem_op = 1;
        mybuf.sem_num = 2;
        mybuf.sem_flg = 0;
        if (semop(semid, &mybuf, 1) < 0)
        {
            printf("Can\'t post for condition");
            exit(-1);
        }
        // sem_post(sem3); // Получен ответ от очередного студента, даём возможность отвечать другим студентам
    }

    pid_t child_pid, wpid;
    int status = 0;

    // Ожидаем завершения всех дочерних процессов
    while ((wpid = wait(&status)) > 0)
    {
    }

    // закрыть открытый объект

    close(shm);

    // if (sem_unlink(sem_name_1) == -1)
    // {
    //     perror("sem_unlink: Incorrect unlink of posix semaphore");
    //     exit(-1);
    // };

    // if (sem_close(sem1) == -1)
    // {
    //     perror("sem_close: Incorrect close of posix semaphore");
    //     exit(-1);
    // };

    // if (sem_unlink(sem_name_3) == -1)
    // {
    //     perror("sem_unlink: Incorrect unlink of posix semaphore");
    //     exit(-1);
    // };

    // if (sem_close(sem3) == -1)
    // {
    //     perror("sem_close: Incorrect close of posix semaphore");
    //     exit(-1);
    // };

    shmdt (msg_p);
    if (shmctl (shmid, IPC_RMID, (struct shmid_ds *) 0) < 0) {
        perror("shared memory remove error");
    }

    shmdt (msg_p1);
    if (shmctl (shmid1, IPC_RMID, (struct shmid_ds *) 0) < 0) {
        perror("shared memory remove error");
    }

    if (semctl (semid, 0, IPC_RMID, (struct semid_ds *) 0) < 0)
        perror ("server: semaphore remove error");

}
