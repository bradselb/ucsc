#if !defined MYSH_PIPE_H
#define MYSH_PIPE_H


int mysh_implemented_with_pipe(void);
int do_interactive_loop(int write_fd, int read_fd);
int do_non_interactive_loop(int read_fd, int write_fd);



#endif // !defined MYSH_PIPE_H

