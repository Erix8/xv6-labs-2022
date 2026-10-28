#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/param.h"
#include "user/user.h"

// Run the command with the extra arguments in new_argv
// (new_argv[cmd_argc .. cmd_argc+token_count-1] hold the extra args).
void
run_cmd(char *new_argv[], int cmd_argc, char *buf)
{
  int start, end;
  int token_count;

  token_count = 0;
  start = 0;
  while (buf[start] != '\0') {
    // Skip leading whitespace (space or tab)
    while (buf[start] == ' ' || buf[start] == '\t')
      start++;

    if (buf[start] == '\0')
      break;  // No more tokens

    // Found the start of a token
    new_argv[cmd_argc + token_count] = &buf[start];
    token_count++;

    if (cmd_argc + token_count >= MAXARG - 1)
      break;  // Too many arguments

    // Find the end of this token
    end = start;
    while (buf[end] != '\0' && buf[end] != ' ' && buf[end] != '\t')
      end++;

    if (buf[end] == '\0')
      break;  // Token reaches end of line

    buf[end] = '\0';  // Terminate the token
    start = end + 1;
  }

  new_argv[cmd_argc + token_count] = 0;  // NULL-terminate argv

  if (fork() == 0) {
    // Child: execute the command
    exec(new_argv[0], new_argv);
    fprintf(2, "xargs: exec %s failed\n", new_argv[0]);
    exit(1);
  } else {
    // Parent: wait for child to finish
    wait(0);
  }
}

int
main(int argc, char *argv[])
{
  char *new_argv[MAXARG];
  char buf[512];
  int cmd_argc;
  int i, n;
  char *p;
  char ch;

  if (argc < 2) {
    fprintf(2, "usage: xargs command [args...]\n");
    exit(1);
  }

  // Copy the base command arguments (argv[1] through argv[argc-1])
  cmd_argc = argc - 1;
  for (i = 0; i < cmd_argc; i++) {
    new_argv[i] = argv[i + 1];
  }

  p = buf;
  while (1) {
    n = read(0, &ch, 1);
    if (n <= 0) {
      break;  // EOF or error
    }

    if (ch == '\n') {
      // End of one line: execute the command with this line's words as extra args
      *p = '\0';  // null-terminate the line

      // Skip empty lines
      if (p != buf) {
        run_cmd(new_argv, cmd_argc, buf);
      }

      // Reset buffer pointer for next line
      p = buf;
    } else {
      // Store the character in the buffer
      *p = ch;
      p++;
      // Prevent buffer overflow
      if (p >= buf + sizeof(buf) - 1) {
        p = buf + sizeof(buf) - 1;
      }
    }
  }

  // Handle the case where the last line has no trailing newline
  if (p != buf) {
    *p = '\0';
    run_cmd(new_argv, cmd_argc, buf);
  }

  exit(0);
}