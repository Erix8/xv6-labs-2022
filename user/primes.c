#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

void 
sieve(int *in_pipe)
{
  int p;
  close(in_pipe[1]);

  if(read(in_pipe[0], &p, sizeof(int)) <= 0){
    close(in_pipe[0]);
    exit(0);
  }
  printf("prime %d\n", p);

  int out_pipe[2];
  pipe(out_pipe);

  if(fork() == 0){
    close(out_pipe[1]);
    sieve(out_pipe);
    exit(0);
  } else {
    close(out_pipe[0]);
    int num;
    while(read(in_pipe[0], &num, sizeof(int)) > 0){
      if(num % p != 0){
        write(out_pipe[1], &num, sizeof(int));
      }
    }
    close(in_pipe[0]);
    close(out_pipe[1]);
    wait(0);
  }
}

int
main(int argc, char *argv[])
{
  int p[2];

  if(argc > 1){
    fprintf(2, "Usage: primes\n");
  }

  pipe(p);

  if(fork() == 0){
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
