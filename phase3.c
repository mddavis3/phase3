/*
phase3.c

University of Arizona South
Computer Science 452
Michael Davis

for debug console printouts:
if (DEBUG2 && debugflag3)
{
    console ("\n");
}
*/

#include <usloss.h>
#include <phase1.h>
#include <phase2.h>
#include <phase3.h>

#include <stdlib.h>                      
#include <strings.h>                     
#include <stdio.h>                     
#include <string.h>  

#include <libuser.h>
#include <usyscall.h>
#include "sems.h"

/* ------------------------- Prototypes ----------------------------------- */
//semaphore 	running;
int start2 (char *); 
extern int start3 (char *);

static void spawn(sysargs *);
int spawn_real(char *, int (*func)(char *), char *, int, int);
int spawn_launch(char *);

static void wait(sysargs *);
int wait_real(int *);
int terminate_real(int);
static void nullsys3(sysargs *);


/* -------------------------- Globals ------------------------------------- */
int debugflag3 = 1;

pcb ProcessTable3[MAXPROC];
pcb dummy_pcb = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};





/* -------------------------- Functions ----------------------------------- */

/* ------------------------------------------------------------------------
   Name - start2
   Purpose - Initializes start3 process in user mode.
             Start the phase3 test process.
   Parameters - one, default arg passed by fork1, not used here.
   Returns - 
   Side Effects - lots since it initializes the phase3 data structures.
   ----------------------------------------------------------------------- */
int start2(char *arg)
{
    int		pid;
    int		status;
    /*
     * Check kernel mode here.
     */
    if ((psr_get() & PSR_CURRENT_MODE) == 0)
    {
        console("start2(): Not in kernel mode\n");
        halt(1);
    }
    
    /*
     * Data structure initialization as needed.
     */

    //initialize the phase 3 process table
    if (DEBUG2 && debugflag3)
    {
        console ("start2(): Initializing Process Table\n");
    }
    for (int i = 0; i < MAXPROC; i++)
    {
        ProcessTable3[i] = dummy_pcb;
    }

    //initialize the system call handlers
    if (DEBUG2 && debugflag3)
    {
        console ("start2(): Initializing Sys Call Handlers\n");
    }
    for (int i = 0; i < MAXSYSCALLS; i++)
    {
        sys_vec[i] = nullsys3;
    }
    sys_vec[SYS_SPAWN] = spawn; 
    sys_vec[SYS_WAIT] = wait; 
    //sys_vec[SYS_TERMINATE] = nullsys3; 
    //sys_vec[SYS_GETTIMEOFDAY] = nullsys3;
    //sys_vec[SYS_CPUTIME] = nullsys3;
    //sys_vec[SYS_GETPID] = nullsys3;

    /*
     * Create first user-level process and wait for it to finish.
     * These are lower-case because they are not system calls;
     * system calls cannot be invoked from kernel mode.
     * Assumes kernel-mode versions of the system calls
     * with lower-case names.  I.e., Spawn is the user-mode function
     * called by the test cases; spawn is the kernel-mode function that
     * is called by the syscall_handler; spawn_real is the function that
     * contains the implementation and is called by spawn.
     *
     * Spawn() is in libuser.c.  It invokes usyscall()
     * The system call handler calls a function named spawn() -- note lower
     * case -- that extracts the arguments from the sysargs pointer, and
     * checks them for possible errors.  This function then calls spawn_real().
     *
     * Here, we only call spawn_real(), since we are already in kernel mode.
     *
     * spawn_real() will create the process by using a call to fork1 to
     * create a process executing the code in spawn_launch().  spawn_real()
     * and spawn_launch() then coordinate the completion of the phase 3
     * process table entries needed for the new process.  spawn_real() will
     * return to the original caller of Spawn, while spawn_launch() will
     * begin executing the function passed to Spawn. spawn_launch() will
     * need to switch to user-mode before allowing user code to execute.
     * spawn_real() will return to spawn(), which will put the return
     * values back into the sysargs pointer, switch to user-mode, and 
     * return to the user code that called Spawn.
     */
    if (DEBUG2 && debugflag3)
    {
        console ("start2(): Calling spawn_real\n");
    }
    pid = spawn_real("start3", start3, NULL, 4*USLOSS_MIN_STACK, 3);

    if (DEBUG2 && debugflag3)
    {
        console ("start2(): Calling wait_real\n");
    }
    pid = wait_real(&status);

    if (DEBUG2 && debugflag3)
    {
        console ("start2(): Calling quit(0)\n");
    }
    quit(0);
} /* start2 */



