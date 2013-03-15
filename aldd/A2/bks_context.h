#ifndef BKS_CONTEXT_H
#define BKS_CONTEXT_H

struct bks_context;

struct bks_context* bks_create_context(void);
void bks_free_context(struct bks_context* ctx);

int bks_get_freq(struct bks_context* ctx);
int bks_get_state(struct bks_context* ctx);
int bks_get_pattern(struct bks_context* ctx);

int bks_set_freq(struct bks_context* ctx, int freq);
int bks_set_state(struct bks_context* ctx, int state);
int bks_set_pattern(struct bks_context* ctx, int pattern);

#endif //ndef BKS_CONTEXT_H
