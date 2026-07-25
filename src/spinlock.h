#include <stdatomic.h>
#include <stdbool.h>

static inline __attribute__((always_inline)) bool try_acquire_spinlock(atomic_flag* spinlock)
{
	  return atomic_flag_test_and_set_explicit(spinlock, memory_order_acquire);
}

static inline __attribute__((always_inline)) void acquire_spinlock(atomic_flag* spinlock)
{
	  while (try_acquire_spinlock(spinlock))
        __builtin_ia32_pause();
}

static inline __attribute__((always_inline)) void release_spinlock(atomic_flag* spinlock)
{
	  atomic_flag_clear_explicit(spinlock, memory_order_release);
}

static inline __attribute__((always_inline)) uint32_t acquire_spinlock_noint(atomic_flag* spinlock)
{
	  acquire_spinlock(spinlock);
	  return 0;
}

static inline __attribute__((always_inline)) void release_spinlock_noint(atomic_flag* spinlock, uint32_t flags)
{
		(void)flags;
	  release_spinlock(spinlock);
}
