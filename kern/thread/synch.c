/*
 * Copyright (c) 2000, 2001, 2002, 2003, 2004, 2005, 2008, 2009
 *	The President and Fellows of Harvard College.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE UNIVERSITY AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE UNIVERSITY OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Synchronization primitives.
 * The specifications of the functions are in synch.h.
 */

#include <types.h>
#include <lib.h>
#include <spinlock.h>
#include <wchan.h>
#include <thread.h>
#include <current.h>
#include <synch.h>

////////////////////////////////////////////////////////////
//
// Semaphore.

struct semaphore *
sem_create(const char *name, unsigned initial_count)
{
        struct semaphore *sem;

        sem = kmalloc(sizeof(*sem));
        if (sem == NULL) {
                return NULL;
        }

        sem->sem_name = kstrdup(name);
        if (sem->sem_name == NULL) {
                kfree(sem);
                return NULL;
        }

	sem->sem_wchan = wchan_create(sem->sem_name);
	if (sem->sem_wchan == NULL) {
		kfree(sem->sem_name);
		kfree(sem);
		return NULL;
	}

	spinlock_init(&sem->sem_lock);
        sem->sem_count = initial_count;

        return sem;
}

void
sem_destroy(struct semaphore *sem)
{
        KASSERT(sem != NULL);

	/* wchan_cleanup will assert if anyone's waiting on it */
	spinlock_cleanup(&sem->sem_lock);
	wchan_destroy(sem->sem_wchan);
        kfree(sem->sem_name);
        kfree(sem);
}

void
P(struct semaphore *sem)
{
        KASSERT(sem != NULL);

        /*
         * May not block in an interrupt handler.
         *
         * For robustness, always check, even if we can actually
         * complete the P without blocking.
         */
        KASSERT(curthread->t_in_interrupt == false);

	/* Use the semaphore spinlock to protect the wchan as well. */
	spinlock_acquire(&sem->sem_lock);


        while (sem->sem_count == 0) {
		/*
		 *
		 * Note that we don't maintain strict FIFO ordering of
		 * threads going through the semaphore; that is, we
		 * might "get" it on the first try even if other
		 * threads are waiting. Apparently according to some
		 * textbooks semaphores must for some reason have
		 * strict ordering. Too bad. :-)
		 *
		 * Exercise: how would you implement strict FIFO
		 * ordering?
		 */

	  wchan_sleep(sem->sem_wchan, &sem->sem_lock);
        }

        KASSERT(sem->sem_count > 0);
        sem->sem_count--;
	spinlock_release(&sem->sem_lock);
}

void
V(struct semaphore *sem)
{
        KASSERT(sem != NULL);

	spinlock_acquire(&sem->sem_lock);

        sem->sem_count++;
        KASSERT(sem->sem_count > 0);
	wchan_wakeone(sem->sem_wchan, &sem->sem_lock);

	spinlock_release(&sem->sem_lock);
}

////////////////////////////////////////////////////////////
//
// Lock.
// Mutex is not binary semaphore : thread ownership
/* On Windows, there are two differences between mutexes and binary semaphores: A mutex can only be released by the thread which has ownership, i.e. the thread which previously called the Wait function, (or which took ownership when creating it). A semaphore can be released by any thread. */

// Mutex is not spinlock : sleeping
// http://stackoverflow.com/questions/5869825/when-should-one-use-a-spinlock-instead-of-mutex
/* n theory, when a thread tries to lock a mutex and it does not succeed, because the mutex is already locked, it will go to sleep, immediately allowing another thread to run. It will continue to sleep until being woken up, which will be the case once the mutex is being unlocked by whatever thread was holding the lock before. When a thread tries to lock a spinlock and it does not succeed, it will continuously re-try locking it, until it finally succeeds; thus it will not allow another thread to take its place (however, the operating system will forcefully switch to another thread, once the CPU runtime quantum of the current thread has been exceeded, of course). */

