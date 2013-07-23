#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <fcntl.h>
#include <time.h>
#include <sys/time.h> /* gettimeofday() */
#include <limits.h>  /* PIPE_BUF */

#include "pipe.h"
#include "lprintf.h"

/*
 * The following structure encapsulates one end of a pipe.
 */
typedef struct {
  pthread_cond_t retry;     /* Signalled when blocked I/O should be retried */
  int retry_ready;          /* True when 'retry' is initialized */
  int fd;                   /* The file descriptor of one end of the pipe */
} PipeFd;

struct Pipe {
  pthread_mutex_t guard;  /* The mutual exclusion guard of the pipe */
  int guard_ready;        /* True when guard has been allocated */
  PipeFd read;            /* The readable end of the pipe */
  PipeFd write;           /* The writable end of the pipe */
/*
 * One can write up to PIPE_BUF bytes atomically, but there is no such
 * guarantee for read(). The following buffer allows this guarantee to
 * be extended to reads. It accumulates data sequentially from one
 * or more incomplete reads, and hands out request-sized chunks to
 * any readers that request <= nread bytes, on a first come first
 * served basis.
 */
  char unread[PIPE_BUF];  /* The initial bytes of an incomplete read */
  size_t nread;            /* The number of bytes in buffer[] */
};

static void init_PipeFd(PipeFd *pfd);
static int fill_PipeFd(PipeFd *pfd);
static void zap_PipeFd(PipeFd *pfd);
static int pipe_gettimeofday(struct timespec *ts);

/*.......................................................................
 * Create an encapsulated unnamed pipe for use in sending messages between
 * Pthreads threads.
 *
 * Output:
 *  return  Pipe *  The new object, or NULL on error.
 */
Pipe *new_Pipe(void)
{
  int status;  /* The return status of pthreads functions. */
  Pipe *p;     /* The object to be returned */
  int fds[2];  /* The read and write fds of the pipe */
/*
 * Allocate the container.
 */
  p = (Pipe* )malloc(sizeof(Pipe));
  if(!p) {
    lprintf(stderr, "new_Pipe: Insufficient memory.\n");
    return NULL;
  };
/*
 * Before attempting any operation that might fail, initialize the
 * container at least up to the point at which it can safely be passed
 * to del_Pipe().
 */
  p->guard_ready = 0;
  init_PipeFd(&p->read);
  init_PipeFd(&p->write);
  p->unread[0] = '\0';
  p->nread = 0;
/*
 * Allocate the guard mutex.
 */
  status = pthread_mutex_init(&p->guard, NULL);
  if(status) {
    lprintf(stderr, "new_Pipe: pthread_mutex_init -> %s\n", strerror(status));
    return del_Pipe(p);
  };
  p->guard_ready = 1;
/*
 * Create the pipe.
 */
  if(pipe(fds)) {
    lprintf(stderr, "new_Pipe: pipe() -> %s\n", strerror(errno));
    return del_Pipe(p);
  };
/*
 * Record the file descriptors of the two ends, so that henceforth
 * calls to del_PipeFd() will cause them to be closed.
 */
  p->read.fd = fds[0];
  p->write.fd = fds[1];
/*
 * Allocate the contents of the objects that represent the
 * two ends of the pipe.
 */
  if(fill_PipeFd(&p->read) ||
     fill_PipeFd(&p->write))
    return del_Pipe(p);
  return p;
}

/*.......................................................................
 * Initialize a newly allocated pipe-fd container. This doesn't involve
 * allocating its contents.
 *
 * Input:
 *  pfd   PipeFd *   The object to be initialized.
 */
static void init_PipeFd(PipeFd *pfd)
{
  pfd->retry_ready = 0;
  pfd->fd = -1;
}

/*.......................................................................
 * This is a private function of new_Pipe(), used to allocate the contents
 * of a PipeFd container. Note that pfd->fd must already have been
 * initialized with the fd of the appropriate end of the pipe.
 *
 * Input:
 *  pfd   PipeFd *  The object who's contents are to be allocated.
 * Output:
 *  return   int    0 - OK.
 *                  1 - Error.
 */
