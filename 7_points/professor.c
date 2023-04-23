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

    shm_unlink(memn);
    shm_unlink(memn_res);

    exit(0);
}

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

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        printf("Wrong number of arguments\n");
        exit(-1);
    }
    
    int n;
    n = atoi(argv[1]);

    (void)signal(SIGINT, my_handler);
    

    Message *message;
    int shm;
    int shm_res;
    time_t t;

    char res[100];
    char *addr;
    int res_mem_size = 100; // размер области

    /* Инициализируем генератор случайных чисел */
    srand((unsigned)time(&t));

    // создаём и инициализируем семофоры
    sem1 = sem_open(sem_name_1, O_CREAT, 0666, 0);
    sem2 = sem_open(sem_name_2, O_CREAT, 0666, 0);

    /*Начальное значение семафора == 1, т. к. хотя бы один студент
    жолжен начать формировать ответ профессору. Остальные будут ожидать окончание его ответа.*/
    sem3 = sem_open(sem_name_3, O_CREAT, 0666, 1); 

    sem_finish = sem_open(sem_name_finish, O_CREAT, 0666, 0); 

    // открыть объект
    if ((shm = shm_open(memn, O_CREAT | O_RDWR, 0666)) == -1)
    {
        printf("Opening error\n");
        perror("parent shm_open");
        return 1;
    }

    ftruncate(shm, sizeof(Message));

    if ((shm_res = shm_open(memn_res, O_CREAT | O_RDWR, 0666)) == -1)
    {
        printf("Opening error\n");
        perror("parent shm_open");
        return 1;
    }

    ftruncate(shm_res, res_mem_size);

    int students_finised = 0;
    
    int k = 2 * n;
    int i = 1;
    /*Получем 2*n сообщений - по два сообщение (вариант и ответ) от каждого из
            n студентов*/
    while (k > 0)
    {
        sem_wait(sem1); // Ожидаем на семафоре

        // получить доступ к памяти
        message = mmap(0, sizeof(Message), PROT_WRITE | PROT_READ, MAP_SHARED, shm, 0);

        if (message == (Message *)-1)
        {
            printf("Error getting pointer to shared memory\n");
            exit(-1);
        }

        if (message->message_type == 0)
        {
            printf("Professor received a variant from Student %d:\n      %s\n", i, message->message);
        }
        else
        {
            printf("Professor received an answer from Student %d:\n      %s\n", i, message->message);

            // получить доступ к памяти
            addr = mmap(0, res_mem_size, PROT_WRITE | PROT_READ, MAP_SHARED, shm_res, 0);
            if (addr == (char *)-1)
            {
                printf("Error getting pointer to shared memory\n");
                perror("mmap");
                return 1;
            }
	    sleep(1); // Профессор решает, какую оценку поставить.
            snprintf(res, 100, "Your mark is %d!", rand() % 11);

            // осуществить запись в память
            memcpy(addr, res, sizeof(res));
            sem_post(sem2);
        }

        k--;

        sem_post(sem3); // Получен ответ от очередного студента, даём возможность отвечать другим студентам
    }

    pid_t child_pid, wpid;
    int status = 0;

    // закрыть открытый объект
    close(shm);


    while(students_finised < n){
        sem_getvalue(sem_finish, &students_finised);
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

    shm_unlink(memn);
    shm_unlink(memn_res);
}
