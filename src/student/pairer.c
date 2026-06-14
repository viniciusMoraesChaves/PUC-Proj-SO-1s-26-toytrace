#include "student_api.h"
#include <string.h>
#include <stdio.h>
#include <sys/syscall.h>
#include <trace_helpers.h>

int student_pair_syscall(struct syscall_pairer *pairer,
                         const struct syscall_event *ev,
                         struct syscall_event *out)
{
    if (ev->entering) {
        if(pairer->has_entry){
            return -1;
        }
    pairer->entry = *ev;
    pairer->has_entry = 1;

    if (ev->syscall_no == SYS_execve) {
        static char execve_path[4096] = {0};
        if (read_child_string(ev->pid, ev->args[0],
                              execve_path, sizeof(execve_path)) < 0) {
            strncpy(execve_path, "<ilegivel>", sizeof(execve_path));
        }
        pairer->entry.args[0] = (unsigned long)execve_path;
    }
    return 0;
}

    if (pairer->has_entry) {
        *out = pairer->entry;
        out->ret = ev->ret;
        out->entering = ev->entering;
        pairer->has_entry = 0;
        return 1;
    }
    return -1;
}