static int fill_PipeFd(PipeFd *pfd)
{
  int status;  /* The return status of pthreads functions. */
  int flags;   /* The fcntl flags of pfd->fd */
/*
 * Allocate the retry-I/O condition variable.
 */
  status = pthread_cond_init(&pfd->retry, NULL);
  if(status) {
    lprintf(stderr, "new_Pipe: pthread_cond_init -> %s\n", strerror(status));
    return 1;
  };
  pfd->retry_ready = 1;
/*
 * Prepare the fd for non-blocking I/O. First get the current
 * fcntl flags.
 */
  flags = fcntl(pfd->fd, F_GETFL, 0);
  if(flags < 0) {
    lprintf(stderr, "new_Pipe: fcntl(F_GETFL) -> %s\n", strerror(status));
    return 1;
  };
/*
 * Now add in the non-blocking I/O bit.
 */
#ifdef O_NONBLOCK
  flags |= O_NONBLOCK;  /* POSIX.1 */
#elif defined(O_NDELAY)
  flags |= O_NDELAY;    /* System V */
#elif defined(FNDELAY)
  flags |= FNDELAY;     /* BSD */
#else
#error "new_Pipe: Non-blocking I/O doesn't appear to be supported."
#endif
/*
 * Install the new flags.
 */
  if(fcntl(pfd->fd, F_SETFL, flags) < 0) {
    lprintf(stderr, "new_Pipe: Unable to select non-blocking I/O.\n");
    return 1;
  };
  return 0;
}

/*.......................................................................
 * Delete a Pipe object.
 *
 * Input:
 *  pipe   Pipe *  The object to be deleted.
 * Output:
 *  return Pipe *  The deleted object (always NULL).
 */
Pipe *del_Pipe(Pipe *pipe)
{
  if(pipe) {
    if(pipe->guard_ready) {
      pthread_mutex_destroy(&pipe->guard);
      pipe->guard_ready = 0;
    };
    zap_PipeFd(&pipe->read);
    zap_PipeFd(&pipe->write);
    free(pipe);
  };
  return NULL;
}

/*.......................................................................
 * This is a private function of del_Pipe(), used to delete the contents
 * of a PipeFd object.
 *
 * Input:
 *  pfd      PipeFd *  The object who's contents are to be deleted.
 */
static void zap_PipeFd(PipeFd *pfd)
{
  if(pfd->retry_ready) {
    pthread_cond_destroy(&pfd->retry);
    pfd->retry_ready = 0;
  };
  if(pfd->fd >= 0) {
    close(pfd->fd);
    pfd->fd = -1;
  };
}

/*.......................................................................
 * Read from a pipe using blocking or non-blocking I/O.
 *
 * Input:
 *  pipe         Pipe *  The pipe to read from.
 *  buffer       void *  The buffer in which to read.
 *  nbyte      size_t    The number of bytes to read into buffer[].
 *  timeout      long    The number of milliseconds to wait if the pipe
 *                       contains < nbyte bytes. Use -1 to wait indefinitely.
 *                       If the timeout is reached then the read will be
 *                       aborted without reading anything. Note that
 *                       if there are > 0 and < 'nbyte' bytes in the pipe,
 *                       then select will indicate that the pipe is readable,
 *                       but a non-blocking read of 'nbyte' bytes will fail.
 *                       To avoid this, always try to read as much per
 *                       message as you wrote.
 * Output:
 *  return  PipeState    The status of the transaction:
 *                         PIPE_OK    - The message was read successfully.
 *                         PIPE_BUSY  - The read couldn't be accomplished
 *                                      without blocking (never returned
 *                                      if timeout = -1).
 *                         PIPE_ERROR - An error occurred.
 */
