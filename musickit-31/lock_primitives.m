/* Modifcation history:

   daj/july 25, 1990 - Created file and changed extern to static.

 */
/*
 * rec_mutex.c
 *
 * Mutex locks which allow recursive locks and unlocks by the same thread.
 *
 * Michael B. Jones
 *
 * 24-Jun-1987
 */

/*
 * HISTORY:
 * $Log:	rec_mutex.c,v $
 * Revision 2.1.1.2  89/07/20  17:20:20  mbj
 * 	13-Dec-88 Mary Thompson (mrt) @ Carnegie Mellon
 * 	Changed string_t to char * as string_t is no
 * 	longer defined in cthreads.h
 * 
 * Revision 2.1.1.1  89/07/20  17:18:06  mbj
 * 	Check parallel libc and file mapping changes into source tree branch.
 * 
 * 24-Jun-87  Michael Jones (mbj) at Carnegie-Mellon University
 *	Started from scratch.
 */

#include "lock_primitives.h"

#ifndef NO_CTHREAD
#define	NO_CTHREAD	((cthread_t) 0)
#endif	NO_CTHREAD

static rec_mutex_t
rec_mutex_alloc()
{
    register rec_mutex_t m;

    m = (rec_mutex_t) malloc(sizeof(struct rec_mutex));
    rec_mutex_init(m);
    return m;
}

static void
rec_mutex_init(m)
    rec_mutex_t m;
{
    m->thread = NO_CTHREAD;
    m->count = 0;
    mutex_init(& m->cthread_mutex);
}

#if 0
static void
rec_mutex_set_name(m, name)
    rec_mutex_t m; char * name;
{
    mutex_set_name(& m->cthread_mutex, name);
}

static char *
rec_mutex_name(m)
    rec_mutex_t m;
{
    return mutex_name(& m->cthread_mutex);
}

static void
rec_mutex_clear(m)
    rec_mutex_t m;
{
    m->thread = NO_CTHREAD;
    m->count = 0;
    mutex_clear(& m->cthread_mutex);
}

static void
rec_mutex_free(m)
    rec_mutex_t m;
{
    rec_mutex_clear(m);
    free((char *) m);
}
#endif

static int
rec_mutex_try_lock(m)
    rec_mutex_t m;
{
    cthread_t self = cthread_self();

    ASSERT(self != NO_CTHREAD);
    if (m->thread == self) {	/* If already holding lock */
	m->count += 1;
	return TRUE;
    }
    if (mutex_try_lock(& m->cthread_mutex)) {	/* If can acquire lock */
	ASSERT(m->count == 0);
	ASSERT(m->thread == NO_CTHREAD);
	m->count = 1;
	m->thread = self;
	return TRUE;
    }
    return FALSE;
}

static void
rec_mutex_lock(m)
    rec_mutex_t m;
{
    cthread_t self = cthread_self();

    ASSERT(self != NO_CTHREAD);
    if (m->thread == self) {	/* If already holding lock */
	m->count += 1;
    } else {
	mutex_lock(& m->cthread_mutex);
	ASSERT(m->count == 0);
	ASSERT(m->thread == NO_CTHREAD);
	m->count = 1;
	m->thread = self;
    }
}

static void
rec_mutex_unlock(m)
    rec_mutex_t m;
{
    if (m->thread == cthread_self()) {	/* Must be holding lock to unlock! */
	if (--(m->count) == 0) {
	    m->thread = NO_CTHREAD;
	    mutex_unlock(& m->cthread_mutex);
	}
    }
}

