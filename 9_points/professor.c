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
#include <sys/msg.h>
#include <sys/sem.h>

#define SHM_ID 0x1111 // ключ разделяемой памяти
#define SHM_ID_1 2002 // ключ разделяемой памяти
#define PERMS 0666    // права доступа

/* Структура сообщения - передача информации от студента к профессору
    student_id - уникальный номер студента, используется для различия студентов
    message_type - тип сообщения:
                    message_type == 0 - сообщение о номере варианта
                    message_type == 1 - ответ на вариант
    message - само сообщение
*/
typedef struct
{
    int student_id;
    int message_type;
    char message[256];
} Message;

/*Имена для семафоров*/
char pathname[] = "professor.c"; // Профессор (родитель :) ) ожидает ответ от студентов (дети :) )


int semid;
int shmid;   // идентификатор разделяемой памяти
Message *msg_p; // адрес сообщения в разделяемой памяти

int semid1;
int shmid1;   // идентификатор разделяемой памяти
char *msg_p1; // адрес сообщения в разделяемой памяти

struct sembuf mybuf;

int msqid1;
int msqid2;

int msgflg = IPC_CREAT | 0666;

key_t key1 = 10;
key_t key2 = 20;


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

    if (msgctl(msqid1, IPC_RMID, NULL) == -1) {
		fprintf(stderr, "Message queue could not be deleted.\n");
		exit(-1);
	}

    if (msgctl(msqid2, IPC_RMID, NULL) == -1) {
		fprintf(stderr, "Message queue could not be deleted.\n");
		exit(-1);
	}

    if (semctl (semid, 0, IPC_RMID, (struct semid_ds *) 0) < 0)
        perror ("server: semaphore remove error");
    
    exit(0);
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
    int shm_res;
    time_t t;

    char res[100];
    char *addr;
    int res_mem_size = 100; // размер области

    /* Инициализируем генератор случайных чисел */
    srand((unsigned)time(&t));

    pid_t pids[n];
    int i;

    key_t key;

    if ((key = ftok(pathname, 0)) < 0)
    {
        printf("Can\'t generate key\n");
        perror("ket error");
        exit(-1);
    }

    if ((semid = semget(key, 4, 0666 | IPC_CREAT)) < 0)
    {
        printf("Can\'t get semid\n");
        exit(-1);
    }

    if ((msqid1 = msgget(10, msgflg)) < 0) {
        perror("msgget 1");
        exit(-1);
    }

    if ((msqid2 = msgget(20, msgflg)) < 0) {
        perror("msgget 2");
        exit(-1);
    }
    
    mybuf.sem_num = 3;
    mybuf.sem_op  = n;
    mybuf.sem_flg = 0;

    if(semop(semid, &mybuf, 1) < 0){
      printf("Can\'t add for condition\n");
      exit(-1);
    }

    
    int k = n * 2;

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
    while (k > 0)
    {
        //printf("WAIT!\n");
        mybuf.sem_op = -1;
        mybuf.sem_num = 0;
        mybuf.sem_flg = 0;
        if (semop(semid, &mybuf, 1) < 0)
        {
            printf("Can\'t wait for condition");
            exit(-1);
        }

        //printf("WAIT OFF!\n");
        // sem_wait(sem1); // Ожидаем на семафоре

        // получить доступ к памяти
        
        Message message;
        
        if (msgrcv(msqid1, &message, sizeof(Message), 0, 0) < 0) {
            perror("msgget");
            exit(-1);
        }

        //printf("GOT MESSAGE!\n");

        if (message.message_type == 0)
        {
            printf("Professor received a variant from Student %d:\n      %s\n", message.student_id, message.message);
        }
        else
        {
            printf("Professor received an answer from Student %d:\n      %s\n", message.student_id, message.message);

            sleep(1); // Профессор решает, какую оценку поставить.
            
            
            message.message_type = 3;
            
            snprintf(message.message, 100, "Your mark is %d!", rand() % 11);

            if (msgsnd(msqid2, &message, strlen(message.message)+1, IPC_NOWAIT) < 0) {
                perror("msgsnd");
                exit(-1);
            }

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

        k--;

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

    mybuf.sem_op  = 0;
    mybuf.sem_num = 3;
    if(semop(semid, &mybuf, 1) < 0){
      printf("Can\'t wait for condition\n");
      exit(-1);
    }

    if (msgctl(msqid1, IPC_RMID, NULL) == -1) {
		fprintf(stderr, "Message queue could not be deleted.\n");
		exit(-1);
	}

    if (msgctl(msqid2, IPC_RMID, NULL) == -1) {
		fprintf(stderr, "Message queue could not be deleted.\n");
		exit(-1);
	}
    
    if (semctl (semid, 0, IPC_RMID, (struct semid_ds *) 0) < 0)
        perror ("server: semaphore remove error");

}