PipeState read_pipe(Pipe *pipe, void *buffer, size_t nbyte, long timeout)
{
  int status;             /* The return status of pthreads functions. */
  PipeFd *rfd;            /* The read end of the pipe */
  PipeState pipe_state;   /* The status of the I/O operation */
  struct timespec endtime;/* The absolute time at which to give up waiting */
/*
 * Check arguments.
 */
  if(!pipe || !buffer) {
    lprintf(stderr, "read_pipe: Bad arguments.\n");
    return PIPE_ERROR;
  };
/*
 * We can't guarantee atomic transactions for > PIPE_BUF bytes.
 */
  if(nbyte > PIPE_BUF) {
    lprintf(stderr, "read_pipe: Rejected nbyte > PIPE_BUF\n");
    return PIPE_ERROR;
  };
/*
 * Work out the timeout, if relevant.
 */
  if(timeout > 0) {
    unsigned long nsec;       /* Temporary variable in nanosecond calculation */
    if(pipe_gettimeofday(&endtime)) {
      lprintf(stderr, "read_pipe: clock_gettime() -> %s.\n", strerror(errno));
      return PIPE_ERROR;
    };
/*
 * Add the timeout duration, being careful to avoid overflowing the
 * nanosecond member (Note that timespec::tv_nsec is signed and nsec isn't).
 */
    endtime.tv_sec += timeout / 1000UL;
    nsec = (unsigned long) endtime.tv_nsec + 1000000UL * (timeout%1000UL);
    if(nsec < 1000000000UL) {
      endtime.tv_nsec = nsec;
    } else {
      endtime.tv_sec += nsec / 1000000000UL;
      endtime.tv_nsec = nsec % 1000000000UL;
    };
  };
/*
 * Acquire exclusive access to the pipe.
 */
  status = pthread_mutex_lock(&pipe->guard);
  if(status) {
    lprintf(stderr, "read_pipe: Couldn't lock mutex.\n");
    return PIPE_ERROR;
  };
/*
 * Attempt to read the requested number of bytes, using non-blocking
 * I/O.  If the data can't be read, and blocking I/O has been
 * requested, wait on a retry condition variable for the pipe to
 * become readable. This variable is posted whenever data is
 * written to the other end of the pipe.
 */
  rfd = &pipe->read;
  do {
/*
 * Work out the number of bytes that still need to be read.
 * Note that if we had to wait in pthread_cond_wait() during the
 * previous iteration of this loop, another thread may have
 * appended some data to pipe->unread[], or taken some out of it.
 */
    int request = nbyte - pipe->nread;
/*
 * If there is sufficient data in pipe->unread[], copy nbyte bytes
 * to the output buffer, then shift any remaining bytes to the beginning
 * of pipe->unread[].
 */
    if(request <= 0) {
      memcpy(buffer, pipe->unread, nbyte);
      pipe->nread -= nbyte;
      memmove(pipe->unread, pipe->unread + nbyte, pipe->nread);
      pipe_state = PIPE_OK;
    } else {
      status = read(rfd->fd, ((char *)buffer) + pipe->nread, request);
/*
 * If the read completed, move buffered data in pipe->unread
 * into the output buffer, and signal blocked writers that the pipe
 * may now have some space available for another message. Note
 * that we use broadcast rather than signal because different
 * threads may be trying to write different amounts of data,
 * and what may not satisfy one thread might satisfy another.
 */
      if(status == request) {
	memcpy(buffer, pipe->unread, pipe->nread);
        pipe->nread = 0;
	pthread_cond_broadcast(&pipe->write.retry);
	pipe_state = PIPE_OK;
/*
 * The return value that signifies that the pipe couldn't be read without
 * blocking, depends on which convention of non-blocking I/O is being used.
 */
      } else if(status == 
#ifdef O_NONBLOCK            /* POSIX.1 */
		-1 && errno == EAGAIN
#elif defined(O_NDELAY)      /* System V */
		0
#elif defined(FNDELAY)       /* BSD */
		-1 && errno == EWOULDBLOCK
#endif
		) {
/*
 * The read would have blocked.
 */
	pipe_state = PIPE_BUSY;
/*
 * Unless non-blocking I/O has been requested, relinquish exclusive
 * access to the pipe while waiting for a message to be written to the
 * other end of the pipe.
 */
	if(timeout < 0) {
	  status = pthread_cond_wait(&rfd->retry, &pipe->guard);
	  if(status) {
	    lprintf(stderr, "read_pipe: cond_wait() -> %s.\n",
		    strerror(status));
	    pipe_state = PIPE_ERROR;
	  };
	} else if(timeout > 0) {
	  status = pthread_cond_timedwait(&rfd->retry, &pipe->guard, &endtime);
	  if(status == ETIMEDOUT) {
	    timeout = 0;
	    pipe_state = PIPE_BUSY;
	  } else if(status) {
	    lprintf(stderr, "read_pipe: cond_timedwait() -> %s.\n",
		    strerror(status));
	    pipe_state = PIPE_ERROR;
	  };
	};
/*
 * I/O error.
 */
      } else if(status < 0) {
	lprintf(stderr, "read_pipe: %s\n", strerror(errno));
	pipe_state = PIPE_ERROR;
/*
 * If fewer bytes were returned than were needed to complete the read,
 * append them to pipe->unread[] then signal other threads that the
 * pipe may now have space for another write(). Also signal other
 * waiting readers that more data has been added to the read buffer.
 * Note that we use broadcast rather than signal because different
 * threads may be attempting different sized transactions,
 * and what may not satisfy one thread might satisfy another.
 */
      } else if(status < request) {
	memcpy(pipe->unread + pipe->nread, ((char *)buffer) + pipe->nread, status);
	pipe->nread += status;
	pthread_cond_broadcast(&pipe->write.retry);
	pthread_cond_broadcast(&pipe->read.retry);
	pipe_state = PIPE_BUSY;
/*
 * Unexpected return code.
 */
      } else {
	lprintf(stderr, "read_pipe: Unexpected code (%d) returned by read().\n",
		status);
	pipe_state = PIPE_ERROR;
      };
    };
  } while(timeout != 0 && pipe_state==PIPE_BUSY);
/*
 * Relinquish exclusive access to the pipe and return the completion
 * status.
 */
  pthread_mutex_unlock(&pipe->guard);
  return pipe_state;
}

