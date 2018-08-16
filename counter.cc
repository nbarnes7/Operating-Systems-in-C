//Author: Nick Barnes 

#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>

#define assertsyscall(x,y) if(x y){int err=errno; {perror(#x); exit(err);}}

int main(__attribute__((unused))int argc, char* argv[]) {
	char* endpoint;
	int numberOfLoops = strtol(argv[1], &endpoint, 10);
	for (int i = 0; i < numberOfLoops; i++) {
		std::cout << "Process: " << getpid() << " " << i + 1 << std::endl;
}
	exit(numberOfLoops);
}
