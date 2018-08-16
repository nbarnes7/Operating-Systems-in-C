//Author: Nick Barnes

#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <stdlib.h>

#define assertsyscall(x,y) if(x y){int err=errno; {perror(#x); exit(err);}}

int main()
{
	int ppid = getppid();
	for (int i = 0; i < 3; i++) {
   		kill(ppid,SIGUSR1);
}
	kill(ppid,SIGUSR2);
	kill(ppid,SIGILL);
	exit(EXIT_SUCCESS);
}
