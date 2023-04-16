#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>

void parent(int* pd1, int semid) {

	int errnum;
	int errno ;

	for (int i = 0; i < 10; i++) {
		
		write(pd1[1], "Parent to child", 15);
		
		/* Выполним операцию D(semid1,1) */
		struct sembuf mybuf;
		mybuf.sem_op = -1;
		mybuf.sem_flg = 0;
		mybuf.sem_num = 0;
		
		if(semop(semid, &mybuf, 1) < 0){
			printf("parent: Can\'t wait for condition\n");
	
      			perror("Error printed by perror");
			
			exit(-1);
		}
		
		char res[20] = "\0";
		read(pd1[0], res, 15);
		
		printf("%s\n", res);
	
	}
	
}

void child(int* pd1, int semid) {

	for (int i = 0; i < 10; i++) { 
		char res[20] = "\0";
		read(pd1[0], res, 15);
		
		printf("%s\n", res);
		
		write(pd1[1], "Child to parent", 15);
		
		/* Выполним операцию A(semid1,1) */
		struct sembuf mybuf;
		mybuf.sem_op = 1;
		mybuf.sem_flg = 0;
		mybuf.sem_num = 0;
		if(semop(semid, &mybuf, 1) < 0){
			printf("child: Can\'t wait for condition\n");
			exit(-1);
		}
	}
    	
}

int main() {
	int fd1[2];
	pid_t result;
	
	if (pipe(fd1) < 0) {
		printf("Can\'t open pipe\n");
        	exit(-1);
	}
	
	char pathname[] = "sem-pipe.c";
	key_t key = ftok(pathname, 0);
	
	int semid = semget(key, 1, 0666|IPC_CREAT|IPC_EXCL);
	
	if (semid >= 0) {
		semctl(semid, 0, SETVAL, 0);
	}
	
	result = fork();
    	if (result < 0)
    	{
        	printf("Can\'t fork child\n");
        	exit(-1);
    	}
    	else if (result == 0) {
    		child(fd1, semid);
    	}
    	else {
    		parent(fd1, semid);
  		int err = semctl(semid, 0, IPC_RMID, 0);
	    	if(err != 0) {
	      		printf("Incorrect sem destroy\n");
	    	}
    	}
    	
    	
    	close(fd1[1]);
    	close(fd1[0]);
}












