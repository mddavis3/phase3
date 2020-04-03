#define DEBUG2 1
#define ITEM_IN_USE 1

#define CHECKMODE { \
	if (psr_get() & PSR_CURRENT_MODE) { \
    		console("Trying to invoke syscall from kernel\n"); \
    		halt(1); \
	} \
}

typedef struct pcb pcb;
typedef struct pcb *pcb_ptr;

/* Phase3 Process Control Block */
struct pcb {
    int pid;
    int status;
    pcb_ptr parent_ptr;
    pcb_ptr child_ptr;
    pcb_ptr sibling_ptr;
    int start_mbox;
};