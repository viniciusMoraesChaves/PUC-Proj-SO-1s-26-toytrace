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
    /*
     * TODO Semana 4:
     *struct syscall_event {
     * pid_t pid;
     * int entering;               1 na entrada da syscall, 0 na saida
     * long syscall_no;
     * long ret;                  valido apenas em eventos de saida
    *  unsigned long args[6];     argumentos capturados na entrada
    *   };
    */

    /*
     * Preencha struct syscall_event usando os registradores x86_64.
     *
     * Dicas:
     * - regs->orig_rax contem o numero da syscall.
     * - regs->rax contem o retorno, valido na saida.
     * - os seis argumentos ficam em rdi, rsi, rdx, r10, r8 e r9.
     * - ev->entering deve copiar o parametro entering.
    */

    memset(ev, 0, sizeof(*ev));
    ev->syscall_no = regs->orig_rax;
    ev->ret = regs->rax ;
    ev->pid = pid;
    ev->entering = entering;
    ev->args[0] = regs->rdi;
    ev->args[1] = regs->rsi;
    ev->args[2] = regs->rdx;
    ev->args[3] = regs->r10;
    ev->args[4] = regs->r8;
    ev->args[5] = regs->r9;

}


static pid_t launch_tracee(char *const argv[])

// FEITO SEMANA 2
{
    pid_t pid = fork();

    if(pid == -1)
    {
        perror("Erro na criação do processo.");
        return -1;
    }

    if(pid == 0){
        ptrace(PTRACE_TRACEME,0,NULL,NULL); // sintaxe do comando ptrace, que é chamada por uma trace;
        raise(SIGSTOP);
        execvp(argv[0],argv);

        perror("Erro durante a execução do argumento passado");        // tratamento de erro na tentativa de execução.
        exit(1);       // comando para eu sair desse bloco de tratamento,e nao, nao preciso fazer mais um if, da pra fazer nesse mesmo bloco
    }

    {   //processo pai
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
        if(WSTOPSIG(status) == SIGSTOP) // verificação se o processo foi parado pela chamada do filho de SIGSTOP
        return 0;
    }
    return -1;

    // FEITO

    /*
     * TODO Semana 2:
     *
     * O filho chama raise(SIGSTOP) antes de executar o programa alvo.
     * O pai precisa esperar essa parada inicial com waitpid().
     *
     * Retorne 0 se o filho parou como esperado, -1 em erro.
     */
}

static int configure_trace_options(pid_t child)
{
    //se SIGTRAP for recebido e o bit 0x80 estiver setado, entao é uma parada de syscall
    if (ptrace(PTRACE_SETOPTIONS, child, NULL, PTRACE_O_TRACESYSGOOD) == -1) {
        fprintf(stderr, "erro: TODO Semana 3: implementar configure_trace_options()\n");
        return -1;
    }
    return 0;

    // FEITO
    /*
     * TODO Semana 3:
     *
     * Configure PTRACE_O_TRACESYSGOOD com PTRACE_SETOPTIONS.
     * Isso ajuda a diferenciar paradas de syscall de outros sinais.
     */
}

static int resume_until_next_syscall(pid_t child, int signal_to_deliver)
{
    if (ptrace(PTRACE_SYSCALL, child, NULL, signal_to_deliver) == -1) {
        fprintf(stderr, "erro: TODO Semana 3: implementar resume_until_next_syscall()\n");
        return -1;
    }
    return 0;

    //FEITO
     /*
     * TODO Semana 3:
     *
     * Use ptrace(PTRACE_SYSCALL, ...) para deixar o filho executar ate a
     * proxima entrada ou saida de syscall.
     *
     * signal_to_deliver deve ser repassado como quarto argumento do ptrace.
     */
}


    static int wait_for_syscall_stop(pid_t child, int *status)
    {
        while(1)
    {
        if(waitpid(child,status,0) == -1) // Primeira verificação de erro na espera
        {
            perror("Erro na espera por processo filho com waipid()");
            return -1;
        }


        if(WIFEXITED(*status)) // returns true if the child terminated normally ()
        {
            return 0; // filho terminou sua execução normalmente
        }

        if(WIFSIGNALED(*status))
        {
            return 0; // O filho terminou por conta de um sinal
        }

        if(WIFSTOPPED(*status))
        {
            // processo oficialmente parado
            if(WSTOPSIG(*status) &  0x80)
        {
            return 1;
        }
        resume_until_next_syscall(child,0);
        }
        
    }
}

    /* FEITO
     * TODO Semana 3:
     *
     * Espere o filho com waitpid().
     *
     * Retorne:
     *   1 se a parada foi uma parada de syscall;
     *   0 se o filho terminou normalmente ou por sinal;
     *  -1 em erro.
     *
     * Dicas:
     * - WIFEXITED e WIFSIGNALED indicam fim do processo.
     * - WIFSTOPPED indica que o processo parou.
     * - com PTRACE_O_TRACESYSGOOD, syscall-stops aparecem com bit 0x80.
     * - paradas SIGTRAP comuns nao devem ser entregues de volta ao filho.
     */

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


        /*
         * TODO Semana 4:
         *
         * Use PTRACE_GETREGS para preencher regs.
         * Depois chame fill_event_from_regs() e observer().
         */
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