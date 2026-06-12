#include "trace_runtime.h"

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/ptrace.h>
#include <sys/user.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#if !defined(__x86_64__)
#error "Este runtime didatico suporta apenas Linux x86_64."
#endif
static unsigned long saved_args[6] = {0};

static void fill_event_from_regs(pid_t pid,
                                 int entering,
                                 const struct user_regs_struct *regs,
                                 struct syscall_event *ev)
{
    memset(ev, 0, sizeof(*ev));
    ev->syscall_no = regs->orig_rax;
    ev->ret = regs->rax;
    ev->pid = pid;
    ev->entering = entering;

    if (entering) {
        saved_args[0] = regs->rdi;
        saved_args[1] = regs->rsi;
        saved_args[2] = regs->rdx;
        saved_args[3] = regs->r10;
        saved_args[4] = regs->r8;
        saved_args[5] = regs->r9;
    }
    ev->args[0] = saved_args[0];
    ev->args[1] = saved_args[1];
    ev->args[2] = saved_args[2];
    ev->args[3] = saved_args[3];
    ev->args[4] = saved_args[4];
    ev->args[5] = saved_args[5];
}


static pid_t launch_tracee(char *const argv[])
{
    pid_t pid = fork();

    if(pid == -1)
    {
        perror("Erro na criação do processo.");
        return -1;
    }

    if(pid == 0){
        ptrace(PTRACE_TRACEME,0,NULL,NULL);
        raise(SIGSTOP);
        execvp(argv[0],argv);

        perror("Erro durante a execução do argumento passado"); 
        exit(1); 
    }

    {
        return pid;
    }
}

static int wait_for_initial_stop(pid_t child)
{
    int status;
    if (waitpid(child,&status,0) == -1){
        perror("Erro na espera por processo filho");
        return -1;
    }

    if(WIFSTOPPED(status))
    {
        if(WSTOPSIG(status) == SIGSTOP)
        return 0;
    }
    return -1;

}

static int configure_trace_options(pid_t child)
{
    if (ptrace(PTRACE_SETOPTIONS, child, NULL, PTRACE_O_TRACESYSGOOD) == -1) {
        fprintf(stderr, "erro: TODO Semana 3: implementar configure_trace_options()\n");
        return -1;
    }
    return 0;

}

static int resume_until_next_syscall(pid_t child, int signal_to_deliver)
{
    if (ptrace(PTRACE_SYSCALL, child, NULL, signal_to_deliver) == -1) {
        fprintf(stderr, "erro: TODO Semana 3: implementar resume_until_next_syscall()\n");
        return -1;
    }
    return 0;

}


    static int wait_for_syscall_stop(pid_t child, int *status)
    {
        while(1)
    {
        if(waitpid(child,status,0) == -1)
        {
            perror("Erro na espera por processo filho com waipid()");
            return -1;
        }


        if(WIFEXITED(*status))
        {
            return 0;
        }

        if(WIFSIGNALED(*status))
        {
            return 0;
        }

        if(WIFSTOPPED(*status))
        {
            if(WSTOPSIG(*status) &  0x80)
        {
            return 1;
        }
        resume_until_next_syscall(child,0);
        }
        
    }
}

int trace_program(char *const argv[],
                  trace_observer_fn observer,
                  void *userdata)
{
    pid_t child;
    int status = 0;
    int entering = 1;

    if (argv == NULL || argv[0] == NULL) {
        fprintf(stderr, "erro: programa alvo ausente\n");
        return -1;
    }

    child = launch_tracee(argv);
    if (child < 0) {
        return -1;
    }

    if (wait_for_initial_stop(child) < 0) {
        return -1;
    }

    if (configure_trace_options(child) < 0) {
        return -1;
    }

    if (resume_until_next_syscall(child, 0) < 0) {
        return -1;
    }

    while (1) {
        struct user_regs_struct regs;
        struct syscall_event ev;
        int stop_kind;

        stop_kind = wait_for_syscall_stop(child, &status);
        if (stop_kind < 0) {
            return -1;
        }
        if (stop_kind == 0) {
            if (WIFEXITED(status)) {
                return WEXITSTATUS(status);
            }
            if (WIFSIGNALED(status)) {
                return 128 + WTERMSIG(status);
            }
            return 0;
        }

        memset(&regs, 0, sizeof(regs));
        ptrace(PTRACE_GETREGS, child , 0 , &regs);
        fill_event_from_regs(child, entering, &regs, &ev);

        if (observer != NULL) {
            observer(&ev, userdata);
        }

        entering = !entering;

        if (resume_until_next_syscall(child, 0) < 0) {
            return -1;
        }
    }
}
