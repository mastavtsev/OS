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
#include <sys/msg.h>

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


/*Имена для семафоров*/
char sem_name_1[] = "/posix-semaphore-1"; // Профессор (родитель :) ) ожидает ответ от студентов (дети :) )
char sem_name_2[] = "/posix-semaphore-2"; // Студенты ожидают получение оценки от преподователя
char sem_name_3[] = "/posix-semaphore-3"; // Студенты ожидают для отправки номера варианта и ответа профессору
char sem_name_finish[] = "/posix-semaphore-finish"; // Студенты ожидают для отправки номера варианта и ответа профессору

char memn[] = "shared-memory"; // адрес разделяемой памяти для ответа студентов
char memn_res[] = "result-memory"; //  адрес разделяемой памяти для ответа профессора

sem_t *sem1;
sem_t *sem2;
sem_t *sem3;

sem_t *sem_finish;

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

    
    if (sem_close(sem1) == -1)
    {
        perror("sem_close: Incorrect close of posix semaphore");
        exit(-1);
    };

    if (sem_unlink(sem_name_1) == -1)
    {
        perror("sem_unlink: Incorrect unlink of posix semaphore");
        exit(-1);
    };

    if (sem_close(sem_finish) == -1)
    {
        perror("sem_close: Incorrect close of posix semaphore");
        exit(-1);
    };

    if (sem_unlink(sem_name_finish) == -1)
    {
        perror("sem_unlink: Incorrect unlink of posix semaphore");
        exit(-1);
    };

    if (sem_close(sem2) == -1)
    {
        perror("sem_close: Incorrect close of posix semaphore");
        exit(-1);
    };

    if (sem_unlink(sem_name_2) == -1)
    {
        perror("sem_unlink: Incorrect unlink of posix semaphore");
        exit(-1);
    };

    if (sem_close(sem3) == -1)
    {
        perror("sem_close: Incorrect close of posix semaphore");
        exit(-1);
    };

    if (sem_unlink(sem_name_3) == -1)
    {
        perror("sem_unlink: Incorrect unlink of posix semaphore");
        exit(-1);
    };

    if (msgctl(msqid1, IPC_RMID, NULL) == -1) {
		fprintf(stderr, "Message queue could not be deleted.\n");
		exit(-1);
	}

    if (msgctl(msqid2, IPC_RMID, NULL) == -1) {
		fprintf(stderr, "Message queue could not be deleted.\n");
		exit(-1);
	}

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

    /* Инициализируем генератор случайных чисел */
    srand((unsigned)time(&t));

    // создаём и инициализируем семофоры
    sem1 = sem_open(sem_name_1, O_CREAT, 0666, 0);
    sem2 = sem_open(sem_name_2, O_CREAT, 0666, 0);

    /*Начальное значение семафора == 1, т. к. хотя бы один студент
    жолжен начать формировать ответ профессору. Остальные будут ожидать окончание его ответа.*/
    sem3 = sem_open(sem_name_3, O_CREAT, 0666, 1); 

    sem_finish = sem_open(sem_name_finish, O_CREAT, 0666, 0);

    if ((msqid1 = msgget(10, msgflg)) < 0) {
        perror("msgget 1");
        exit(-1);
    }

    if ((msqid2 = msgget(20, msgflg)) < 0) {
        perror("msgget 2");
        exit(-1);
    }
    
    int k = n * 2;

    /*Получем 2*n сообщение - по два сообщение (вариант и ответ) от каждого из
            n студентов*/
    while (k > 0)
    {
        sem_wait(sem1); // Ожидаем на семафоре

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

            sem_post(sem2);
            
        }

        k--;

        sem_post(sem3); // Получен ответ от очередного студента, даём возможность отвечать другим студентам
    }

    int students_finised = 0;
    while(students_finised < n){
        sem_getvalue(sem_finish, &students_finised);
    }

    if (msgctl(msqid1, IPC_RMID, NULL) == -1) {
		fprintf(stderr, "Message queue could not be deleted.\n");
		exit(-1);
	}

    if (msgctl(msqid2, IPC_RMID, NULL) == -1) {
		fprintf(stderr, "Message queue could not be deleted.\n");
		exit(-1);
	}
    
    if (sem_close(sem1) == -1)
    {
        perror("sem_close: Incorrect close of posix semaphore");
        exit(-1);
    };

    if (sem_unlink(sem_name_1) == -1)
    {
        perror("sem_unlink: Incorrect unlink of posix semaphore");
        exit(-1);
    };

    if (sem_close(sem_finish) == -1)
    {
        perror("sem_close: Incorrect close of posix semaphore");
        exit(-1);
    };

    if (sem_unlink(sem_name_finish) == -1)
    {
        perror("sem_unlink: Incorrect unlink of posix semaphore");
        exit(-1);
    };

    if (sem_close(sem2) == -1)
    {
        perror("sem_close: Incorrect close of posix semaphore");
        exit(-1);
    };

    if (sem_unlink(sem_name_2) == -1)
    {
        perror("sem_unlink: Incorrect unlink of posix semaphore");
        exit(-1);
    };

    if (sem_close(sem3) == -1)
    {
        perror("sem_close: Incorrect close of posix semaphore");
        exit(-1);
    };

    if (sem_unlink(sem_name_3) == -1)
    {
        perror("sem_unlink: Incorrect unlink of posix semaphore");
        exit(-1);
    };

}