/* spawn */
static void spawn(sysargs *args_ptr)
{

    //define local variables
    char *name;
    int (*func)(char *);
    char *arg;
    int stack_size;
    int priority;
    int kid_pid;

    //terminate process if zapped
    if (DEBUG2 && debugflag3)
    {
        console ("spawn(): Checking if zapped\n");
    }
    if (is_zapped()) 
    { 
        //should terminate the process 
    }

    //unpack the sysargs structure
    if (DEBUG2 && debugflag3)
    {
        console ("spawn(): Unpacking sysargs structure\n");
    }
    func = args_ptr->arg1;
    arg = args_ptr->arg2;
    stack_size = (int) args_ptr->arg3;
    priority = (int) args_ptr->arg4;
    name = args_ptr->arg5;

    // exceptional conditions checking and handling
    // call another function to modularize the code better
    if (DEBUG2 && debugflag3)
    {
        console ("spawn(): Calling spawn_real\n");
    }
    kid_pid = spawn_real(name, func, arg, stack_size, priority);

    if (DEBUG2 && debugflag3)
    {
        console ("spawn(): Returning kid_pid\n");
    }
    args_ptr->arg1 = (void *) kid_pid;
    args_ptr->arg4 = (void *) 0;

    //terminate process if zapped
    if (DEBUG2 && debugflag3)
    {
        console ("spawn(): Checking if zapped\n");
    }
    if (is_zapped()) 
    { 
        //should terminate the process 
    }

    /* set to user mode */
    if (DEBUG2 && debugflag3)
    {
        console ("spawn(): Setting user mode\n");
    }
    psr_set(psr_get() & ~PSR_CURRENT_MODE);   

    if (DEBUG2 && debugflag3)
    {
        console ("spawn(): Return\n");
    }
    return;
} /* spawn */



/* spawn_real */
int spawn_real(char *name, int (*func)(char *), char *arg, int stack_size, int priority)
{
    //Define local variables
    if (DEBUG2 && debugflag3)
    {
        console ("spawn_real(): Defining local variables\n");
    }
    int kidpid;
    int my_location;  /* parent's location in process table */
    int kid_location; /* child's  location in process table */
    int result;
    pcb_ptr kidptr, prevptr;
    

    my_location = getpid() % MAXPROC;

    //Create a mailbox for start2
    //Every other process will have a mailbox created for it later
    if (name = "start3")
    {
        if (DEBUG2 && debugflag3)
        {
            console ("spawn_real(): Creating start_mbox for start2\n");
        }
        ProcessTable3[my_location].start_mbox = MboxCreate(0, MAX_MESSAGE);
    }

    /* create our child */
    if (DEBUG2 && debugflag3)
    {
        console ("spawn_real(): Calling fork1\n");
    }
    kidpid = fork1(name, spawn_launch, NULL, stack_size, priority);

    //more to check the kidpid and put the new process data to the process table
    if (kidpid == -1)
    {   
        if (DEBUG2 && debugflag3)
        {
            console ("spawn_real(): kidpid = -1, no process created\n");
        }
        return kidpid;
    }
    kid_location = kidpid % MAXPROC;
    ProcessTable3[kid_location].pid = kidpid;
    ProcessTable3[kid_location].start_mbox = MboxCreate(0, MAX_MESSAGE);
    ProcessTable3[kid_location].parent_ptr = &ProcessTable3[my_location];
    ProcessTable3[kid_location].start_arg = arg;
    ProcessTable3[kid_location].start_func = func;

    //Then synchronize with the child using a mailbox
    if (DEBUG2 && debugflag3)
    {
        console ("spawn_real(): Calling MboxSend\n");
    }
    result = MboxSend(ProcessTable3[kid_location].start_mbox, &my_location, sizeof(int));

    //more to add
    if (DEBUG2 && debugflag3)
    {
        console ("spawn_real(): Returning kidpid\n");
    }
    return kidpid;
} /* spawn_real */



