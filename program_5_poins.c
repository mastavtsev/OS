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
    int fd1, fd2, result;
    int mes_size;
    int N1, N2;
    size_t size;
    char str_buf[buf_size];
    FILE *input_file;
    FILE *output_file;
    int string_size;

    // Канал для передачи информации от
    // процесса grand parent к процессу parent
    char name1[] = "aaa.fifo";

    // Канал для приёма информации от
    // процесса parent к процессу child
    char name2[] = "bbb.fifo";

    if (argc < 5)
    {
        printf("Wrong number of arguments\n");
        exit(-1);
    }

    // устанавливаем права на чтение из пайпа для дочернего процесса
    (void)umask(0);

    mknod(name1, S_IFIFO | 0666, 0);
    mknod(name2, S_IFIFO | 0666, 0);

    input_file = fopen(argv[1], "r");
    output_file = fopen(argv[2], "w");
    N1 = atoi(argv[3]);
    N2 = atoi(argv[4]);

    result = fork();
    if (result < 0)
    {
        printf("Can\'t fork child\n");
        exit(-1);
    }
    else if (result > 0)
    { /* Grand parent process */

        // Seek the last byte of the file
        fseek(input_file, 0, SEEK_END);
        // Offset from the first to the last byte, or in other words, filesize
        string_size = ftell(input_file);
        // go back to the start of the file
        rewind(input_file);

        if (N2 > string_size)
        {
            printf("grand parent: Parametrs N1 and N2 can\'t be greater than the size of text\n");
            exit(-1);
        }

        fread(str_buf, sizeof(char), string_size, input_file);
        str_buf[string_size + 1] = '\0';

        // Открытие файлового дескриптора только на запись
        if ((fd1 = open(name1, O_WRONLY)) < 0)
        {
            printf("Can\'t open FIFO for writing\n");
            exit(-1);
        }

        size = write(fd1, str_buf, buf_size);

        if (size != buf_size)
        {
            printf("Can\'t write all string to pipe\n");
            exit(-1);
        }

        if (close(fd1) < 0)
        {
            printf("grand parent: Can\'t close FIFO\n");
            exit(-1);
        }

        // printf("Grand Parent exit\n");
    }
    else
    { /* Parent process */

        result = fork();

        if (result < 0)
        {
            printf("Can\'t fork child\n");
            exit(-1);
        }
        else if (result > 0)
        { /* Parent process */

            // Открытие файлового дескриптора только на чтение
            if ((fd1 = open(name1, O_RDONLY)) < 0)
            {
                printf("Can\'t open FIFO for reading\n");
                exit(-1);
            }

            size = read(fd1, str_buf, buf_size);

            if (close(fd1) < 0)
            {
                printf("parent: Can\'t close FIFO\n");
                exit(-1);
            }

            if (size < 0)
            {
                printf("Can\'t read string from pipe\n");
                exit(-1);
            }

            revstr(str_buf, N1, N2);

            // Открытие файлового дескриптора только на запись
            if ((fd2 = open(name2, O_WRONLY)) < 0)
            {
                printf("Can\'t open FIFO for writing\n");
                exit(-1);
            }

            size = write(fd2, str_buf, buf_size);

            if (close(fd2) < 0)
            {
                printf("parent: Can\'t close FIFO\n");
                exit(-1);
            }

            if (size != buf_size)
            {
                printf("Can\'t write all string to pipe\n");
                exit(-1);
            }
        }
        else
        { /* Child process */

            // Открытие файлового дескриптора только на чтение
            if ((fd2 = open(name2, O_RDONLY)) < 0)
            {
                printf("Can\'t open FIFO for reading\n");
                exit(-1);
            }

            size = read(fd2, str_buf, buf_size);

            if (size < 0)
            {
                printf("Can\'t read string from pipe\n");
                exit(-1);
            }

            if (close(fd2) < 0)
            {
                printf("child: Can\'t close FIFO\n");
                exit(-1);
            }

            // printf("Child exit, str_buf: %s\n", str_buf);
            fputs(str_buf, output_file);
        }
    }

    fclose(output_file);
    fclose(input_file);
    return 0;
}
