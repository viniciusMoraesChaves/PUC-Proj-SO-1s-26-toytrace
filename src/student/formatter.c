#include "student_api.h"

#include "syscall_names.h"
#include <string.h>
#include <stdio.h>
#include <sys/syscall.h>
#include <trace_helpers.h>
#include <stdint.h>


void student_debug_raw_event(const struct syscall_event *ev,
                             char *buf,
                             size_t bufsz)
{
     snprintf(buf, bufsz, "pid=%d %s %s",
             ev->pid,
             syscall_name(ev->syscall_no),
             ev->entering ? "entrada" : "saida");
}

void student_format_event(const struct syscall_event *ev,
                          char *buf,
                          size_t bufsz)
{
    switch(ev->syscall_no) 
    {
        case SYS_read: {
        char armz_read[4096] = {0};
        if(read_child_string(ev->pid, ev->args[1], armz_read, sizeof(armz_read)) >= 0) {
               snprintf(buf, bufsz, "read(%ld, \"%s\", %lu) = %ld",
                    ev->args[0],
                    armz_read,
                    ev->args[2],
                    ev->ret);
            break;
            }
            snprintf(buf, bufsz, "read(%ld, <ilegivel>, %lu) = %ld",
                    ev->args[0],
                    ev->args[2],
                    ev->ret);
            break;

        }

        case SYS_write: {
        char armz_write[4096] = {0};
         if(read_child_string(ev->pid, ev->args[1], armz_write, sizeof(armz_write)) >= 0) {
                    snprintf(buf, bufsz, "write(%ld, \"%s\", %lu) = %ld",
                    ev->args[0],
                    armz_write,
                    ev->args[2],
                    ev->ret);
            break;
            }
            snprintf(buf, bufsz, "write(%ld, <ilegivel>, %lu) = %ld",
                    ev->args[0],
                    ev->args[2],
                    ev->ret);
            break;

        }

        case SYS_openat: {
            char path[4096] = {0};
            if(read_child_string(ev->pid, ev->args[1], path, sizeof(path)) >= 0) {
                snprintf(buf, bufsz, "openat(%ld, \"%s\", %#lx, %#lx) = %ld",
                    ev->args[0],
                    path,
                    ev->args[2],
                    ev->args[3],
                    ev->ret);
            break;
            }

            snprintf(buf, bufsz, "openat(%ld, <ilegivel>, %#lx, %#lx) = %ld",
                    ev->args[0],
                    ev->args[2],
                    ev->args[3],
                    ev->ret);
            break;
        }

        case SYS_execve: 
        {
                const char *path_execve = (const char *)ev->args[0];
                snprintf(buf, bufsz, "execve(\"%s\",...) = %ld", path_execve, ev->ret);
                 break;
        }

        case SYS_exit_group: {
            snprintf(buf, bufsz, "exit_group(%ld) = %ld",ev->args[0],ev->ret);
            break;
        }

        default:
            snprintf(buf, bufsz, "%s(%#lx, %#lx, %#lx, %#lx, %#lx, %#lx) = %ld",
            syscall_name(ev->syscall_no),
            ev->args[0],
            ev->args[1],
            ev->args[2],
            ev->args[3],
            ev->args[4],
            ev->args[5],
            ev->ret);
        break;
    }
}


