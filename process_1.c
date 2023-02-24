#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
	int fd, result;
	size_t size;
	char msg[] = "Hello, from process 1!";
	
	char resstring[23];
		
	int msg_size = sizeof(msg);
	
	// Канал для передачи информации от
	// процесса 1 к процессу 2
	char name1[] = "aaa.fifo";
	
	
	// Канал для приёма информации от
	// процесса 2 к процессу 1
	char name2[] = "bbb.fifo";
	

	
	(void)umask(0);
	
	mknod(name1, S_IFIFO | 0666, 0);
	mknod(name2, S_IFIFO | 0666, 0);
	
	while(1) {

		// Блок передачи сообщения 2 процессу
		
		// Открытие файлового дескриптора только на запись
		if ((fd = open(name1, O_WRONLY)) < 0) {
			printf("Can\'t open FIFO for writing\n");
			exit(-1);
		}
		
		// Запись сообщения 
		size = write(fd, msg, msg_size);
		
		if (size != msg_size) {
			printf("Can\'t write all string to FIFO\n");
			exit(-1);
		}
			
		// Процесс спит 5 секунд	
		sleep(5);
		
		if (close(fd) < 0) {
			printf("process 1: Can\'t close FIFO\n"); 
			exit(-1);
		}
		
		
		// Блок получения сообщения от 2 процесса
		
		// Открытие файлового дескриптора только на чтение
		if ((fd = open(name2, O_RDONLY)) < 0) {
			printf("Can\'t open FIFO for reading\n");
			exit(-1);
		}
		
		// Получение сообщения	
		size = read(fd, resstring, msg_size);

		if (size < 0) {
			printf("Can\'t read all string to FIFO\n");
			exit(-1);
		}

		printf("Process 1 exit, resstring: %s\n", resstring);
		
	}
	
}	
