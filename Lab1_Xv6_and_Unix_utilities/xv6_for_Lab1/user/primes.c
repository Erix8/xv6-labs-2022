#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

void sieve(int *left_pipe)
{
    int p;
    close(left_pipe[1]);

    if (read(left_pipe[0], &p, sizeof(int)) <= 0) {
        close(left_pipe[0]);
        exit(0);
    }
    printf("prime %d\n", p);

    int right_pipe[2];
    pipe(right_pipe);

    if (fork() == 0) {
        close(right_pipe[1]);
        sieve(right_pipe);
        exit(0);
    } else {
        close(right_pipe[0]);
        int num;
        while (read(left_pipe[0], &num, sizeof(int)) > 0) {
            if (num % p != 0) {
                write(right_pipe[1], &num, sizeof(int));
            }
        }
        close(left_pipe[0]);
        close(right_pipe[1]);
        wait(0);
    }
}

int
main(int argc, char *argv[])
{
    int p[2];
    pipe(p);

    if (fork() == 0) {
        close(p[0]);
        for (int i = 2; i <= 35; i++) {
            write(p[1], &i, sizeof(int));
        }
        close(p[1]);
        exit(0);
    } else {
        close(p[1]);
        sieve(p);
        close(p[0]);
        wait(0);
    }

    exit(0);
}