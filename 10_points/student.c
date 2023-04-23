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

/*Обработчик сигнало*/
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

    exit(0);
}




/*Метод для отправки ответа от студента*/
void SendMessageStudent(int i, int message_type)
{
    
    sem_wait(sem3); // Ожидаем возможности отослать ответ
    //printf("MAKING MESSAGE %d!\n", message_type);

    // Записываем сообщение в разделяемую память
    // Message new_message;
    // new_message.student_id = i;
    // new_message.message_type = message_type;
    Message message;
    
    message.student_id =  1;
    message.message_type = message_type;
    // Формируем текст ответа
    if (message_type == 0)
    {
        snprintf(message.message, 256, "Student %d: my variant is %d", message.student_id, rand() % 25 + i + 1);
    }
    else
    {
        snprintf(message.message, 256, "Student %d: my answer is ANSWER %d", message.student_id, rand() % 25 + i + 1);
    }

    //printf("Made MESSAGE %d!\n", message_type);
    
    if (msgsnd(msqid1, &message, sizeof(Message), IPC_NOWAIT) < 0) {
        perror("msgsnd 1");
        exit(-1);
    }

    //printf("SEND MESSAGE %d!\n", message_type);

    sem_post(sem1); // Открываем семофор профессора, теперь он будет получать наше сообщение
}

/*Метод для получение студентом сообщения от профессора*/
void ReceiveMessageStudent(int i)
{

    sem_wait(sem2);
    
    Message message;
        
    if (msgrcv(msqid2, &message, 256, 0, 0) < 0) {
        perror("msgget");
        exit(-1);
    }

    // осуществить вывод сообщение от профессора
    printf("Student %d received a mark from professor:\n      %s\n", i, message.message);
}

int main(int argc, char **argv)
{
    (void)signal(SIGINT, my_handler);

    char *message;
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

    if ((msqid1 = msgget(key1, msgflg)) < 0) {
        perror("msgget 1");
        exit(-1);
    }

    if ((msqid2 = msgget(key2, msgflg)) < 0) {
        perror("msgget 2");
        exit(-1);
    }

    SendMessageStudent(1, 0);
    SendMessageStudent(1, 1);
    ReceiveMessageStudent(1);
    
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
