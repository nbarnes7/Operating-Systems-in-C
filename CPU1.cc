// HW#3
// Author: Nick Barnes 


#include <iostream>
#include <list>
#include <iterator>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <assert.h>
#include <cstring>
#include <cstdlib>
#include <cstdio>

/*
This program does the following.
1) Create handlers for two signals.
2) Create an idle process which will be executed when there is nothing
   else to do.
3) Create a send_signals process that sends a SIGALRM every so often.

If compiled with -DEBUG, when run it should produce the following
output (approximately):

$ ./a.out
state:        3
name:         IDLE
pid:          21145
ppid:         21143
interrupts:   0
switches:     0
started:      0
in CPU.cc at 240 at beginning of send_signals getpid() = 21144
in CPU.cc at 245 sending signal = 14
in CPU.cc at 246 to pid = 21143
In ISR stopped:     21145
---- entering scheduler
continuing    21145
---- leaving scheduler
in CPU.cc at 245 sending signal = 14
in CPU.cc at 246 to pid = 21143
In ISR stopped:     21145
---- entering scheduler
continuing    21145
---- leaving scheduler
in CPU.cc at 245 sending signal = 14
in CPU.cc at 246 to pid = 21143
In ISR stopped:     21145
---- entering scheduler
continuing    21145
---- leaving scheduler
in CPU.cc at 245 sending signal = 14
in CPU.cc at 246 to pid = 21143
In ISR stopped:     21145
---- entering scheduler
continuing    21145
---- leaving scheduler
in CPU.cc at 245 sending signal = 14
in CPU.cc at 246 to pid = 21143
in CPU.cc at 250 at end of send_signals
In ISR stopped:     21145
---- entering scheduler
continuing    21145
Terminated: 15
---------------------------------------------------------------------------
Add the following functionality.
1) Change the NUM_SECONDS to 20.

2) Take any number of arguments for executables and place each on the
   processes list with a state of NEW. The executables will not require
   arguments themselves.

3) When a SIGALRM arrives, scheduler() will be called; it currently simply
   restarts the idle process. Instead, do the following.
   a) Update the PCB for the process that was interrupted including the
      number of context switches and interrupts it had and changing its
      state from RUNNING to READY.
   b) If there are any NEW processes on processes list, change its state to
      RUNNING, and fork() and execl() it.
   c) If there are no NEW processes, round robin the processes in the
      processes queue that are READY. If no process is READY in the
      list, execute the idle process.

4) When a SIGCHLD arrives notifying that a child has exited, process_done() is
   called. process_done() currently only prints out the PID and the status.
   a) Add the printing of the information in the PCB including the number
      of times it was interrupted, the number of times it was context
      switched (this may be fewer than the interrupts if a process
      becomes the only non-idle process in the ready queue), and the total
      system time the process took.
   b) Change the state to TERMINATED.
   c) Restart the idle process to use the rest of the time slice.
*/

#define NUM_SECONDS 20
#define EVER ;;

