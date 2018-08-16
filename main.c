//Author: Nick Barnes 6/17/18


#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stddef.h>
#include <assert.h>
#include <stdarg.h>

#define assertsyscall(x,y) if(x y){int err=errno; {perror(#x); exit(err);}}
#define nullptr NULL

void handler(int signo) {
    	if(signo == SIGUSR1) {
            assert(printf("SIGUSR1 signal recieved! \n")!=0);
}
	if(signo == SIGUSR2) {
            assert(printf("SIGUSR2 signal recieved! \n")!=0);
}
	if(signo == SIGILL) {
            assert(printf("SIGILL signal recieved! \n")!=0);
}
}

int main()
{
	struct sigaction action;
	action.sa_handler = handler;
	sigemptyset (&action.sa_mask);
	action.sa_flags = SA_RESTART;
	assert(sigaction(SIGUSR1, &action, NULL)==0);
	assert(sigaction(SIGUSR2, &action, NULL)==0);
	assert(sigaction(SIGILL, &action, NULL)==0);	

	int cpid;
	assertsyscall((cpid = fork()),<0);

	if (cpid == 0) { //child
		assertsyscall(execl("./child", "child", (char*)nullptr),<0);
}
	
	else { //parent
		int wstatus;
		assertsyscall(wait(&wstatus),<0);
		
}	
}



