#include <linux/spinlock.h>

static int bks_open_count; // how many times has open() been called?
static spinlock_t bks_open_count_lock = SPIN_LOCK_UNLOCKED;

void bks_incrOpenCount(void)
{
	spin_lock(&bks_open_count_lock);
	++bks_open_count;
	spin_unlock(&bks_open_count_lock);
}

int bks_getOpenCount(void)
{
	int count = bks_open_count;
	spin_lock(&bks_open_count_lock);
	count = bks_open_count;
	spin_unlock(&bks_open_count_lock);
	return count;
}

