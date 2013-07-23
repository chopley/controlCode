#ifndef netbuf_h
#define netbuf_h

#include <stddef.h>  /* size_t */

/*
 * This header describes an interface for sending and receiving messages
 * over byte-oriented streams that are accessed via a file descriptor.
 * It is intended to be used for communication between machines that may
 * have different architectures. Messages are composed of a few primitive
 * datatypes which are packed into network byte-order for transmission
 * and unpacked back to host-byte order on receipt. Method functions for
 * packing, unpacking, reading and writing messages are provided below.
 */

/*
 * Each network message is preceded by a 32-bit byte-count and a
 * 32-bit opcode. The byte-count includes both of these items.
 * The following enumerates the length of the message prefix in bytes.
 */
enum {NET_PREFIX_LEN = 8};

/*
 * List the packed sizes of each of the supported network data types.
 */
typedef enum {
  NET_CHAR_SIZE   = 1,
  NET_SHORT_SIZE  = 2,
  NET_INT_SIZE    = 4,
  NET_FLOAT_SIZE  = 4,
  NET_DOUBLE_SIZE = 8
} NetTypeSize;

namespace gcp {
  namespace control {
    /*
     * Declare an object type to encapsulate network messages in packed
     * form. All members of this structure should be treated as private.
     * An object of this type is encapsulated in both of the read and write
     * stream structures described later.
     */
    typedef struct NetBuf {
      unsigned char *buf;    /* The network I/O buffer */
      unsigned int size;     /* The allocated size of buf[] */
      unsigned int nget;     /* The number of bytes extracted from the buffer */
      unsigned int nput;     /* The number of bytes added to the buffer */
      unsigned int external; /* True if buf[] is an externally provided buffer */
    } NetBuf;
    
  }
}

/*
 * The following functions are used to create and destroy a network
 * buffer object. Note that this is performed for you by new_NetReadStr()
 * and new_NetSendStr() [see below].
 */
gcp::control::NetBuf *new_NetBuf(int size);     /* NetBuf constructor */
gcp::control::NetBuf *del_NetBuf(gcp::control::NetBuf *net);   /* NetBuf desctructor */

/*
 * Return the size that was passed to new_NetBuf(). Note that this differs
 * from net->size by NET_PREFIX_LEN.
 */
int size_NetBuf(gcp::control::NetBuf *net);

/*
 * NetBuf containers created with a buffer 'size' of zero must have
 * an external buffer supplied before use.
 */
void *net_set_buffer(gcp::control::NetBuf *net, void *buf, size_t length);
/*
 * When an external buffer is being used to format messages for
 * output, net_inc_nput() can be used to increment the recorded length
 * of the message to account for externally formatted data in the
 * provided buffer. net_inc_nput() returns the buffer index of the
 * next byte in the buffer. It can thus also be used to determine the
 * buffer index of the current end-of-message by sending nbytes=0.
 */
int net_inc_nput(gcp::control::NetBuf *net, int nbytes);

/*
 * After reading a message into a NetBuf buffer, as described later,
 * the following method functions should be used to unpack the
 * primitive datatype components of the message. To extract a message
 * first call net_start_get() to read the message header, then use the
 * net_get_*() functions to sequentially unpack each component of the
 * message body, then finally call net_end_get() to verify that all
 * of the bytes in the message have been extracted.
 */
int net_start_get(gcp::control::NetBuf *net, int *opcode);
int net_end_get(gcp::control::NetBuf *net);
int net_get_char(gcp::control::NetBuf *net, int ndata, unsigned char *data);
int net_get_short(gcp::control::NetBuf *net, int ndata, unsigned short *data);
int net_get_int(gcp::control::NetBuf *net, int ndata, unsigned int *data);
int net_get_float(gcp::control::NetBuf *net, int ndata, float *data);
int net_get_double(gcp::control::NetBuf *net, int ndata, double *data);
int net_inc_nget(gcp::control::NetBuf *net, int nbytes);

/*
 * The following method functions are used for packing
 * primitive datatype components of a network message into a
 * NetBuf buffer. To compose a new message first call
 * net_start_put() to prepend the standard message prefix,
 * then use the net_put_*() functions to pack one or more
 * items into subsequent bytes of the message, then finally
 * call net_end_put() to insert the final message byte-count
 * at the start of the message. The message is then ready to
 * be sent over a stream with nss_send_msg() as described below.
 *
 * Note that the opcode argument of net_start_put() is intended to be
 * used by the application to enumerate the type of message being
 * sent.
 */
