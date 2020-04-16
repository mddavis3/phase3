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
#include <time.h>  

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

static void terminate(sysargs *);
int terminate_real(int);

static void cpuTime(sysargs *);
int cpuTime_real(int *);

static void getTimeOfDay(sysargs *);
int getTimeOfDay_real(int *);

static void getPID(sysargs *);
int getPID_real(int *);

static void nullsys3(sysargs *);

int insertChild(int, int);
int removeChild(int);

/* -------------------------- Globals ------------------------------------- */
int debugflag3 = 0;

pcb ProcessTable3[MAXPROC];
pcb dummy_pcb = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};





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
    sys_vec[SYS_TERMINATE] = terminate; 
    sys_vec[SYS_GETTIMEOFDAY] = getTimeOfDay;
    sys_vec[SYS_CPUTIME] = cpuTime;
    sys_vec[SYS_GETPID] = getPID;

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
    if (DEBUG2 && debugflag3)
    {
        console ("spawn(): defining local variables\n");
    }
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

    if (is_zapped())
    {
        if (DEBUG2 && debugflag3)
        {
            console ("spawn_real(): Is zapped. terminate_real(0)\n");
        }
        terminate_real(0);
    }
    //Create a mailbox for start2
    //start2 pcb entries
    //Every other process will have a mailbox created for it later
    if (name == "start3")
    {
        if (DEBUG2 && debugflag3)
        {
            console ("spawn_real(): Creating start_mbox for start2\n");
        }
        ProcessTable3[my_location].pid = getpid();
        ProcessTable3[my_location].start_mbox = MboxCreate(0, MAX_MESSAGE);
        ProcessTable3[my_location].name = "start2";
    }

    /* create our child */
    if (DEBUG2 && debugflag3)
    {
        console ("spawn_real(): Calling fork1\n");
    }
    kidpid = fork1(name, spawn_launch, NULL, stack_size, priority);

    //check the kidpid and put the new process data to the process table
    if (kidpid == -1)
    {   
        if (DEBUG2 && debugflag3)
        {
            console ("spawn_real(): kidpid = -1, no process created\n");
        }
        return kidpid;
    }
    kid_location = kidpid % MAXPROC;
    if (ProcessTable3[kid_location].pid == NULL)
    {
        ProcessTable3[kid_location].pid = kidpid;
    }
    if (ProcessTable3[kid_location].start_mbox == NULL)
    {
        ProcessTable3[kid_location].start_mbox = MboxCreate(0, MAX_MESSAGE);
    }
    ProcessTable3[kid_location].parent_ptr = &ProcessTable3[my_location];
    ProcessTable3[kid_location].name = name;
    ProcessTable3[kid_location].num_of_children = 0;
    ProcessTable3[kid_location].start_arg = arg;
    ProcessTable3[kid_location].start_func = func;

    //Then synchronize with the child using a mailbox
    if (DEBUG2 && debugflag3)
    {
        console ("spawn_real(): Calling MboxSend\n");
    }
    result = MboxSend(ProcessTable3[kid_location].start_mbox, &my_location, sizeof(int));

    if (is_zapped())
    {
        if (DEBUG2 && debugflag3)
        {
            console ("spawn_real(): Is zapped. terminate_real(0)\n");
        }
        terminate_real(0);
    }

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

    //get child's location in process table
    my_location = getpid() % MAXPROC;

    /* Maintain the process table entry, you can add more */
    if (DEBUG2 && debugflag3)
    {
        console ("spawn_launch(): Maintaining Process Table entry\n");
    }
    ProcessTable3[my_location].status = ITEM_IN_USE;

    //if child runs before parent after fork
    if (ProcessTable3[my_location].pid == NULL)
    {
        ProcessTable3[my_location].pid = getpid();
    }
    if (ProcessTable3[my_location].start_mbox == NULL)
    {
        ProcessTable3[my_location].start_mbox = MboxCreate(0, MAX_MESSAGE);
        if (DEBUG2 && debugflag3)
        {
            console ("spawn_launch(): Calling MboxReceive (child ran first)\n");
        }
        result = MboxReceive(ProcessTable3[my_location].start_mbox, &parent_location, sizeof(int));
    }

    //initialize the start_func and start_arg 
    //after parent has unblocked child using mboxsend
    start_func = ProcessTable3[my_location].start_func;
    start_arg = ProcessTable3[my_location].start_arg;

    //insert the child to parents linked list
    //result will equal the number of children the parent has
    if (DEBUG2 && debugflag3)
    {
        console ("spawn_launch(): calling insertChild - %s\n", ProcessTable3[my_location].name);
    }
    parent_location = ProcessTable3[my_location].parent_ptr->pid % MAXPROC;
    ProcessTable3[parent_location].num_of_children = insertChild(my_location, parent_location);

    if (DEBUG2 && debugflag3)
    {
        console ("spawn_launch(): Process %s num_of_children = %d\n", ProcessTable3[parent_location].name, ProcessTable3[parent_location].num_of_children);
    }

    //Synchronize with the parent process 
    if (result != sizeof(int))
    {
        if (DEBUG2 && debugflag3)
        {
            console ("spawn_launch(): Calling MboxReceive\n");
        }
        result = MboxReceive(ProcessTable3[my_location].start_mbox, &parent_location, sizeof(int));
    }
    
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
        terminate(args_ptr);
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
        terminate(args_ptr); 
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
    int kidpid;

    my_location = getpid() % MAXPROC;

    if (is_zapped())
    {
        if (DEBUG2 && debugflag3)
        {
            console ("spawn_real(): Is zapped. terminate_real(0)\n");
        }
        terminate_real(0);
    }
    
    //if the process has no children
    if (ProcessTable3[my_location].child_ptr == NULL)
    {
        if (DEBUG2 && debugflag3)
        {
            console ("wait_real(): process %s has no children, return -1\n", ProcessTable3[my_location].name);
        }
        return -1; 
    }
    else
    {
        kidpid = ProcessTable3[my_location].child_ptr->pid;
    }
    
    //block the process until the child process terminates
    if (DEBUG2 && debugflag3)
    {
        console ("wait_real(): calling MboxReceive to block process until child terminates\n");
    }
    ProcessTable3[my_location].status = ITEM_WAITING;
    result = MboxReceive(ProcessTable3[my_location].start_mbox, status, sizeof(int));
    ProcessTable3[my_location].status = ITEM_IN_USE;
    join(&status);

    if (is_zapped())
    {
        if (DEBUG2 && debugflag3)
        {
            console ("spawn_real(): Is zapped. terminate_real(0)\n");
        }
        terminate_real(0);
    }

    //success
    if (DEBUG2 && debugflag3)
    {
        console ("wait_real(): returning kidpid\n");
    }
    return kidpid;
} /* wait_real */