#define assertsyscall(x, y) if(!((x) y)){int err = errno; \
    fprintf(stderr, "In file %s at line %d: ", __FILE__, __LINE__); \
        perror(#x); exit(err);}

#ifdef EBUG
#   define dmess(a) cout << "in " << __FILE__ << \
    " at " << __LINE__ << " " << a << endl;

#   define dprint(a) cout << "in " << __FILE__ << \
    " at " << __LINE__ << " " << (#a) << " = " << a << endl;

#   define dprintt(a,b) cout << "in " << __FILE__ << \
    " at " << __LINE__ << " " << a << " " << (#b) << " = " \
    << b << endl
#else
#   define dmess(a)
#   define dprint(a)
#   define dprintt(a,b)
#endif

using namespace std;

// http://man7.org/linux/man-pages/man7/signal-safety.7.html

#define WRITES(a) { const char *foo = a; write(1, foo, strlen(foo)); }
#define WRITEI(a) { char buf[10]; assert(eye2eh(a, buf, 10, 10) != -1); WRITES(buf); }

enum STATE { NEW, RUNNING, WAITING, READY, TERMINATED };

struct PCB
{
    STATE state;
    const char *name;   // name of the executable
    int pid;            // process id from fork();
    int ppid;           // parent process id
    int interrupts;     // number of times interrupted
    int switches;       // may be < interrupts
    int started;        // the time this process started
};

PCB *running;
PCB *idle;

// http://www.cplusplus.com/reference/list/list/
list<PCB *> new_list;
list<PCB *> processes;

int sys_time;

/*
** Async-safe integer to a string. i is assumed to be positive. The number
** of characters converted is returned; -1 will be returned if bufsize is
** less than one or if the string isn't long enough to hold the entire
** number. Numbers are right justified. The base must be between 2 and 16;
** otherwise the string is filled with spaces and -1 is returned.
*/
int eye2eh(int i, char *buf, int bufsize, int base)
{
    if(bufsize < 1) return(-1);
    buf[bufsize-1] = '\0';
    if(bufsize == 1) return(0);
    if(base < 2 || base > 16)
    {
        for(int j = bufsize-2; j >= 0; j--)
        {
            buf[j] = ' ';
        }
        return(-1);
    }

    int count = 0;
    const char *digits = "0123456789ABCDEF";
    for(int j = bufsize-2; j >= 0; j--)
    {
        if(i == 0)
        {
            buf[j] = ' ';
        }
        else
        {
            buf[j] = digits[i%base];
            i = i/base;
            count++;
        }
    }
    if(i != 0) return(-1);
    return(count);
}

/*
** a signal handler for those signals delivered to this process, but
** not already handled.
*/
void grab(int signum) { WRITEI(signum); WRITES("\n"); }

// c++decl> declare ISV as array 32 of pointer to function(int) returning void
void(*ISV[32])(int) = {
/*        00    01    02    03    04    05    06    07    08    09 */
/*  0 */ grab, grab, grab, grab, grab, grab, grab, grab, grab, grab,
/* 10 */ grab, grab, grab, grab, grab, grab, grab, grab, grab, grab,
/* 20 */ grab, grab, grab, grab, grab, grab, grab, grab, grab, grab,
/* 30 */ grab, grab
};

/*
** stop the running process and index into the ISV to call the ISR
*/
void ISR(int signum)
{
    if(signum != SIGCHLD)
    {
        if(kill(running->pid, SIGSTOP) == -1)
        {
            WRITES("In ISR kill returned: ");
            WRITEI(errno);
            WRITES("\n");
            return;
        }

        WRITES("In ISR stopped: ");
        WRITEI(running->pid);
        WRITES("\n");
        running->state = READY;
    }

    ISV[signum](signum);
}

/*
** an overloaded output operator that prints a PCB
*/
ostream& operator <<(ostream &os, struct PCB *pcb)
{
    os << "state:        " << pcb->state << endl;
    os << "name:         " << pcb->name << endl;
    os << "pid:          " << pcb->pid << endl;
    os << "ppid:         " << pcb->ppid << endl;
    os << "interrupts:   " << pcb->interrupts << endl;
    os << "switches:     " << pcb->switches << endl;
    os << "started:      " << pcb->started << endl;
    return(os);
}

/*
** an overloaded output operator that prints a list of PCBs
*/
ostream& operator <<(ostream &os, list<PCB *> which)
{
    list<PCB *>::iterator PCB_iter;
    for(PCB_iter = which.begin(); PCB_iter != which.end(); PCB_iter++)
    {
        os <<(*PCB_iter);
    }
    return(os);
}

/*
**  send signal to process pid every interval for number of times.
*/
void send_signals(int signal, int pid, int interval, int number)
{
    dprintt("at beginning of send_signals", getpid());

    for(int i = 1; i <= number; i++)
    {
        assertsyscall(sleep(interval), == 0);
        dprintt("sending", signal);
        dprintt("to", pid);
        assertsyscall(kill(pid, signal), == 0)
    }

    dmess("at end of send_signals");
}

struct sigaction *create_handler(int signum, void(*handler)(int))
{
    struct sigaction *action = new(struct sigaction);

    action->sa_handler = handler;

/*
**  SA_NOCLDSTOP
**  If  signum  is  SIGCHLD, do not receive notification when
**  child processes stop(i.e., when child processes  receive
**  one of SIGSTOP, SIGTSTP, SIGTTIN or SIGTTOU).
*/
    if(signum == SIGCHLD)
    {
        action->sa_flags = SA_NOCLDSTOP | SA_RESTART;
    }
    else
    {
        action->sa_flags =  SA_RESTART;
    }

    sigemptyset(&(action->sa_mask));
    assert(sigaction(signum, action, NULL) == 0);
    return(action);
}

void scheduler(int signum)
{
    WRITES("---- entering scheduler\n");
    assert(signum == SIGALRM);
    sys_time++;
        
    running->interrupts++;

    bool found_one = false;

    for (int i = 0; i < (int)processes.size(); i++) {
	    PCB *torun = processes.front();
	    processes.pop_front();
	    processes.push_back(torun);

	    if (torun->state == NEW) {
		    torun->state = RUNNING;
		    torun->ppid = getpid();
		    torun->interrupts = 0;
		    torun->switches = 0;
		    torun->started = sys_time;
		    running = torun;
	            WRITES("Running New: ");
		    WRITES(torun->name);
		    WRITES("\n");
		    if ((torun->pid = fork()) == 0) {
			    assertsyscall(execl(torun->name, torun->name, NULL),<0);
		    }
		    found_one = true;
		    break;
	}
	    else if (torun->state == READY) {
		    WRITES("continuing");
		    WRITEI(torun->pid);
		    WRITES("\n");
		    if (running->pid != torun->pid) {
		    	running->switches++;
			}
		    torun->state = RUNNING;
		    running= torun;
		    if (kill(torun->pid, SIGCONT) == -1) {
			    assert(kill(0, SIGTERM)==0);
		    }
		    found_one = true;
		    break;
	    }
            if(kill(running->pid, SIGCONT) == -1){
        	WRITES("in sceduler kill error: ");
       	        WRITEI(errno);
                WRITES("\n");
                return;
            	}
	}
    if (!found_one) {
		    // continuing idle
		    idle->state = RUNNING;
		    running = idle;
		    if (kill (idle->pid, SIGCONT) == -1) {
			   assert(kill(0, SIGTERM)==0);
			}
	    
        }
    cout << running;
    WRITES("---- leaving scheduler\n");
}

void process_done(int signum)
{
    assert(signum == SIGCHLD);
    WRITES("---- entering process_done\n");

    // might have multiple children done.
    for(int i = 0; i < (int)processes.size(); i++)
    {
        int status, cpid;

        // we know we received a SIGCHLD so don't wait.
        cpid = waitpid(-1, &status, WNOHANG);

        if(cpid < 0)
        {
            WRITES("cpid < 0\n");
            assertsyscall(kill(0, SIGTERM), != 0);
        }
        else if(cpid == 0)
        {
            // no more children.
            break;
        }
        else
        {
	    running->state = TERMINATED;
	    WRITES("process exited: ");
	    WRITES("\n");
	    cout << running;
	    int totaltime = sys_time - running->started;
	    WRITES("Total System Time = ");
	    WRITEI(totaltime);
	    WRITES("\n");
        }
    }
    running = idle;
    idle->state = RUNNING;
    WRITES("---- leaving process_done\n");
}


/*
** set up the "hardware"
*/
void boot()
{
    sys_time = 0;

    ISV[SIGALRM] = scheduler; //ISV for SIGALRM sends to scheduler
    ISV[SIGCHLD] = process_done;
    struct sigaction *alarm = create_handler(SIGALRM, ISR); //create handler
    struct sigaction *child = create_handler(SIGCHLD, ISR); //create handler

    // start up clock interrupt
    int ret;
    if((ret = fork()) == 0) //create a child
    {
        send_signals(SIGALRM, getppid(), 1, NUM_SECONDS);

        // once that's done, cleanup and really kill everything...
        delete(alarm);
        delete(child);
        delete(idle);
        kill(0, SIGTERM);
    }

    if(ret < 0)
    {
        perror("fork");
    }
}

void create_idle()
{
    idle = new(PCB);
    idle->state = READY;
    idle->name = "IDLE";
    idle->ppid = getpid();
    idle->interrupts = 0;
    idle->switches = 0;
    idle->started = sys_time;

    if((idle->pid = fork()) == 0) //child
    {
        pause();
        perror("pause in create_idle");
    }
}

int main(int argc, char **argv)
{
    boot();

    
    create_idle();
    running = idle;
    cout << running;
	
    for (int i = 1; i < argc; i++) {
	
	PCB *process;
	process = new(PCB);
	process->state = NEW;
	process->name = argv[i];
	processes.push_back(process);
   	}

    
    //for (auto v : processes) { //for printing the new PCB's
     //   std::cout << v << "\n";
	//}

    // we keep this process around so that the children don't die and
    // to keep the IRQs in place.
    for(EVER)
    {
        // "Upon termination of a signal handler started during a
        // pause(), the pause() call will return."
        pause();
    }
}
