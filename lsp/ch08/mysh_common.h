#if !defined MYSH_COMMON_H
#define MYSH_COMMON_H


int do_cmd(char* buf, int len);
int parse_cmd(char* buf, char** argv);
int do_built_in_cmd(int argc, char** argv);
int do_external_cmd(char** argv);


int cat(int argc, char** argv);
int wc(int argc, char** argv);
int ls(int argc, char** argv);

#endif // !defined MYSH_COMMON_H