/* terminate */
static void terminate(sysargs *args_ptr)
{
    //define local variables
    if (DEBUG2 && debugflag3)
    {
        console ("terminate(): defining local variables\n");
    }
    int status;
    int result;

    //unpack sysargs struct
    if (DEBUG2 && debugflag3)
    {
        console ("terminate(): unpacking sysargs structure\n");
    }
    status = (int) args_ptr->arg1;

    //call terminate_real handler
    if (DEBUG2 && debugflag3)
    {
        console ("terminate(): calling terminate_real\n");
    }
    result = terminate_real(status);

    return ;
} /* terminate */



/* terminate_real */
int terminate_real(int status)
{
    //define local variables
    if (DEBUG2 && debugflag3)
    {
        console ("terminate_real(): defining local variables\n");
    }
    int my_location = getpid() % MAXPROC;
    int parent_location;
    int result;

    parent_location = ProcessTable3[my_location].parent_ptr->pid % MAXPROC;
    
    if (ProcessTable3[my_location].num_of_children == 0)
    {
        if(ProcessTable3[parent_location].status == ITEM_WAITING)
        {
            if (DEBUG2 && debugflag3)
            {
                console ("terminate_real(): Process %s - no child. Call MboxSend - sync w/%s wait\n", ProcessTable3[my_location].name, ProcessTable3[parent_location].name);
            }
            removeChild(ProcessTable3[my_location].parent_ptr->pid);
            result = MboxSend(ProcessTable3[parent_location].start_mbox, &status, sizeof(int));
        }
        else
        {
            removeChild(ProcessTable3[my_location].parent_ptr->pid);
        }
        
        if (DEBUG2 && debugflag3)
        {
            console ("terminate_real(): Process %s - no child. quit(0)\n", ProcessTable3[my_location].name);
        }
        //removeChild(ProcessTable3[my_location].parent_ptr->pid);
        quit(0);
    }

    //call zap to zap each child
    if (DEBUG2 && debugflag3)
    {
        console ("terminate_real(): entering zap, removeChild while loop\n");
    }
    while (ProcessTable3[my_location].num_of_children != 0)
    {
        if (DEBUG2 && debugflag3)
        {
            console ("terminate_real(): zapping %s, pid %d\n", ProcessTable3[my_location].child_ptr->name, ProcessTable3[my_location].child_ptr->pid);
        }
        //call zap to zap child
        result = zap(ProcessTable3[my_location].child_ptr->pid);
        //maintain linked list for parent/children
        result = removeChild(my_location);
    }

    if (DEBUG2 && debugflag3)
    {
        console ("terminate_real(): completed while loop\n");
    }

    //terminate user-level process using quit(0)
    if (DEBUG2 && debugflag3)
    {
        console ("terminate_real(): Process %s call MboxSend - sync w/%s wait\n", ProcessTable3[my_location].name, ProcessTable3[parent_location].name);
    }
    removeChild(ProcessTable3[parent_location].pid);
    result = MboxSend(ProcessTable3[parent_location].start_mbox, &status, sizeof(int));

    if (DEBUG2 && debugflag3)
    {
        console ("terminate_real(): calling quit(0) for process %s\n", ProcessTable3[my_location].name);
    }
    quit(0);

    return 0;
} /* terminate_real */



