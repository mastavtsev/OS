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

char memn[] = "shared-memory"; // адрес разделяемой памяти для ответа студентов
char memn_res[] = "/result-memory"; //  адрес разделяемой памяти для ответа профессора

sem_t *sem1;
sem_t *sem2;
sem_t *sem3;


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

/*Метод для отправки ответа от студента*/
void SendMessageStudent(int i, int message_type)
{
    sem_wait(sem3);                // Ожидаем возможности отослать ответ
    Message *addr;
    int shm;

    // открыть объект
    if ((shm = shm_open(memn, O_RDWR, 0666)) == -1)
    {
        printf("Opening error\n");
        perror("child shm_open");
        exit(-1);
    }

    ftruncate(shm, sizeof(Message));

    // Записываем сообщение в разделяемую память
    Message new_message;
    new_message.student_id = i;
    new_message.message_type = message_type;

    // Формируем текст ответа
    if (message_type == 0)
    {
        snprintf(new_message.message, 100, "Student %d: my variant is %d", i, rand() % 25 + i + 1);
    }
    else
    {
        snprintf(new_message.message, 100, "Student %d: my answer is ANSWER %d", i, rand() % 25 + i + 1);
    }

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

    //  закрыть открытый объект
    close(shm);
    sem_post(sem1); // Открываем семофор профессора, теперь он будет получать наше сообщение
}

/*Метод для получение студентом сообщения от профессора*/
void ReceiveMessageStudent(int i)
{
    sem_wait(sem2);
    char *addr;
    int shm;
    int mem_size = 100; // размер области

    // открыть объект
    if ((shm = shm_open(memn_res, O_RDWR, 0666)) == -1)
    {
        printf("Opening error\n");
        perror("shm_open");
        exit(-1);
    }

    // получить доступ к памяти
    addr = mmap(0, mem_size, PROT_WRITE | PROT_READ, MAP_SHARED, shm, 0);
    if (addr == (char *)-1)
    {
        printf("Error getting pointer to shared memory\n");
        exit(-1);
    }

    // осуществить вывод сообщение от профессора
    printf("Student %d received a mark from professor:\n      %s\n", i, addr);

    //  закрыть открытый объект
    close(shm);
}

int main(int argc, char **argv)
{
    (void)signal(SIGINT, my_handler);
    int n;

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

    ftruncate(shm, sizeof(Message));

    if ((shm_res = shm_open(memn_res, O_CREAT | O_EXCL | O_RDWR, 0666)) == -1)
    {
        printf("Opening error\n");
        perror("parent shm_open");
        return 1;
    }

    ftruncate(shm_res, res_mem_size);

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

    /*Получем 2*n сообщение - по два сообщение (вариант и ответ) от каждого из
            n студентов*/
    while (n > 0)
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
            printf("Professor received a variant from Student %d:\n      %s\n", message->student_id, message->message);
        }
        else
        {
            printf("Professor received an answer from Student %d:\n      %s\n", message->student_id, message->message);

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

        n--;

        sem_post(sem3); // Получен ответ от очередного студента, даём возможность отвечать другим студентам
    }

    pid_t child_pid, wpid;
    int status = 0;

    // Ожидаем завершения всех дочерних процессов
    while ((wpid = wait(&status)) > 0)
    {
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
    shm_unlink(memn_res);
}
