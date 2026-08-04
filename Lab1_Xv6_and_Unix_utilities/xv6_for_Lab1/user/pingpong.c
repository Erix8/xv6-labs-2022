#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
    int p1[2];  // parent -> child
    int p2[2];  // child -> parent
    char byte;

    pipe(p1);
    pipe(p2);

    if (fork() == 0) {
        // child process
        close(p1[1]);  // close write end of parent->child pipe
        close(p2[0]);  // close read end of child->parent pipe

        read(p1[0], &byte, 1);
        printf("%d: received ping\n", getpid());
        write(p2[1], &byte, 1);

        close(p1[0]);
        close(p2[1]);
        exit(0);
    } else {
        // parent process
        close(p1[0]);  // close read end of parent->child pipe
        close(p2[1]);  // close write end of child->parent pipe

        write(p1[1], "x", 1);
        read(p2[0], &byte, 1);
        printf("%d: received pong\n", getpid());

        close(p1[1]);
        close(p2[0]);
        wait(0);
        exit(0);
    }
}
