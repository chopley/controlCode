#ifndef pipe_h
#define pipe_h

/*
 * This module provides a thread-safe architecture-neutral wrapper
 * around unnamed pipes. It allows multiple threads to read and/or write
 * to a pipe, each with different blocking requirements, and hides the
 * differences between non-blocking I/O return codes on different
 * architectures.
 */

typedef struct Pipe Pipe;

/*
 * Pipe constructor and destructor functions.
 */
Pipe *new_Pipe(void);
Pipe *del_Pipe(Pipe *pipe);

/*
 * Enumerate the return statuses of read_pipe() and write_pipe().
 */
typedef enum {
  PIPE_OK,      /* The I/O completed successfully */
  PIPE_BUSY,    /* The I/O couldn't be performed without blocking */
  PIPE_ERROR    /* An error occurred */
} PipeState;

/*
 * Enumerate the two special I/O timeout values.
 */
enum {
  PIPE_WAIT = -1,    /* Wait to complete the transaction */
  PIPE_NOWAIT = 0    /* Return immediately if the transaction would block */
};

PipeState read_pipe(Pipe *pipe, void *buffer, size_t nbyte, long timeout);
PipeState write_pipe(Pipe *pipe, void *buffer, size_t nbyte, long timeout);

/*
 * To allow select() and poll() to be used with a pipe, the following
 * functions return the file descriptors of the pipe. Do not use these
 * fds for any other purpose.
 */
int pipe_read_fd(Pipe *pipe);
int pipe_write_fd(Pipe *pipe);

#endif
