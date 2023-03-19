#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const int buf_size = 5001;

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

    input_file = fopen(argv[1], "r");
    output_file = fopen(argv[2], "w");

    // Канал для передачи информации от
    // процесса 1 к процессу 2
    char name1[] = "aaa.fifo";

    // Канал для приёма информации от
    // процесса 2 к процессу 1
    char name2[] = "bbb.fifo";

    (void)umask(0);

    mknod(name1, S_IFIFO | 0666, 0);
    mknod(name2, S_IFIFO | 0666, 0);

    // Seek the last byte of the file
    fseek(input_file, 0, SEEK_END);
    // Offset from the first to the last byte, or in other words, filesize
    string_size = ftell(input_file);
    // go back to the start of the file
    rewind(input_file);

    if (N1 > string_size | N2 > string_size)
    {
        printf("parent: Parametrs N1 and N2 can\'t be greater than the size of the text\n");
        exit(-1);
    }

    fread(str_buf, sizeof(char), string_size, input_file);
    str_buf[string_size + 1] = '\0';

    // Блок передачи сообщения 2 процессу
    // Открытие файлового дескриптора только на запись
    if ((fd = open(name1, O_WRONLY)) < 0)
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

    // Блок получения сообщения от 2 процесса

    // Открытие файлового дескриптора только на чтение
    if ((fd = open(name2, O_RDONLY)) < 0)
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

    fputs(str_buf, output_file);
}