/*.......................................................................
 * Write to a pipe using blocking or non-blocking I/O.
 *
 * Input:
 *  pipe         Pipe *  The pipe to read from.
 *  buffer       void *  The buffer from which to write.
 *  nbyte      size_t    The number of bytes to write from buffer[].
 *  timeout      long    The maximum number of milliseconds to wait for
 *                       sufficient space to become available in the pipe.
 *                       To wait indefinitely, send -1.
 *                       If the timeout is reached then the write will be
 *                       aborted without writing anything. Note that
 *                       if there are > 0 and < 'nbyte' bytes available
 *                       in the pipe, then select will indicate that the
 *                       pipe is readable, but a non-blocking write of this
 *                       number of bytes will fail. To avoid this, always
 *                       try to read as much per message as you wrote.
 * Output:
 *  return  PipeState    The status of the transaction:
 *                         PIPE_OK    - The message was written successfully.
 *                         PIPE_BUSY  - The write couldn't be accomplished
 *                                      without blocking (never returned
 *                                      when timeout = -1).
 *                         PIPE_ERROR - An error occurred.
 */
PipeState write_pipe(Pipe *pipe, void *buffer, size_t nbyte, long timeout)
{
  int status;             /* The return status of pthreads functions. */
  PipeFd *wfd;            /* The write end of the pipe */
  PipeState pipe_state;   /* The status of the I/O operation */
  struct timespec endtime;/* The absolute time at which to give up waiting */
/*
 * Check arguments.
 */
  if(!pipe || !buffer) {
    fprintf(stderr, "write_pipe: Bad arguments.\n");
    return PIPE_ERROR;
  };
/*
 * We can't guarantee atomic transactions for > PIPE_BUF bytes.
 */
  if(nbyte > PIPE_BUF) {
    fprintf(stderr, "write_pipe: Rejected nbyte=%lu > PIPE_BUF=%lu.\n",
	    (unsigned long) nbyte, (unsigned long) PIPE_BUF);
    return PIPE_ERROR;
  };
/*
 * Work out the timeout, if relevant.
 */
  if(timeout > 0) {
    unsigned long nsec;       /* Temporary variable in nanosecond calculation */
    if(pipe_gettimeofday(&endtime)) {
      fprintf(stderr, "write_pipe: clock_gettime() -> %s.\n", strerror(errno));
      return PIPE_ERROR;
    };
/*
 * Add the timeout duration, being careful to avoid overflowing the
 * nanosecond member (Note that timespec::tv_nsec is signed and nsec isn't).
 */
    endtime.tv_sec += timeout / 1000UL;
    nsec = (unsigned long) endtime.tv_nsec + 1000000UL * (timeout%1000UL);
    if(nsec < 1000000000UL) {
      endtime.tv_nsec = nsec;
    } else {
      endtime.tv_sec += nsec / 1000000000UL;
      endtime.tv_nsec = nsec % 1000000000UL;
    };
  };
/*
 * Acquire exclusive access to the pipe.
 */
  status = pthread_mutex_lock(&pipe->guard);
  if(status) {
    fprintf(stderr, "write_pipe: Couldn't lock mutex.\n");
    return PIPE_ERROR;
  };
/*
 * Attempt to write the requested number of bytes, using non-blocking
 * I/O.  If the data can't be written, and blocking I/O has been
 * requested, wait on a retry condition variable for the pipe to
 * become writable. This variable is posted whenever data is
 * read from the other end of the pipe.
 */
  wfd = &pipe->write;
  do {
    status = write(wfd->fd, buffer, nbyte);
/*
 * If the write succeded, signal blocked readers that the pipe
 * now contains some data to be read.  Note that we use broadcast
 * rather than signal because different threads may be trying to
 * read different amounts of data, and what may not satisfy one
 * thread might satisfy another.
 */
    if(status == (int)nbyte) {
      pipe_state = PIPE_OK;
      pthread_cond_broadcast(&pipe->read.retry);
/*
 * The return value that signifies that the pipe couldn't be written to
 * without blocking, depends on which convention of non-blocking I/O is
 * being used.
 */
    } else if(status == 
#ifdef O_NONBLOCK            /* POSIX.1 */
     -1 && errno == EAGAIN
#elif defined(O_NDELAY)      /* System V */
     0
#elif defined(FNDELAY)       /* BSD */
     -1 && errno == EWOULDBLOCK
#endif
	      ) {
/*
 * The write would have blocked.
 */
      pipe_state = PIPE_BUSY;
/*
 * Unless non-blocking I/O has been requested, relinquish exclusive
 * access to the pipe while waiting for a message to be read from the
 * other end of the pipe.
 */
      if(timeout < 0) {
	status = pthread_cond_wait(&wfd->retry, &pipe->guard);
	if(status) {
	  fprintf(stderr, "write_pipe: cond_wait() -> %s.\n", strerror(status));
	  pipe_state = PIPE_ERROR;
	};
      } else if(timeout > 0) {
	status = pthread_cond_timedwait(&wfd->retry, &pipe->guard, &endtime);
	if(status == ETIMEDOUT) {
	  timeout = 0;
	  pipe_state = PIPE_BUSY;
	} else if(status) {
	  fprintf(stderr, "write_pipe: cond_wait() -> %s.\n", strerror(status));
	  pipe_state = PIPE_ERROR;
	};
      };
/*
 * I/O error.
 */
    } else if(status < 0) {
      fprintf(stderr, "write_pipe: %s\n", strerror(errno));
      pipe_state = PIPE_ERROR;
/*
 * Unexpected return code.
 */
    } else {
      fprintf(stderr, "write_pipe: Unexpected code (%d) returned by write().\n",
	      status);
      pipe_state = PIPE_ERROR;
    };
  } while(timeout != 0 && pipe_state==PIPE_BUSY);
/*
 * Relinquish exclusive access to the pipe and return the completion
 * status.
 */
  pthread_mutex_unlock(&pipe->guard);
  return pipe_state;
}