int net_start_put(gcp::control::NetBuf *net, int opcode);
int net_end_put(gcp::control::NetBuf *net);
int net_put_char(gcp::control::NetBuf *net, int ndata, unsigned char *data);
int net_put_short(gcp::control::NetBuf *net, int ndata, unsigned short *data);
int net_put_int(gcp::control::NetBuf *net, int ndata, unsigned int *data);
int net_put_float(gcp::control::NetBuf *net, int ndata, float *data);
int net_put_double(gcp::control::NetBuf *net, int ndata, double *data);

namespace gcp {
  namespace control {
    /*
     * Declare an object to encapsulate the process of reading messages
     * from a network input stream.
     */
    typedef struct NetReadStr {
      NetBuf *net;          /* Network I/O buffer container */
      unsigned int msglen;  /* The target message length (bytes) */
      int fd;               /* A stream file-descriptor open for reading */
      enum NetReadId {                /* (Do not change the ordering below) */
	NET_READ_SIZE,      /* The message byte-count is being read */
	NET_READ_DATA,      /* The message body is being read */
	NET_READ_DONE,      /* The message has been completely read */
	NET_READ_CLOSED,    /* Closed connection detected at start of message */
	NET_READ_ERROR      /* I/O error detected while reading */
      } state;
    } NetReadStr;
  }
}

/*
 * The following functions must be used to attach/detach to a given open,
 * readable byte stream. Note that the file-descriptor can refer to
 * a blocking or non-blocking stream. The 'size' argument specifies
 * the maximum expected size of a network message.
 */
gcp::control::NetReadStr *new_NetReadStr(int fd, int size);  /* NetReadStr constructor */
gcp::control::NetReadStr *del_NetReadStr(gcp::control::NetReadStr *nrs);    /* NetReadStr destructor */
void attach_NetReadStr(gcp::control::NetReadStr *nrs, int fd);/* Assign a new stream fd */

/*
 * A single message is read through one or more calls to
 * nrs_read_stream(). If the file-descriptor is set up to use
 * non-blocking I/O then this may require more than one call. When
 * a complete message has been read, nrs_read_stream() will return
 * NET_READ_DONE. The resulting message can be found in nrs->net,
 * and can be decoded as described earlier, via calls to net_start_get(),
 * net_get_*() and net_end_get(). Other nrs_read_msg() return values
 * are listed above in the declaration of the 'state' member of the
 * NetReadStr object.
 */
int nrs_read_msg(gcp::control::NetReadStr *nrs);

namespace gcp {
  namespace control {
    /*
     * Declare an object to encapsulate the process of writing messages
     * to a network output stream.
     */
    typedef struct NetSendStr {
      NetBuf *net;         /* Network I/O buffer container */
      int fd;              /* A stream file-descriptor open for writing */
      enum NetSendId {
	NET_SEND_DATA,     /* Message is in the process of being written */
	NET_SEND_DONE,     /* Message has been completely sent */
	NET_SEND_CLOSED,   /* Closed connection detected */
	NET_SEND_ERROR     /* Write I/O error detected */
      } state;
    } NetSendStr;
  }
}

/*
 * The following functions must be used to attach/detach to a given open,
 * writable byte stream. Note that the file-descriptor can refer to
 * a blocking or non-blocking stream. The 'size' argument specifies
 * the maximum expected size of a network message.
 */
gcp::control::NetSendStr *new_NetSendStr(int fd, int size);  /* NetSendStr constructor */
gcp::control::NetSendStr *del_NetSendStr(gcp::control::NetSendStr *nss);    /* NetSendStr destructor */
void attach_NetSendStr(gcp::control::NetSendStr *nss, int fd);/* Assign a new stream fd */

/*
 * Once a message has been completely composed in nss->net via calls
 * to net_start_put(), net_put_*() and net_end_put(), it can be sent with
 * one or more calls to nss_send_msg(). If the stream has been set up for
 * non-blocking I/O then more than one call may be required. When the
 * message has been completely sent, nss_send_msg() returns NET_SEND_DONE.
 * Other return values are listed above in the declaration of the 'state'
 * member of the NetSendStr object.
 */
int nss_send_msg(gcp::control::NetSendStr *nss);

#endif
