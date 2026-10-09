#pragma once

#include <stdatomic.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>

#include "log.h"

typedef struct
{
	atomic_flag flag;
} spinlock_t;
typedef struct
{
	atomic_flag flag;
} spinlock_noint_t;

extern uint32_t cpu_eflags;

#define SPINLOCK_INIT         ((spinlock_t){ .flag = ATOMIC_FLAG_INIT })
#define SPINLOCK_NOINT_INIT   ((spinlock_noint_t){ .flag = ATOMIC_FLAG_INIT })

static inline __attribute__((always_inline)) bool try_acquire_spinlock(spinlock_t* spinlock)
{
	return atomic_flag_test_and_set_explicit(&spinlock->flag, memory_order_acquire);
}

static inline __attribute__((always_inline)) void acquire_spinlock(spinlock_t* spinlock)
{
	while (try_acquire_spinlock(spinlock))
	{
		LOG(ERROR, "DEADLOCK");
		abort();
        __builtin_ia32_pause();
	}
}

static inline __attribute__((always_inline)) void release_spinlock(spinlock_t* spinlock)
{
	atomic_flag_clear_explicit(&spinlock->flag, memory_order_release);
}

static inline __attribute__((always_inline)) uint32_t acquire_spinlock_noint(spinlock_noint_t* spinlock)
{
	acquire_spinlock((spinlock_t*)spinlock);
	cpu_eflags = 0;
	return 0;
}

static inline __attribute__((always_inline)) void release_spinlock_noint(spinlock_noint_t* spinlock, uint32_t flags)
{
	(void)flags;
	release_spinlock((spinlock_t*)spinlock);
	cpu_eflags = 1;
}
