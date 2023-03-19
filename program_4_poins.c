// 02-parent-child.c
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

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
    int fd1[2], fd2[2], result;
    int mes_size;
    int N1, N2;
    size_t size;
    char str_buf[buf_size];
    FILE *input_file;
    FILE *output_file;
    int string_size;

    if (argc < 5)
    {
        printf("Wrong number of arguments\n");
        exit(-1);
    }

    input_file = fopen(argv[1], "r");
    output_file = fopen(argv[2], "w");
    N1 = atoi(argv[3]);
    N2 = atoi(argv[4]);

    if (pipe(fd1) < 0)
    {
        printf("Can\'t open first pipe\n");
        exit(-1);
    }

    result = fork();
    if (result < 0)
    {
        printf("Can\'t fork child\n");
        exit(-1);
    }
    else if (result > 0)
    { /* Grand parent process */

        if (close(fd1[0]) < 0)
        {
            printf("grand parent: Can\'t close reading side of pipe\n");
            exit(-1);
        }

        // Seek the last byte of the file
        fseek(input_file, 0, SEEK_END);
        // Offset from the first to the last byte, or in other words, filesize
        string_size = ftell(input_file);
        // go back to the start of the file
        rewind(input_file);

        if (N1 > string_size | N2 > string_size) {
            printf("grand parent: Parametrs N1 and N2 can\'t be greater than the size of text\n");
            exit(-1);
        }
        


        fread(str_buf, sizeof(char), string_size, input_file);
        str_buf[string_size + 1] = '\0';

        size = write(fd1[1], str_buf, buf_size);

        if (size != buf_size)
        {
            printf("Can\'t write all string to pipe\n");
            exit(-1);
        }

        if (close(fd1[1]) < 0)
        {
            printf("grand parent: Can\'t close writing side of pipe\n");
            exit(-1);
        }

        // printf("Grand Parent exit\n");
    }
    else
    { /* Parent process */

        if (pipe(fd2) < 0)
        {
            printf("Can\'t open second pipe\n");
            exit(-1);
        }

        result = fork();

        if (result < 0)
        {
            printf("Can\'t fork child\n");
            exit(-1);
        }
        else if (result > 0)
        { /* Parent process */

            if (close(fd1[1]) < 0)
            {
                printf("child: Can\'t close writing side of pipe\n");
                exit(-1);
            }

            size = read(fd1[0], str_buf, buf_size);

            if (size < 0)
            {
                printf("Can\'t read string from pipe\n");
                exit(-1);
            }

            revstr(str_buf, N1, N2);

            size = write(fd2[1], str_buf, buf_size);

            if (size != buf_size)
            {
                printf("Can\'t write all string to pipe\n");
                exit(-1);
            }

            if (close(fd2[1]) < 0)
            {
                printf("parent: Can\'t close writing side of pipe\n");
                exit(-1);
            }

            // printf("Grand Parent exit\n");
        }
        else
        { /* Child process */
            if (close(fd2[1]) < 0)
            {
                printf("child: Can\'t close writing side of pipe\n");
                exit(-1);
            }

            size = read(fd2[0], str_buf, buf_size);

            if (size < 0)
            {
                printf("Can\'t read string from pipe\n");
                exit(-1);
            }

            // printf("Child exit, str_buf: %s\n", str_buf);
            fputs(str_buf, output_file);

            if (close(fd2[0]) < 0)
            {
                printf("child: Can\'t close reading side of pipe\n");
                exit(-1);
            }
        }
    }

    fclose(output_file);
    fclose(input_file);
    return 0;
}