/* cpuTime */
static void cpuTime(sysargs *args_ptr)
{
    int time;
    int result;

    result = cpuTime_real(&time);

    args_ptr->arg1 = (void *) result;
    return;
} /* cpuTime */



/* cpuTime_real */
int cpuTime_real(int *time)
{
    time = readtime();
    return time;
} /* cpuTime_real */



/* getTimeOfDay */
static void getTimeOfDay(sysargs *args_ptr)
{
    int timeOfDay;
    int result;

    result = getTimeOfDay_real(&timeOfDay);

    args_ptr->arg1 = (void *) result;
    return;
} /* getTimeOfDay */



/* getTimeOfDay_real */
int getTimeOfDay_real(int *timeOfDay)
{
    time_t currentTime;
    timeOfDay = (int) time(&currentTime);
    return timeOfDay;
} /* getTimeOfDay_real */



/* getPID */
static void getPID(sysargs *args_ptr)
{
    int pid;
    int result;

    result = getPID_real(&time);

    args_ptr->arg1 = (void *) result;
    return;
} /* getPID */



/* getPID_real */
int getPID_real(int *pid)
{
    pid = getpid();
    return pid;
} /* getPID_real */



/* nullsys3 */
static void nullsys3(sysargs *args_ptr)
{
   printf("nullsys3(): Invalid syscall %d\n", args_ptr->number);
   printf("nullsys3(): process %d terminating\n", getpid());
   terminate_real(1);
} /* nullsys3 */



/* insertChild */
int insertChild(int child_location, int parent_location)
{
    int num_of_children = 0;
    pcb_ptr temp_ptr;

    //check if parent's child_ptr is null
    //number of children starts at 0
    if (ProcessTable3[parent_location].child_ptr == NULL)
    {
        ProcessTable3[parent_location].child_ptr = &ProcessTable3[child_location];
        num_of_children++;
    }
    //parent's child_ptr is not null
    //number of children starts at 1
    else
    {
        num_of_children = 1;

        //point the temp_ptr at the first child
        temp_ptr = ProcessTable3[parent_location].child_ptr;

        //while the child's sibling_ptr is not NULL
        //point the temp_ptr at the sibling
        while (temp_ptr->sibling_ptr != NULL)
        {
            temp_ptr = temp_ptr->sibling_ptr;
            num_of_children++;
        }

        //point the NULL sibling_ptr at the target child PCB
        temp_ptr->sibling_ptr = &ProcessTable3[child_location];
        num_of_children++;
    }
    
    return num_of_children;
} /* insertChild */



/* removeChild */
int removeChild(int parent_location)
{
    pcb_ptr temp;
    int child_location;

    //if parent has no children
    if (ProcessTable3[parent_location].num_of_children == 0)
    {
        if (DEBUG2 && debugflag3)
        {
            console ("removeChild(): Process %s has no children\n", ProcessTable3[parent_location].name);
        }
        return ProcessTable3[parent_location].num_of_children;
    }
    //if parent has >= 1 children
    else
    {
        temp = ProcessTable3[parent_location].child_ptr;

        //point the parent's child_ptr at the child's sibling
        ProcessTable3[parent_location].child_ptr = ProcessTable3[parent_location].child_ptr->sibling_ptr;
        ProcessTable3[parent_location].num_of_children--;

        //erase removed child's sibling ptr
        temp->sibling_ptr = NULL;

        //cleanup Process Table
        //child_location = temp->pid % MAXPROC;
        //ProcessTable3[child_location] = dummy_pcb;
    }
    
    return ProcessTable3[parent_location].num_of_children;
} /* removeChild */