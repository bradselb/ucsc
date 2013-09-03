#if !defined MYSH_COMMON_H
#define MYSH_COMMON_H



// --------------------------------------------------------------------------
int do_interactive_loop(int server_fd);
int do_non_interactive_loop(int client_fd, int log_fd);

int do_cmd(char* buf, int len, int fd);
int parse_cmd(char* buf, char** argv);
int do_built_in_cmd(int argc, char** argv, int fd);
int do_external_cmd(char** argv, int fd);


// each of these is contained in a separate file. 
int cat(int argc, char** argv, int fd);
int wc(int argc, char** argv, int fd);
int ls(int argc, char** argv, int fd);

#endif // !defined MYSH_COMMON_H

