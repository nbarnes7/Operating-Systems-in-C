//Author: Nick Barnes

#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <stdarg.h>

#define assertsyscall(x,y) if((x)y){int err=errno; {perror(#x); exit(err);}}
#define nullptr NULL

int main()
{
	int cpid;
	assertsyscall(cpid = fork(),<0);
	if (cpid != 0){
		std::cout << "Child PID: " << cpid << std::endl;
		std::cout << "Parent PID: " << getpid() << std::endl;
}
	if (cpid == 0) {
		execl("./counter", "counter", "5", (char*)nullptr);
}
	int wstatus = 0;
	assertsyscall(wait(&wstatus),<0);
	if (WIFEXITED(wstatus)) {
		int childReturn = WEXITSTATUS(wstatus);
		std::cout << "Process " << cpid << " exited with status: " << childReturn << std::endl;
}
	
}
