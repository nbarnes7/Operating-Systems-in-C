//Author: Nick Barnes 7/24/18

#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <stdlib.h>
#include <assert.h>
#include <fcntl.h>
#include <string.h>
#include <string>
#include <iomanip>
#include <locale>
#include <sstream>


#define assertsyscall(x,y) if(x y){int err=errno; {perror(#x); exit(err);}}
#define READ 0
#define WRITE 1

int main(int argc, char** argv)
{
	int pid = getpid();
	for (int i = 1; i <= 3; ++i) {
		 	std::ostringstream str1;
	                str1 << i;
	        	std::string temp = str1.str();
			char *message = (char*)temp.c_str();
        		assert(write(3, message, strlen(message))!= -1);
			printf("\nIN CHILD: %d \n", pid);
			kill(getppid(),SIGTRAP);
			char buffer[1024];
            		int len;
            		len = read(4, buffer, sizeof (buffer));
            		buffer[len] = 0;
            		printf("%s\n", buffer);
		
	
	}
	char *message = (char*)"4Hello World!";
        assert(write(3, message, strlen(message))!= -1);
	printf("\nIN CHILD: %d \n", pid);
	kill(getppid(),SIGTRAP);
	char buffer[1024];
        int len;
        len = read(4, buffer, sizeof (buffer));
        buffer[len] = 0;
        printf("%s\n", buffer);
		
	exit(0);
}
	

