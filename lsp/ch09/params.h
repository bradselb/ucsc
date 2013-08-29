#if !defined PARAMETERS_H
#define PARAMETERS_H

struct params;

struct params* alloc_params(void);
int extract_params_from_cmdline_options(struct params*, int argc, char* argv[]);
void free_params(struct params*);

int use_pipe(struct params*);
int use_sysv_ipc(struct params*);
int is_help_desired(struct params*);

const char* log_file_name(struct params*);

// this is here because the help message is all about the parameters.
// clean code pedantics would say it belongs...not here.
void show_help(const char* argv0);

#endif //!defined PARAMETERS_H

