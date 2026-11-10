#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  int p1[2], p2[2];  
  char byte;

  if(argc > 1){
    fprintf(2, "Usage: pingpong\n");
    exit(1);
  }

  pipe(p1); // Parent -> child pipe.
  pipe(p2); // Child -> parent pipe.

  if(fork() == 0){
    close(p1[1]);
    close(p2[0]);

    read(p1[0], &byte, 1);
    printf("%d: received ping\n", getpid());
    write(p2[1], &byte, 1);

    close(p1[0]);
    close(p2[1]);

    exit(0);
  } else {
    close(p1[0]);
    close(p2[1]);

    write(p1[1], "x", 1);
    read(p2[0], &byte, 1);
    printf("%d: received pong\n", getpid());

    close(p1[1]);
    close(p2[0]);

    wait(0);
    exit(0);
  }
}
