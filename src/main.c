#include <stdlib.h> // exit
#include <stdio.h>
#include <unistd.h>
#include <string.h> // strerror
#include <sys/errno.h>
#include <sys/wait.h>
#include <stdbool.h>

// exit codes
static const int EXIT_CODE_OK              = 0;
static const int EXIT_CODE_UNKNOWN_ERROR   = 1;
static const int EXIT_CODE_FORK_FAILURE    = 2;
static const int EXIT_CODE_EXEC_FAILURE    = 3;
static const int EXIT_CODE_WAIT_FAILURE    = 4;
static const int EXIT_CODE_INCORRECT_USAGE = 5;
static const int EXIT_CODE_CHILD_LOST      = 6;

static int spawnAndAwaitTermination(const char* const* argv) {
    int exitCode = EXIT_CODE_UNKNOWN_ERROR;

    pid_t child = fork();

    if (child < 0) {
        fprintf(stderr, "Unable to fork; errno=%d (%s)\n", errno, strerror(errno));
        exitCode = EXIT_CODE_FORK_FAILURE;
    } else if (child == 0) {
        // Er, we are the child.
        // NOTE: execvp is a wrapper around execve that:
        //  (1) looks up the given program in $PATH
        //  (2) copies our environment to the child
        // NOTE: dropping the const- qualifiers from argv is legal here.  If
        // the call to execvp() modifies them, then they have only been
        // modified in this child process.  They have not been modified in the
        // parent process.
        int execRes = execvp(argv[0], (char**)argv);
        if (execRes < 0) {
            fprintf(stderr, "Unable to exec; errno=%d (%s)\n", errno, strerror(errno));
            exitCode = EXIT_CODE_EXEC_FAILURE;
        }

        // The child should not keep running if exec fails.  This ensures that
        // control flow meant for the parent process (i.e. what happens when
        // this procedure returns) does not affect the child's behavior.
        exit(exitCode);
    } else {
        // We are the parent.

        // NOTE: The POSIX standard [1] says:
        //  > blksize_t, pid_t, and ssize_t shall be signed integer types [...]
        //  > the widths of blksize_t, pid_t, size_t, ssize_t, suseconds_t, and
        //  > useconds_t are no greater than the width of type long
        // So the cast to long here will never overflow.
        // [1]: https://pubs.opengroup.org/onlinepubs/009695399/basedefs/sys/types.h.html
        fprintf(stderr, "Child is pid=%ld\n", (long)child);

        exitCode = EXIT_CODE_WAIT_FAILURE;
        int childStatus;
        for (;;) {
            int waitRes = waitpid(child, &childStatus, 0);
            // "If wait3(), wait4(), or waitpid() returns due to a
            // stopped or terminated child process, the process ID of
            // the child is returned"
            // "The The wait3() and waitpid() calls will fail and return
            // immediately if:
            //     [ECHILD] The process specified by pid does not exist
            //              or is not a child of the calling process"
            if (waitRes == child) {
                bool terminatedViaExit   = WIFEXITED(childStatus);
                bool terminatedViaSignal = WIFSIGNALED(childStatus);
                bool currentlyStopped    = WIFSTOPPED(childStatus);
                if (terminatedViaExit) {
                    fprintf(stderr, "Child exited (ret=%d)\n", WEXITSTATUS(childStatus));
                } else if (terminatedViaSignal) {
                    fprintf(stderr, "Child died to signal %d\n", WTERMSIG(childStatus));
                } else if (currentlyStopped) {
                    // NOTE from waitpid manual page:
                    //  > This macro [WIFSTOPPED] can be true only if the wait
                    //  > call specified the WUNTRACED option or if the child
                    //  > process is being traced (see ptrace(2)).
                    // I expect this condition to be vanishingly rare, but I
                    // think it can happen if someone attaches a debugger to
                    // the child process.  Right now this program busy-loops
                    // while the child is stopped... perhaps that behavior can
                    // be improved?
                    fprintf(stderr, "Child is stopped\n");
                } else {
                    fprintf(stderr, "Child exited for an unknown reason\n");
                }
                exitCode = EXIT_CODE_OK;
                break;
            } else if (waitRes < 0 && errno == ECHILD) {
                fprintf(stderr, "Oops, lost track of the child process!\n");
                exitCode = EXIT_CODE_CHILD_LOST;
                break;
            }
        }
    }

    return exitCode;
}

int main(int argc, const char* const* argv) {
    int exitCode = EXIT_CODE_UNKNOWN_ERROR;

    if (argc >= 2) {
        do {
            exitCode = spawnAndAwaitTermination(argv + 1);
        } while (exitCode == EXIT_CODE_OK);
    } else {
        fprintf(stderr, "Usage: %s program [program_args...]\n", argv[0]);
        exitCode = EXIT_CODE_INCORRECT_USAGE;
    }

    if (exitCode == EXIT_CODE_UNKNOWN_ERROR) {
        fprintf(stderr, "An unknown error happened, indicating a bug in this program (%s).\n", argv[0]);
    }

    return exitCode;
}
