#if !defined PARAMETERS_H
#define PARAMETERS_H

struct params;

struct params* alloc_params(void);
int initialize_params(int argc, char* argv[], struct params*);
void free_params(struct params*);

int use_pipe(struct params*);
int use_sysv_ipc(struct params*);
int is_help_desired(struct params*);
char* config_file_name(struct params*);
char* log_file_name(struct params*);
char* pid_file_name(struct params*);




#endif //!defined PARAMETERS_H
