#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const int buf_size = 5001;

// function definition of the revstr()
void revstr(char *str1, int N1, int N2)
{

    int i, len, temp;
    len = N2 - N1;

    for (i = N1; i <= N2 / 2; i++)
    {
        temp = str1[i];
        str1[i] = str1[N2 - i];
        str1[N2 - i] = temp;
    }
}

int main(int argc, char **argv)
{
    int fd, result;
    int mes_size;
    int N1, N2;
    size_t size;
    char str_buf[buf_size];
    FILE *input_file;
    FILE *output_file;
    int string_size;

    if (argc < 3)
    {
        printf("Wrong number of arguments\n");
        exit(-1);
    }
    N1 = atoi(argv[1]);
    N2 = atoi(argv[2]);

    // Канал для передачи информации от
    // процесса 1 к процессу 2
    char name1[] = "aaa.fifo";

    // Канал для приёма информации от
    // процесса 2 к процессу 1
    char name2[] = "bbb.fifo";

    (void)umask(0);

    mknod(name1, S_IFIFO | 0666, 0);
    mknod(name2, S_IFIFO | 0666, 0);

    // Блок получения сообщения от 1 процесса

    // Открытие файлового дескриптора только на чтение
    if ((fd = open(name1, O_RDONLY)) < 0)
    {
        printf("Can\'t open FIFO for reading\n");
        exit(-1);
    }

    // Получение сообщения
    size = read(fd, str_buf, buf_size);

    if (size < 0)
    {
        printf("Can\'t read all string to FIFO\n");
        exit(-1);
    }

    revstr(str_buf, N1, N2);

    // Блок передачи сообщения 1 процессу
    // Открытие файлового дескриптора только на запись
    if ((fd = open(name2, O_WRONLY)) < 0)
    {
        printf("Can\'t open FIFO for writing\n");
        exit(-1);
    }

    // Запись сообщения
    size = write(fd, str_buf, buf_size);

    if (size != buf_size)
    {
        printf("Can\'t write all string to FIFO\n");
        exit(-1);
    }

    if (close(fd) < 0)
    {
        printf("process 1: Can\'t close FIFO\n");
        exit(-1);
    }
}