#include "student_api.h"
#include <string.h>
#include <stdio.h>
#include <sys/syscall.h>
#include <trace_helpers.h>
#include <stdint.h>

int student_pair_syscall(struct syscall_pairer *pairer,
                         const struct syscall_event *ev,
                         struct syscall_event *out)
{

    if (ev->entering) {
        pairer->entry = *ev;
        pairer->has_entry = 1;
        return 0;
    }

    if (pairer->has_entry) {
        *out = pairer->entry;
        out->ret = ev->ret;
        out->entering = ev->entering;
        pairer->has_entry = 0;
        return 1;
    }

    *out = *ev;
    return 1;
}
