#if !defined MYSH_COMMON_H
#define MYSH_COMMON_H

// --------------------------------------------------------------------------
int do_interactive_loop(int server_fd);
int do_non_interactive_loop(int client_fd, int log_fd);

#endif // !defined MYSH_COMMON_H