/*.......................................................................
 * Return the file descriptor of the readable end of the pipe. This
 * is only for use in select.
 *
 * Input:
 *  pipe     Pipe *    The pipe to query.
 * Output:
 *  return    int      The pipe fd, or -1 if pipe isn't valid.
 */
int pipe_read_fd(Pipe *pipe)
{
  return pipe ? pipe->read.fd : -1;
}

/*.......................................................................
 * Return the file descriptor of the writable end of the pipe. This
 * is only for use in select.
 *
 * Input:
 *  pipe     Pipe *    The pipe to query.
 * Output:
 *  return    int      The pipe fd, or -1 if pipe isn't valid.
 */
int pipe_write_fd(Pipe *pipe)
{
  return pipe ? pipe->write.fd : -1;
}

/*.......................................................................
 * Get the current time of day.
 *
 * Input/Output:
 *  ts   struct timespec *  The current time of day.
 * Output:
 *  return           int    0 - OK.
 *                          1 - Error.
 */
static int pipe_gettimeofday(struct timespec *ts)
{
  struct timeval tp;
/*
 * The BSD gettimeofday() function seems to be much more widely
 * available than the posix clock_gettime() function.
 */
   if(gettimeofday(&tp, NULL)) {
     fprintf(stderr, "pipe_gettimeofday: Time not available.\n");
     return 1;
  };
  ts->tv_sec = tp.tv_sec;
  ts->tv_nsec = tp.tv_usec * 1000;
  return 0;
}
