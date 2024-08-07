#include "kernel/types.h"
#include "kernel/stat.h"
#include "user.h"

void ping(int sem, int count){
    for(unsigned int i = 0; i < count; i++){
        sem_down(sem);
        printf("ping\n");
        sem_up(sem);
    }
    sem_close(sem);
}

void pong(int sem, int count){
    for(unsigned int i = 0; i < count; i++){
        sem_down(sem);
        printf("pong\n");
        sem_up(sem);
    }
    sem_close(sem);
}

int 
main(int argc,char* argv[]) 
{
    char* aux = argv[1];
    int count= atoi(aux);
    
    int sem1=0, sem2=0;
    sem_open(sem1,1);
    sem_open(sem2,0);

    int pid = fork();
    if(pid == 0){
        ping(sem1, count);
    } else {
        pong(sem2, count);
    }

    printf("&d&d\n", sem1, sem2);
    
    exit(1);
} 