struct lock *
lock_create(const char *name)
{
        struct lock *lock;

        lock = kmalloc(sizeof(*lock));
        if (lock == NULL) {
                return NULL;
        }

        lock->lk_name = kstrdup(name);
        if (lock->lk_name == NULL) {
                kfree(lock);
                return NULL;
        }

	lock->lk_wchan = wchan_create(lock->lk_name);
	if (lock->lk_wchan == NULL) {
		kfree(lock->lk_name);
		kfree(lock);
		return NULL;
	}

        // no thread is holding it
	spinlock_init(&lock->lk_lock);

	lock->lk_thread = NULL;

        return lock;
}

void
lock_destroy(struct lock *lock)
{
        KASSERT(lock != NULL);

	spinlock_cleanup(&lock->lk_lock);

	wchan_destroy(lock->lk_wchan);

        kfree(lock->lk_name);

        kfree(lock);
}

void
lock_acquire(struct lock *lock)
{

  KASSERT(lock != NULL);
  
  KASSERT(curthread->t_in_interrupt == false);

  spinlock_acquire(&lock->lk_lock);

  // While is used instead of If, because
  // we want to check the condition again after
  // the sleeping thread wakes up.
  while(lock->lk_thread != NULL) {
    wchan_sleep(lock->lk_wchan, &lock->lk_lock);
  }

  KASSERT(lock->lk_thread == NULL); 

  lock->lk_thread = curthread;

  spinlock_release(&lock->lk_lock);
}

void
lock_release(struct lock *lock)
{

  
  KASSERT(lock != NULL);

  spinlock_acquire(&lock->lk_lock);

  lock->lk_thread = NULL;

  KASSERT(lock->lk_thread == NULL); 

  wchan_wakeone(lock->lk_wchan, &lock->lk_lock);

  spinlock_release(&lock->lk_lock);

}

bool
lock_do_i_hold(struct lock *lock)
{
  if (!CURCPU_EXISTS()) {
    return true;
  }

  return (lock->lk_thread == curthread);
}

////////////////////////////////////////////////////////////
//
// CV


struct cv *
cv_create(const char *name)
{
        struct cv *cv;

        cv = kmalloc(sizeof(*cv));
        if (cv == NULL) {
                return NULL;
        }

        cv->cv_name = kstrdup(name);
        if (cv->cv_name==NULL) {
                kfree(cv);
                return NULL;
        }

	cv->lockCount = 0;
	
        return cv;
}

void
cv_destroy(struct cv *cv)
{
        KASSERT(cv != NULL);

        kfree(cv->cv_name);
        kfree(cv);

}

void
cv_wait(struct cv *cv, struct lock *lock)
{
  
        KASSERT(cv != NULL);
	KASSERT(lock != NULL);

        // Release lock

	lock_release(lock);

	spinlock_acquire(&lock->lk_lock);

	cv->locks[cv->lockCount] = lock;

	cv->lockCount++;

	wchan_sleep(lock->lk_wchan, &lock->lk_lock);
	
	KASSERT(cv->lockCount > 0 );

	cv->lockCount--;

	spinlock_release(&lock->lk_lock);

	lock_acquire(lock);
}


void
cv_signal(struct cv *cv, struct lock *lock)
{
	KASSERT(cv != NULL);
	KASSERT(lock != NULL);

	unsigned int lastLockIndex;
	spinlock_acquire(&lock->lk_lock);
	if (cv->lockCount > 0) {
	  lastLockIndex = cv->lockCount - 1;
	  wchan_wakeone(cv->locks[lastLockIndex]->lk_wchan,
			&cv->locks[lastLockIndex]->lk_lock);
	}

	spinlock_release(&lock->lk_lock);

}

void
cv_broadcast(struct cv *cv, struct lock *lock)
{
	KASSERT(cv != NULL);
	KASSERT(lock != NULL);

	spinlock_acquire(&lock->lk_lock);
	
	for(int i=0; i< cv->lockCount; i++){
	  wchan_wakeone(cv->locks[i]->lk_wchan, &cv->locks[i]->lk_lock);
	}

	spinlock_release(&lock->lk_lock);
	

}
