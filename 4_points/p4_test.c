// 02-parent-child.c
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <semaphore.h>

char sem_name_1[] = "/posix-sem-1"; // имя семафора для варианта

char sem_name_3[] = "/posix-sem-3"; // имя семафора для варианта

void TellTheVariant(sem_t *sem1, sem_t* sem3, int i)
{
    sem_wait(sem3);
    char memn[] = "shared-mem"; // адрес разделяемой памяти
    char buf[100];
    char *addr;
    int shm;
    int mem_size = 100; // размер области

    snprintf(buf, 100, "Variant from a Student %d", i);

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

    ftruncate(shm, mem_size);

    // получить доступ к памяти
    addr = mmap(0, mem_size, PROT_WRITE | PROT_READ, MAP_SHARED, shm, 0);
    if (addr == (char *)-1)
    {
        printf("Error getting pointer to shared memory\n");
        perror("mmap");
        exit(-1);
    }

    // осуществить запись в память
    memcpy(addr, buf, sizeof(buf));
    // printf("Recorded:\n     %s\n", addr);
    //  закрыть открытый объект
    close(shm);

    sem_post(sem1); // Release the semaphore lock
}

void WriteAnAnswer(sem_t *sem1, sem_t* sem3, int i)
{
    sem_wait(sem3);
    char memn[] = "shared-mem"; // адрес разделяемой памяти
    char buf[100];
    char *addr;
    int shm;
    int mem_size = 100; // размер области

    snprintf(buf, 100, "Answer from a student %d", i);

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

    ftruncate(shm, mem_size);

    // получить доступ к памяти
    addr = mmap(0, mem_size, PROT_WRITE | PROT_READ, MAP_SHARED, shm, 0);
    if (addr == (char *)-1)
    {
        printf("Error getting pointer to shared memory\n");
        perror("mmap");
        exit(-1);
    }

    // осуществить запись в память
    memcpy(addr, buf, sizeof(buf));
    // printf("Recorded:\n     %s\n", addr);
    //  закрыть открытый объект
    close(shm);

    sem_post(sem1); // Release the semaphore lock
}

int main(int argc, char **argv)
{
    int n;
    char memn[] = "shared-mem"; // адрес разделяемой памяти
    char *addr;
    int shm;
    int mem_size = 100; // размер области
    sem_t *sem1;        // Первый семофор
    sem_t *sem2;        // Второй семофор
    sem_t *sem3;

    int value1;

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
    if ((shm = shm_open(memn, O_CREAT | O_RDWR, 0666)) == -1)
    {
        printf("Opening error\n");
        perror("parent shm_open");
        return 1;
    }
    // else
    // {
    //     printf("parent Object is open: name = %s, id = 0x%x\n", memn, shm);
    // }

    ftruncate(shm, mem_size);

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
            TellTheVariant(sem1, sem3, i+1);
            int value2;
            // sem_getvalue(sem2, &value2);
            // printf("The initial value of the semaphore 2 is %d\n", value2);
            // sem_getvalue(sem2, &value2);
            // printf("The value of the semaphore _2_ after the wait is %d\n", value2);
            WriteAnAnswer(sem1, sem3, i+1);
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
        addr = mmap(0, mem_size, PROT_WRITE | PROT_READ, MAP_SHARED, shm, 0);

        if (addr == (char *)-1)
        {
            printf("Error getting pointer to shared memory\n");
            exit(-1);
        }

        // осуществить вывод содержимого разделяемой памяти
        printf("Professor received from a student:\n      %s\n", addr);
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

    // if (sem_unlink(sem_name_2) == -1)
    // {
    //     perror("sem_unlink: Incorrect unlink of posix semaphore");
    //     exit(-1);
    // };

    // if (sem_close(sem2) == -1)
    // {
    //     perror("sem_close: Incorrect close of posix semaphore");
    //     exit(-1);
    // };

}
