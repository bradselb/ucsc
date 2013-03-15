#include <linux/slab.h> // kmalloc()

#include "bks_context.h"

// -------------------------------------------------------------------------
struct bks_context {
    rwlock_t lock; // protect access to this struct
    int freq; // the blink frequency in Hz.
    int state; // current state of key board LEDS.
    int pattern; // the blink pattern code
};

// -------------------------------------------------------------------------
struct bks_context* bks_create_context(void)
{
    struct bks_context* ctx = 0;
    ctx = kmalloc(sizeof(struct bks_context), GFP_KERNEL);

    if (!ctx) {
        ; // not much we can do here.
    } else {
        memset(ctx, 0, sizeof *ctx);
        rwlock_init(&ctx->lock);
        ctx->freq = 1; // Hz
        ctx->state = 0;
        ctx->pattern = 8;
    }

    return ctx;
}

// -------------------------------------------------------------------------
void bks_free_context(struct bks_context* ctx)
{
    if (ctx) {
        kfree(ctx);
    }
}

// -------------------------------------------------------------------------
int bks_get_freq(struct bks_context* ctx)
{
    int rc = 0;
    if (!ctx) {
        rc = -1;
    } else {
        unsigned long flags;
        read_lock_irqsave(&ctx->lock, flags);
        rc = ctx->freq;
        read_unlock_irqrestore(&ctx->lock, flags);
    }

    return rc;
}


// -------------------------------------------------------------------------
int bks_get_state(struct bks_context* ctx)
{
    int rc = 0;
    if (!ctx) {
        rc = -1;
    } else {
        unsigned long flags;
        read_lock_irqsave(&ctx->lock, flags);
        rc = ctx->state;
        read_unlock_irqrestore(&ctx->lock, flags);
    }

    return rc;
}


// -------------------------------------------------------------------------
int bks_get_pattern(struct bks_context* ctx)
{
    int rc = 0;
    if (!ctx) {
        rc = -1;
    } else {
        unsigned long flags;
        read_lock_irqsave(&ctx->lock, flags);
        rc = ctx->pattern;
        read_unlock_irqrestore(&ctx->lock, flags);
    }

    return rc;
}


// -------------------------------------------------------------------------
int bks_set_freq(struct bks_context* ctx, int freq)
{
    int rc = 0;
    if (!ctx) {
        rc = -1;
    } else {
        unsigned long flags;
        write_lock_irqsave(&ctx->lock, flags);
        ctx->freq = freq;
        write_unlock_irqrestore(&ctx->lock, flags);
    }

    return rc;
}


// -------------------------------------------------------------------------
int bks_set_state(struct bks_context* ctx, int state)
{
    int rc = 0;
    if (!ctx) {
        rc = -1;
    } else {
        unsigned long flags;
        write_lock_irqsave(&ctx->lock, flags);
        ctx->state = (state & 0x0f);
        write_unlock_irqrestore(&ctx->lock, flags);
    }

    return rc;
}


// -------------------------------------------------------------------------
int bks_set_pattern(struct bks_context* ctx, int pattern)
{
    int rc = 0;
    if (!ctx) {
        rc = -1;
    } else {
        unsigned long flags;
        write_lock_irqsave(&ctx->lock, flags);
        ctx->pattern = pattern;
        write_unlock_irqrestore(&ctx->lock, flags);
    }

    return rc;
}


