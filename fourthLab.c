#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/wait.h>
#include "sys/ipc.h"
#include <libgen.h>
#include "sys/shm.h"

#define TREE_COUNT 8 
#define PERM S_IRUSR|S_IWUSR 

char *exe_name;
int sig1 = 0, sig2 = 0;
pid_t gID, ppID;
struct sigaction sact;


size_t m_size;
key_t shmid_Tree;
pid_t *process_ID; 

struct timeval curr_Time, start_Time, elapsed_Time;

void treeDo(int index);	
void forestDo(int pID);

int getOrdForTree(int pid) {
	for (int i = TREE_COUNT;; i--) if (process_ID[i] == pid) return i;
	return -1;
}

void treeDo(int index) {
	if (fork() == 0) {
		process_ID = shmat(shmid_Tree, 0, 0);
		process_ID[index] = getpid();		
		forestDo(process_ID[index]);
	}
}

void forestDo(int pID) {
	gID = getpgid(pID);
	ppID = getppid();

	int treeOrd = getOrdForTree(pID);
	switch(treeOrd) {
		case 1:
			treeDo(2);
			treeDo(3);
			break;
		case 2:
			break;
		case 3:			
			treeDo(4);			
			break;
		case 4:
			treeDo(5);
			treeDo(6);
			break;
		case 5:	
			break;
		case 6:
			treeDo(7);
			break;
		case 7:
			treeDo(8);
			break;
		case 8:		
			break;
		default:
			break;
	}
	while (1) pause();
}

int gettime() 
{
	gettimeofday(&curr_Time, NULL);
	timersub(&curr_Time, &start_Time, &elapsed_Time);
	return elapsed_Time.tv_usec/1000;
} 

void manageTree1() {
	sig1++;	
	if (sig1 + sig2 > 100) {
		kill(process_ID[8], SIGTERM);
	}
    else {
		printf("%d  %d  %d  put  SIGUSR2  %3d\n", 1, getpid(), getppid(), gettime());    		
		kill(-getpgid(process_ID[2]), SIGUSR2);
    }
}

void manageTree2() {
	sig2++;
	printf("%d  %d  %d  put  SIGUSR1  %3d\n", 2, getpid(), getppid(), gettime());
	kill(process_ID[6], SIGUSR1);
}

void manageTree3() {
	sig2++;
	printf("%d  %d  %d  put  SIGUSR1  %3d\n", 3, getpid(), getppid(), gettime());
	kill(process_ID[7], SIGUSR1);
}

void manageTree4() {
	sig2++;
	printf("%d  %d  %d  put  SIGUSR1  %3d\n", 4, getpid(), getppid(), gettime());
	kill(process_ID[8], SIGUSR1);
}

void manageTree5() {
	sig2++;
}

void manageTree6() {
	sig1++;
}

void manageTree7() {
	sig1++;
}

void manageTree8() {
	sig1++;
	printf("%d  %d  %d  put  SIGUSR1  %3d\n", 8, getpid(), getppid(), gettime());
	kill(process_ID[1], SIGUSR1);
}

static void manage (int signal) {
	int timer = gettime();
	int tree_Num = getOrdForTree(getpid());
	
	if (signal == SIGTERM) {
       switch (tree_Num) {
			case 1:	waitpid(process_ID[1], 0, 0);
                printf("%d  %d  %d  done SIGUSR1: %3d,  SIGUSR2: %3d\n", tree_Num, getpid(), ppID, sig1, sig2);
                kill(process_ID[0],SIGTERM);
                exit(EXIT_SUCCESS);  
				break;
		   	case 0: 
			    wait(NULL);
            	exit(EXIT_SUCCESS); 
				break;
			default: // 2,3,4,5,6,7,8
				printf("%d  %d  %d  done SIGUSR1: %3d,  SIGUSR2: %3d\n", tree_Num, getpid(), ppID, sig1, sig2);
            	kill(process_ID[tree_Num-1],SIGTERM);
            	exit(EXIT_SUCCESS);   
				break;
	   	}	
		return;
	}	   		
			
  	if (signal == SIGUSR1) {
        switch (tree_Num) {
        	case 1: 
				printf("%d  %d  %d  get  SIGUSR1  %3i\n", 1, getpid(), getppid(), timer);
				manageTree1();
				break;
        	case 6: 
				printf("%d  %d  %d  get  SIGUSR1  %3i\n", 6, getpid(), getppid(), timer);
				manageTree6();
				break;
        	case 7:
				printf("%d  %d  %d  get  SIGUSR1  %3i\n", 7, getpid(), getppid(), timer); 
				manageTree7();
				break;
        	case 8:
				printf("%d  %d  %d  get  SIGUSR1  %3i\n", 8, getpid(), getppid(), timer);
				manageTree8();
				break;
		}
		return;
	}

    if (signal == SIGUSR2) { 
		switch (tree_Num) {         	
        	case 2: 
				printf("%d  %d  %d  get  SIGUSR2  %3i\n", 2, getpid(), getppid(), timer);
				manageTree2();
				break;				
        	case 3: 
				printf("%d  %d  %d  get  SIGUSR2  %3i\n", 3, getpid(), getppid(), timer);
				manageTree3();
				break;
        	case 4:
				printf("%d  %d  %d  get  SIGUSR2  %3i\n", 4, getpid(), getppid(), timer); 
				manageTree4();
				break;
			case 5: 
				printf("%d  %d  %d  get  SIGUSR2  %3i\n", 5, getpid(), getppid(), timer);
				manageTree5();
				break;	
		}
		return;	     
	}
	return;
}

int main(int argc, char *argv[]) {
	exe_name = basename(argv[0]);		
	m_size = (TREE_COUNT+1) * sizeof(pid_t);

	if ((shmid_Tree = shmget(IPC_PRIVATE, m_size , PERM)) == -1 ) { 
		perror(exe_name);
        exit(EXIT_FAILURE);
    }
         
    process_ID = shmat(shmid_Tree, 0, 0);
		
    memset(process_ID, 0, m_size);
    process_ID[0] = getpid(); 
    
  	sact.sa_handler = manage;
  	sigemptyset(&sact.sa_mask);
  	sact.sa_flags = 0;

  	if (sigaction(SIGUSR1, &sact, 0) != 0) perror(exe_name);
  	if (sigaction(SIGUSR2, &sact, 0) != 0) perror(exe_name);	
  	if (sigaction(SIGTERM, &sact, 0) != 0) perror(exe_name);	
	
	if (fork() == 0) {		
	    process_ID[1] = getpid(); 
        forestDo(process_ID[1]);	   
	} else {      
	    process_ID[1] = process_ID[0]+1;       
    }

	if (setpgid(process_ID[2], process_ID[2]) != 0) perror(exe_name);	
	if (setpgid(process_ID[3], process_ID[2]) != 0) perror(exe_name);	
	if (setpgid(process_ID[4], process_ID[2]) != 0) perror(exe_name);		
	if (setpgid(process_ID[5], process_ID[2]) != 0) perror(exe_name);	

	int flag = 0;
	while (!flag) {
		int temp = -1;
		for (int i = 1; i <= TREE_COUNT; i++) {
			temp = temp && process_ID[i];
		}
		flag = temp;
	}
	
	gettimeofday(&start_Time, NULL);
	
	if (kill(process_ID[1], SIGUSR1) != 0) perror(exe_name);
   
	pid_t wpid;
	int status;
    while (1) pause();	
}