/* spawn_launch */
int spawn_launch(char *arg)
{
    if (DEBUG2 && debugflag3)
    {
        console ("spawn_launch(): Defining local variables\n");
    }
    int parent_location = 0;
    int my_location;
    int result;
    int (* start_func) (char *);
    char *start_arg;
    pcb_ptr parent_ptr;
    // more to add if you see necessary

    my_location = getpid() % MAXPROC;

    /* Sanity Check */

    /* Maintain the process table entry, you can add more */
    if (DEBUG2 && debugflag3)
    {
        console ("spawn_launch(): Maintaining Process Table entry\n");
    }
    ProcessTable3[my_location].status = ITEM_IN_USE;

    //if the parent process established the child process control block first
    //then these statements will work
    //may want to wrap these in some sort of conditional block
    start_func = ProcessTable3[my_location].start_func;
    start_arg = ProcessTable3[my_location].start_arg;

    if (ProcessTable3[my_location].parent_ptr->child_ptr == NULL)
    {
        if (DEBUG2 && debugflag3)
        {
            console ("spawn_launch(): Parent has no child, maintain pointers\n");
        }
        ProcessTable3[my_location].parent_ptr->child_ptr = &ProcessTable3[my_location];
    }

    //Synchronize with the parent process 
    if (DEBUG2 && debugflag3)
    {
        console ("spawn_launch(): Calling MboxReceive\n");
    }
    result = MboxReceive(ProcessTable3[my_location].start_mbox, &parent_location, sizeof(int));

    //add more code
    if (DEBUG2 && debugflag3)
    {
        console ("spawn_launch(): Checking if zapped\n");
    }
    if ( !is_zapped() ) 
    {
        if (DEBUG2 && debugflag3)
        {
            console ("spawn_launch(): Not zapped, set user mode\n");
        }
        //more code if you see necessary
        //Set user mode
        psr_set(psr_get() & ~PSR_CURRENT_MODE);

        //double check the psr mode
        if (DEBUG2 && debugflag3)
        {
            if ((psr_get() & PSR_CURRENT_MODE) == 0)
            {
                console("spawn_launch(): in user mode\n");
            }
        }

        result = (start_func)(start_arg);

        if (DEBUG2 && debugflag3)
        {
            console ("spawn_launch(): Terminate(result)\n");
        }
        Terminate(result);
    }
    else 
    {
        if (DEBUG2 && debugflag3)
        {
            console ("spawn_launch(): zapped, terminate_real(0)\n");
        }
        terminate_real(0);
    }
    printf("spawn_launch(): should not see this message following Terminate!\n");

    return 0;
} /* spawn_launch */



/* wait */
static void wait(sysargs *args_ptr)
{

    int status;
    int kid_pid;

    //terminate process if zapped
    if (DEBUG2 && debugflag3)
    {
        console ("wait(): Checking if zapped\n");
    }
    if (is_zapped()) 
    { 
        //should terminate the process 
    }

    if (DEBUG2 && debugflag3)
    {
        console ("wait(): Calling wait_real\n");
    }
    kid_pid = wait_real(&status);

    if (DEBUG2 && debugflag3)
    {
        console ("wait(): Returning kid_pid and status\n");
    }
    args_ptr->arg1 = (void *) kid_pid;
    args_ptr->arg2 = (void *) status;
    args_ptr->arg4 = (void *) 0;

    //terminate process if zapped
    if (DEBUG2 && debugflag3)
    {
        console ("wait(): Checking if zapped\n");
    }
    if (is_zapped()) 
    { 
        //should terminate the process 
    }

    return;
} /* wait */



/* wait_real */
int wait_real(int *status)
{
    if (DEBUG2 && debugflag3)
    {
        console ("wait_real(): Defining local variables\n");
    }
    int my_location;
    int kid_location = 0;
    int result;

    my_location = getpid() % MAXPROC;
    

    //if the process has no children
    if (ProcessTable3[my_location].child_ptr == NULL)
    {
        if (DEBUG2 && debugflag3)
        {
            console ("wait_real(): process has no children, return -1\n");
        }
        return -1; 
    }

    //block the process until the child process terminates
    if (DEBUG2 && debugflag3)
    {
        console ("wait_real(): calling MboxReceive to block process\n");
    }
    result = MboxReceive(ProcessTable3[my_location].start_mbox, &kid_location, sizeof(int));

    //success
    if (DEBUG2 && debugflag3)
    {
        console ("wait_real(): returning 0\n");
    }
    return 0;
} /* wait_real */



/* terminate_real */
int terminate_real(int status)
{
    quit(0);
    return 0;
} /* terminate_real */



/* nullsys3 */
static void nullsys3(sysargs *args_ptr)
{
   printf("nullsys3(): Invalid syscall %d\n", args_ptr->number);
   printf("nullsys3(): process %d terminating\n", getpid());
   terminate_real(1);
} /* nullsys3 */
