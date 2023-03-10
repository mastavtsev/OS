// shared-memory-client.c
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include "message.h"

void sys_err (char *msg) {
  puts (msg);
  exit (1);
}

message_t *msg_p;     // адрес сообщения в разделяемой памяти

void sigfunc(int sig) {
  if(sig != SIGINT && sig != SIGTERM) {
    return;
  }
  if(sig == SIGINT) {
  	//msg_p->type = SIGTERM;
  	kill(msg_p->server_pid, SIGTERM);
    printf("Client(SIGINT) ---> Server(SIGTERM)\n");
  } else if(sig == SIGTERM) {
    printf("Server(SIGTERM) <--- Client(SIGINT)\n");
  }
  shmdt (msg_p);
  printf("Client: bye!!!\n");
  exit (10);
}

int main () {

  signal(SIGINT,sigfunc);
  signal(SIGTERM,sigfunc);

  srandom(time(NULL));
  int shmid;            // идентификатор разделяемой памяти
  char s[MAX_STRING];   // текст сообщения
  char stop_msg[] = "stop"; // команда конца генерации чисел
  int pid;

  // получение доступа к сегменту разделяемой памяти
  if ((shmid = shmget (SHM_ID, sizeof (message_t), 0)) < 0) {
    sys_err ("client: can not get shared memory segment");
  }

  // получение адреса сегмента
  if ((msg_p = (message_t *) shmat (shmid, 0, 0)) == NULL) {
    sys_err ("client: shared memory attach error");
  }


	pid = getpid();
	printf("pid = %d\n", pid);
	msg_p->client_pid = pid;
  // Организация передачи сообщений
  while (1) {
    fgets(s, MAX_STRING, stdin);
    s[strcspn(s, "\n")] = 0;
    int r = random();
    if(strcmp(s, stop_msg) != 0) {
      // Не прощаемся.
      // Записать сообщение о передаче строки
      msg_p->type = MSG_TYPE_STRING;
      msg_p->random_num = r;
    } else {
      // Записать сообщение о завершении обмена
      msg_p->type = MSG_TYPE_FINISH;
    };
    
    if(msg_p->type == MSG_TYPE_FINISH) {
      break;
    }
  }
  // Окончание цикла передачи сообщений
  shmdt (msg_p);  // отсоединить сегмент разделяемой памяти
  
  
  exit (0);
}

