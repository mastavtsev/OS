// shared-memory-server.c
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include "message.h"

void sys_err (char *msg) {
  puts (msg);
  exit (1);
}

message_t *msg_p;     // адрес сообщения в разделяемой памяти
int shmid;             // идентификатор разделяемой памяти

void sigfunc(int sig) {
  if(sig != SIGINT && sig != SIGTERM) {
    return;
  }
  if(sig == SIGINT) {
  	//msg_p->type = SIGTERM;
    kill(msg_p->client_pid, SIGTERM);
    printf("Server(SIGINT) ---> Client(SIGTERM)\n");
  } else if(sig == SIGTERM) {
    printf("Server(SIGTERM) <--- Client(SIGINT)\n");
  }
  
  // удаление сегмента разделяемой памяти
  shmdt (msg_p);
  if (shmctl (shmid, IPC_RMID, (struct shmid_ds *) 0) < 0) {
    sys_err ("server: shared memory remove error");
  }
  printf("Server: bye!!!\n");
  exit (10);
}



int main () {

	signal(SIGINT,sigfunc);
  signal(SIGTERM,sigfunc);

  //message_t *msg_p;      // адрес сообщения в разделяемой памяти
  char s[MAX_STRING];
  int pid;

  // создание сегмента разделяемой памяти
  if ((shmid = shmget (SHM_ID, sizeof (message_t), PERMS | IPC_CREAT)) < 0) {
    sys_err ("server: can not create shared memory segment");
  }
  printf("Shared memory %d created\n", SHM_ID);

  // подключение сегмента к адресному пространству процесса
  if ((msg_p = (message_t *) shmat (shmid, 0, 0)) == NULL) {
    sys_err ("server: shared memory attach error");
  }
  printf("Shared memory pointer = %p\n", msg_p);

  msg_p->type = MSG_TYPE_EMPTY;
  
  pid = getpid();
  msg_p->server_pid = pid;
  printf("pid = %d\n", msg_p->server_pid);
  while (1) {
    if (msg_p->type != MSG_TYPE_EMPTY) {
      // обработка сообщения
      if (msg_p->type == MSG_TYPE_STRING) {
        printf ("%d\n", msg_p->random_num);
      } else if (msg_p->type == MSG_TYPE_FINISH) {
        break;
      }
      msg_p->type = MSG_TYPE_EMPTY; // сообщение обработано
    }
  }

  // удаление сегмента разделяемой памяти
  shmdt (msg_p);
  if (shmctl (shmid, IPC_RMID, (struct shmid_ds *) 0) < 0) {
    sys_err ("server: shared memory remove error");
  }
  exit (0);
}

