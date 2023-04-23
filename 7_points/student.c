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

    if (sem_close(sem_finish) == -1)
    {
        perror("sem_close: Incorrect close of posix semaphore");
        exit(-1);
    };


    if (sem_close(sem2) == -1)
    {
        perror("sem_close: Incorrect close of posix semaphore");
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
    int shm_res;
    int res_mem_size = 100; // размер области

    if ((shm_res = shm_open(memn_res, O_RDWR, 0666)) == -1)
    {
        printf("Opening error\n");
        perror("student receive shm_open");
        exit(-1);
    }

    ftruncate(shm_res, res_mem_size);

    // получить доступ к памяти
    addr = mmap(0, res_mem_size, PROT_WRITE | PROT_READ, MAP_SHARED, shm_res, 0);
    if (addr == (char *)-1)
    {
        printf("Error getting pointer to shared memory\n");
        exit(-1);
    }

    // осуществить вывод сообщение от профессора
    printf("Student %d received a mark from professor:\n      %s\n", i, addr);

    //  закрыть открытый объект
    close(shm_res);
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

    sem_finish = sem_open(sem_name_finish, O_CREAT, 0666, 0); 

    ftruncate(shm, sizeof(Message));

    
    SendMessageStudent(1, 0);
    SendMessageStudent(1, 1);
    ReceiveMessageStudent(1);
        
    // закрыть открытый объект
    close(shm);
    close(shm_res);

    sem_post(sem_finish);
    

    if (sem_close(sem1) == -1)
    {
        perror("sem_close: Incorrect close of posix semaphore");
        exit(-1);
    };

    if (sem_close(sem2) == -1)
    {
        perror("sem_close: Incorrect close of posix semaphore");
        exit(-1);
    };

    if (sem_close(sem3) == -1)
    {
        perror("sem_close: Incorrect close of posix semaphore");
        exit(-1);
    };

    if (sem_close(sem_finish) == -1)
    {
        perror("sem_close: Incorrect close of posix semaphore");
        exit(-1);
    };
}
