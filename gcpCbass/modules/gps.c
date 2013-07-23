/*--------------------------------------------------------------------------- 
 * Filename: gps.c 
 * Author:   Erik Leitch 
 * Date:     July-2005 
 * Platform: Linux 
 * Purpose:  Device driver development 
 *--------------------------------------------------------------------------- 
 * 
 * This is a template driver for use while developing driver code.  
 * 
 * This template implements: 
 * 
 *  - module load and unload  
 *  - install parameter passing/interrogation 
 *  - open, release 
 *  - read, write 
 *  - llseek 
 *  - ioctl 
 *  - poll 
 *  - fasync 
 *  - kernel timers 
 *  - IRQ handling 
 * 
 * This code is extended from gps_timer.c with the modifications that 
 * the kernel timer is used to generate an IRQ after a given time delay.  
 * The IRQ then unblocks the device wait-queue (non-blocking calls to  
 * read/write return immediately). 
 * 
 * The device sleeps on a wait-queue while waiting for the IRQ. 
 * This wait is limited by a timeout. The IRQ delay is 1s, while 
 * the timeout is 2s. If there is no hardware connected to generate 
 * an IRQ, then the timeout will complete the transaction. 
 * 
 *--------------------------------------------------------------------------- 
 * Command line tools can test open/read/write/release: 
 *  
 * eg. blocking write-only : dd bs=10 count=1 if=/dev/zero of=/dev/gps 
 *     blocking read-only  : dd bs=10 count=1 if=/dev/gps of=/dev/zero 
 *  
 *--------------------------------------------------------------------------- 
 */ 
 
/*--------------------------------------------------------------------------- 
 * Includes 
 *--------------------------------------------------------------------------- 
 */ 

#define DEBUG
 
#include <linux/module.h> 
#include <linux/kernel.h> 
#include <linux/init.h> 
#include <linux/version.h> 
 
#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,5,0)
#else
#include <asm-i386/mach-default/irq_vectors_limits.h>
#include <asm-i386/mach-default/irq_vectors.h>
#endif
  
#include <linux/fs.h>			/* file and device operations         */ 
#include <linux/poll.h>         /* for poll                           */ 
#include <linux/sched.h>        /* ISR structure definitions          */ 
#include <linux/interrupt.h>	/* IRQ handling - ISRs                */ 
#include <linux/slab.h>         /* used to resolve kmalloc/kfree      */ 
#include <linux/pci.h>          /* PCI related macros and functions   */ 
#include <linux/timer.h>        /* Kernel timers                      */ 
#include <asm/uaccess.h>        /* get/put_user, copy_to/from_user    */ 
#include <asm/io.h>             /* ioremap, iounmap, memcpy_to/fromio */ 
 
#include "gps.h" 
 
/*--------------------------------------------------------------------------- 
 * Global parameters 
 *--------------------------------------------------------------------------- 
 */ 
 
/* Default parallel port parameters */ 
//#define ACU_BASE 0x378 
//#define ACU_SIZE 5 
#define ACU_BASE 0xb000
#define ACU_SIZE 5
#define ACU_IRQ  7
//#define ACU_IRQ  9
 
#define ACU_DATA      ACU_BASE 
#define ACU_STATUS   (ACU_BASE+1) 
#define ACU_CONTROL  (ACU_BASE+2) 
 
#define ACU_DELAY    (HZ) 
#define ACU_TIMEOUT  (2*HZ) 
  

#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,5,0)
static char debug = 1; 
#else
int debug = 1; 
#endif

static char *name = "gps"; 
static int  major = 0; 
static struct fasync_struct *gps_async_queue; 
static struct timer_list gps_timer;  
 
DECLARE_WAIT_QUEUE_HEAD (gps_write_q); 
DECLARE_WAIT_QUEUE_HEAD (gps_read_q); 
 
static char *gps_buf = NULL; 
static char *gps_buf_wp = NULL; 
static char *gps_buf_rp = NULL; 
static int   gps_buf_cnt = 0; 
spinlock_t   gps_buf_lock; 
 
#define BUFFER_SIZE  (4*PAGE_SIZE) 

static int clientData;

#define SHARED

#ifdef SHARED
static void* deviceId = (void*)&clientData;
#else
static void* deviceId = NULL;
#endif

 
/*--------------------------------------------------------------------------- 
 * Information extractable via `modinfo' command 
 *--------------------------------------------------------------------------- 
 * 
 * Use /sbin/modinfo -p gps_driver.o (i.e., on the driver file)  
 * to see the parameters. 
 *  
 */ 
 
MODULE_DESCRIPTION("Acu device driver module"); 
MODULE_AUTHOR("Erik Leitch"); 
 
#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,5,0)
MODULE_PARM(debug, "1b"); 
#else
module_param(debug, int, 0); 
#endif
 
MODULE_PARM_DESC(debug, "Enable/disable debugging messages"); 
 
/*--------------------------------------------------------------------------- 
 * Debug message macro 
 *--------------------------------------------------------------------------- 
 * 
 * This macro is used to generate debug information in the module. A module 
 * compiled with the symbol DEBUG defined can turn message generation on or 
 * off using an IOCTL call. A module compiled without DEBUG defined will 
 * not generate messages. (See the schar example in Ch. 21 [Mat99]). 
 * 
 */ 
 
#define DEBUG

#ifdef DEBUG 
#define MSG(string, args...) if (debug) printk("gps: " string, ##args) 
#else 
#define MSG(string, args...) 
#endif 
 
/*--------------------------------------------------------------------------- 
 * File operations declarations 
 *--------------------------------------------------------------------------- 
 */ 
 
/* Function prototypes */ 
static loff_t gps_llseek( 
			  struct file *file, 
			  loff_t offset, 
			  int mode); 
static ssize_t gps_read( 
			 struct file *file, 
			 char *buf, 
			 size_t count, 
			 loff_t *offset); 
static ssize_t gps_write( 
			  struct file *file, 
			  const char *buf, 
			  size_t count, 
			  loff_t *offset); 
static unsigned int gps_poll( 
			      struct file *file, 
			      poll_table *wait); 
static int gps_ioctl( 
		      struct inode *inode, 
		      struct file *file, 
		      unsigned int cmd, 
		      unsigned long arg); 
static int gps_mmap( 
		     struct file *file, 
		     struct vm_area_struct *vma); 
static int gps_open( 
		     struct inode *inode, 
		     struct file *file); 
static int gps_release( 
			struct inode *inode, 
			struct file *file); 
static int gps_fasync( 
		       int fd, 
		       struct file *file, 
		       int mode); 
 
/* File operations structure (changed in 2.4) 
 * - the structure is defined in /usr/include/linux/fs.h 
 * - for an example use, see the Watch Dog Timer driver 
 *   /usr/src/linux-2.4/drivers/char/wdt_pci.c 
 * - the labels allow you to miss out any unimplemented functions. 
 */ 
static struct file_operations gps_fops = { 
  owner:		THIS_MODULE, 
  llseek:		gps_llseek, 
  read:		gps_read, 
  write:		gps_write, 
  poll:		gps_poll, 
  ioctl:		gps_ioctl, 
  mmap:		gps_mmap, 
  open:		gps_open, 
  release:	gps_release, 
  fasync:     gps_fasync, 
}; 
 
/*--------------------------------------------------------------------------- 
 * Timer and IRQ handler declarations 
 *--------------------------------------------------------------------------- 
 */ 
 
#if 0
static void gps_timer_handler(unsigned long data); 
#endif

#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,5,0)
static void gps_irq_handler(int irq, void *dev_id, struct pt_regs *regs);
#else
static irqreturn_t gps_irq_handler(int irq, void *dev_id, struct pt_regs *regs);
#endif

 
/*--------------------------------------------------------------------------- 
 * File operations definitions 
 *--------------------------------------------------------------------------- 
 */ 
 
static loff_t gps_llseek( 
			  struct file *file, 
			  loff_t offset, 
			  int mode) 
{ 
  int major,minor; 
 
  /* Find the board and BAR */ 
  major = MAJOR(file->f_dentry->d_inode->i_rdev); 
  minor = MINOR(file->f_dentry->d_inode->i_rdev); 
 
  MSG("  <llseek> called with major %d, minor %d, offset %ld.\n", 
      major, minor, (long)offset); 
 
  /* Always return an error */ 
  return -ESPIPE; 
} 
 
static ssize_t gps_read( 
			 struct file *file, 
			 char *buf, 
			 size_t count, 
			 loff_t *offset) 
{ 
  int major,minor,status; 
  unsigned long flags; 
 
  /* Find the board and BAR */ 
  major = MAJOR(file->f_dentry->d_inode->i_rdev); 
  minor = MINOR(file->f_dentry->d_inode->i_rdev); 
 
  MSG("  <read> called with major %d, minor %d, count %ld, " 
      "by %s (pid = %u).\n", 
      major, minor, (long)count, current->comm, current->pid); 
 
  /* Check the buffer count */ 
  spin_lock_irqsave(&gps_buf_lock,flags); 
  while (gps_buf_cnt == 0) { 
    spin_unlock_irqrestore(&gps_buf_lock, flags); 
		 
    /* Blocking/non-blocking */ 
    /* Nothing to read */ 
    if (file->f_flags & O_NONBLOCK) { 
      MSG("  - non-blocking read - no data in buffer.\n"); 
      return -EAGAIN; 
    } else { 
      MSG("  - blocking read - no data in buffer.\n"); 
 
      MSG("  - waiting on the device's read wait-queue.\n"); 
      status = interruptible_sleep_on_timeout(&gps_read_q,ACU_TIMEOUT); 
      if (!status) { 
	MSG("  - sleep timed out (status = %d).\n",status); 
	/* see comments near the write timeout below */ 
	return -ENODATA; 
      } 
      /* Woken by IRQ, write, or signal */ 
      if (signal_pending(current)) { 
	MSG("  - signal pending, so restart-sys.\n"); 
	return -ERESTARTSYS; 
      } else { 
	MSG("  - data available / woken by IRQ or write\n"); 
      } 
    } 
    /* Must have got here by new data - double check for the data */ 
    spin_lock_irqsave(&gps_buf_lock,flags); 
  } 
  /* Ok, the buffer is ours - hang onto the spin lock until 
   * we're all done - we don't want a write from another process, 
   * or the IRQ to mess with the buffer and buffer count until  
   * we're done. 
   */ 
 
  /* Request is for more than we've got */ 
  if (gps_buf_cnt < count) {  
    MSG("  - read count (%d) exceeds buffer contents " 
	"- reducing count.\n", count); 
    count = gps_buf_cnt; 
  } 
 
  /* Request goes over the end of the circular buffer  
   * (let user-space handle the pointer wrap) 
   */ 
  if ((gps_buf_rp + count) > (gps_buf + BUFFER_SIZE)) { 
    /* Data from gps_buf_rp to the end of the buffer */ 
    MSG("  - read count (%d) exceeds buffer end "
	"- reducing count.\n", count); 
    count = gps_buf + BUFFER_SIZE - gps_buf_rp;  
  } 
 
  /* Copy read data to the user buffer */ 
  MSG("  - copying %ld bytes to user-space.\n", (unsigned long)count); 
  if (copy_to_user(buf, gps_buf_rp, count)) { 
    MSG("  * copy-to-user fault!\n"); 
    spin_unlock_irqrestore(&gps_buf_lock, flags); 
    return -EFAULT; 
  } 
  gps_buf_cnt -= count; 
  gps_buf_rp  += count; 
  if (gps_buf_rp == gps_buf + BUFFER_SIZE) { 
    gps_buf_rp = gps_buf; 
  } 
  spin_unlock_irqrestore(&gps_buf_lock,flags); 
	 
  /* Wake up any writers, as there is space there now */ 
  wake_up_interruptible(&gps_write_q); 
	 
  /* Return the number of bytes sucessfully read */ 
  return count; 
} 
 
static ssize_t gps_write( 
			  struct file *file, 
			  const char *buf, 
			  size_t count, 
			  loff_t *offset) 
{ 
  int major,minor,status; 
  unsigned long flags; 
	 
  /* Find the board and BAR */ 
  major = MAJOR(file->f_dentry->d_inode->i_rdev); 
  minor = MINOR(file->f_dentry->d_inode->i_rdev); 
 
  MSG("  <write> called with major %d, minor %d, count %ld, "
      "by %s (pid = %u).\n", 
      major, minor, (long)count, current->comm, current->pid); 
	 
  /* Check for space in the buffer */ 
  spin_lock_irqsave(&gps_buf_lock,flags); 
  while (gps_buf_cnt == BUFFER_SIZE) { 
    spin_unlock_irqrestore(&gps_buf_lock, flags); 
		 
    /* Blocking/non-blocking */ 
    if (file->f_flags & O_NONBLOCK) { 
      MSG("  - non-blocking write - no room in buffer.\n"); 
      return -EAGAIN; 
    } else { 
      MSG("  - blocking write - no room in buffer.\n"); 
      MSG("  - waiting on the device's write wait-queue.\n"); 
      status = interruptible_sleep_on_timeout(&gps_write_q,ACU_TIMEOUT); 
      if (!status) { 
	MSG("  - sleep timed out (status = %d).\n",status); 
				 
	/* What is the appropriate return value? (asm/errno.h)  
	 * 
	 * TEST: cp /dev/zero /dev/gps returns with 
	 * 
	 * -EAGAIN: 
	 *     Resource temporarily unavailable. 
	 * 
	 * -ERESTARTSYS: 
	 *     Unknown error 512 
	 * 
	 * -ENOSPC: 
	 *     No space left on device. 
	 * 
	 * -ENOBUFS: 
	 *    No buffer space available 
	 *  
	 *     Ok, either of the last two look like the 
	 *     appropriate value for a write timeout. 
	 * 
	 *     -ENODATA looks right for the read timeout. 
	 * 
	 *     Hmmm, -ETIME (Timer expired) could also be used. 
	 */ 
	return -ENOSPC; 
      } 
      /* Woken by read or signal */ 
      if (signal_pending(current)) { 
	MSG("  - signal pending, so restart-sys.\n"); 
	return -ERESTARTSYS; 
      } else { 
	MSG("  - woken by read.\n"); 
      } 
    } 
    /* Must have got here due to space available  
     * - double check for the data */ 
    spin_lock_irqsave(&gps_buf_lock,flags); 
  } 
  /* Ok, the buffer is ours - hang onto the spin lock until 
   * we're all done - we don't want a read from another process, 
   * or the IRQ to mess with the buffer and buffer count until  
   * we're done. 
   */ 
 
  /* Request is too big */ 
  if ((BUFFER_SIZE - gps_buf_cnt) < count) {  
    MSG("  - write count (%d) exceeds buffer space (%ld) "
	"- reducing count.\n", count, BUFFER_SIZE - gps_buf_cnt); 
    count = BUFFER_SIZE - gps_buf_cnt; 
  } 
 
  /* Request goes over the end of the circular buffer  
   * (let user-space handle the pointer wrap) 
   */ 
  if ((gps_buf_wp + count) > (gps_buf + BUFFER_SIZE)) { 
    /* Data from gps_buf_wp to the end of the buffer */ 
    MSG("  - write count (%d) exceeds buffer end "
	"- reducing count.\n", count); 
    count = gps_buf + BUFFER_SIZE - gps_buf_wp;  
  } 
  /* Copy write data to the kernel buffer */ 
  MSG("  - copying %ld bytes from user-space.\n", (unsigned long)count); 
  if (copy_from_user(gps_buf_wp, buf, count)) { 
    MSG("  * copy-from-user fault!\n"); 
    spin_unlock_irqrestore(&gps_buf_lock, flags); 
    return -EFAULT; 
  } 
  gps_buf_cnt += count; 
  gps_buf_wp  += count; 
  /* Wrap to buffer start */ 
  if (gps_buf_wp == gps_buf + BUFFER_SIZE) { 
    gps_buf_wp = gps_buf; 
  } 
  spin_unlock_irqrestore(&gps_buf_lock,flags); 
	 
  /* Wake up any readers, as there is data there now */ 
  wake_up_interruptible(&gps_read_q); 
	 
  /* Return the number of bytes successfully written */ 
  return count; 
} 
 
/* Poll: used by non-blocking I/O */ 
static unsigned int gps_poll( 
			      struct file *file, 
			      poll_table *wait) 
{ 
  unsigned int mask = 0; 
  int major,minor; 
  unsigned long flags; 
  int count; 
	 
  /* Find the board and BAR */ 
  major = MAJOR(file->f_dentry->d_inode->i_rdev); 
  minor = MINOR(file->f_dentry->d_inode->i_rdev); 
 
  MSG("  <poll> called with major %d, minor %d, by %s (pid = %u).\n", 
      major, minor, current->comm, current->pid); 
	 
  /* Add the wait-queues to the poll table */ 
  poll_wait(file,&gps_read_q,wait); 
  poll_wait(file,&gps_write_q,wait); 
	 
  /* Get the buffer count */ 
  spin_lock_irqsave(&gps_buf_lock,flags); 
  count = gps_buf_cnt; 
  gps_buf_cnt = 0; 
  spin_unlock_irqrestore(&gps_buf_lock, flags); 
		 
  if (count > 0)  
    mask |= POLLIN | POLLRDNORM;  /* readable  */ 
	 
  if (count < BUFFER_SIZE) 
    mask |= POLLOUT | POLLWRNORM; /* writable  */ 
 
  /* no exceptions supported */ 
	 
  return mask; 
} 
 
static int gps_ioctl( 
		      struct inode *inode, 
		      struct file *file, 
		      unsigned int cmd, 
		      unsigned long arg) 
{ 
  int major,minor; 
 
  /* Find the board and BAR */ 
  major = MAJOR(inode->i_rdev); 
  minor = MINOR(inode->i_rdev); 
 
  MSG("  <ioctl> called with major %d, minor %d, command 0x%X.\n", 
      major, minor, cmd); 
 
  switch (cmd) 
    { 
    case ACU_IOCTL_DEBUG_GET: 
      if(put_user(debug, (char *)arg)) 
	return -EFAULT; 
      break; 
 
    case ACU_IOCTL_DEBUG_SET: 
      if(get_user(debug, (char *)arg)) 
	return -EFAULT; 
      break; 
 
    case ACU_IOCTL_SIGIO: 
      MSG("  * ioctl call to send SIGIO.\n"); 
      kill_fasync(&gps_async_queue, SIGIO, POLL_IN); 
      break; 
    default: 
      return -ENOTTY; 
    } 
  return 0; 
} 
 
static int gps_mmap( 
		     struct file *file, 
		     struct vm_area_struct *vma) 
{ 
  int major,minor; 
  unsigned long vm_size, vm_offset; 
	 
  /* Find the board and BAR */ 
  major = MAJOR(file->f_dentry->d_inode->i_rdev); 
  minor = MINOR(file->f_dentry->d_inode->i_rdev); 
 
  MSG("  <mmap> called with major %d, minor %d.\n", 
      major, minor); 
 
  /* Look at the VMA passed in */ 
  vm_size   = vma->vm_end - vma->vm_start; 
  vm_offset = vma->vm_pgoff << PAGE_SHIFT; 
 
  MSG("  * requested map size is %ld, and offset is %ld.\n",vm_size,vm_offset); 
 
  /* Increase vm_size to the nearest page */ 
  if (vm_size % PAGE_SIZE) { 
    vm_size = (vm_size/PAGE_SIZE+1)*PAGE_SIZE; 
    MSG("  * increasing map size to %ld.\n",vm_size); 
  } 
	 
  /* Don't actually mmap anything */ 
	 
  return 0; 
} 
 
static int gps_open( 
		     struct inode *inode, 
		     struct file *file) 
{ 
  int major,minor; 
 
  /* Find the board and BAR */ 
  major = MAJOR(inode->i_rdev); 
  minor = MINOR(inode->i_rdev); 
 
  /* Test for how it was opened */ 
  if (file->f_mode & FMODE_READ) { 
    if (file->f_mode & FMODE_WRITE) { 
      MSG("<open> read/write. " 
	  "Called with major %d, minor %d, by %s (pid = %u).\n", 
	  major, minor, current->comm, current->pid); 
    } else { 
      MSG("<open> read-only. " 
	  "Called with major %d, minor %d, by %s (pid = %u).\n", 
	  major, minor, current->comm, current->pid); 
    } 
  } else { 
    if (file->f_mode & FMODE_WRITE) { 
      MSG("<open> write-only. " 
	  "Called with major %d, minor %d, by %s (pid = %u).\n", 
	  major, minor, current->comm, current->pid);; 
    } else { 
      MSG("<open> with unexpected f_mode setting. " 
	  "Called with major %d, minor %d, by %s (pid = %u).\n", 
	  major, minor, current->comm, current->pid);; 
    } 
  }		 
 
  /* Increment the use count (obsolete in kernels later than 2.6) */

#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,5,0)
  MOD_INC_USE_COUNT;
#endif

  /* Enable interrupts */ 
 
  MSG("Enabling interrupts\n");

  outb(0x00, ACU_DATA); 
  outb(0x10, ACU_CONTROL); 
 
  return 0; 
} 
 
static int gps_release( 
			struct inode *inode, 
			struct file *file) 
{ 
  int major,minor; 
 
  /* Find the board and BAR */ 
  major = MAJOR(inode->i_rdev); 
  minor = MINOR(inode->i_rdev); 
 
  /* Remove this file pointer from the list of asynchronous 
   * notifications. Do this before the message, so that 
   * the Fasync message appears before the Release message.  
   */ 
  if (file->f_flags & FASYNC) { 
    gps_fasync(-1, file, 0); 
  } 
 
  MSG("<release> called with major %d, minor %d.\n", 
      major, minor); 
 
    /* Decrement the use count (obsolete in later kernels) */
     
#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,5,0)
    MOD_DEC_USE_COUNT;
#endif
 
  /* Disable interrupts if this module is no longer in use */ 
 
  MSG("About to disable interrupts\n");

  outb(0x00, ACU_DATA); 
  outb(0x00, ACU_CONTROL); 

  MSG("Done disabling interrupts\n");

  return 0; 
} 
 
static int gps_fasync( 
		       int fd, 
		       struct file *file, 
		       int mode) 
{ 
  int major,minor; 
 
  /* Find the board and BAR */ 
  major = MAJOR(file->f_dentry->d_inode->i_rdev); 
  minor = MINOR(file->f_dentry->d_inode->i_rdev); 
 
  MSG("  <fasyc> called with major %d, minor %d, mode %d.\n", 
      major, minor, mode); 
 
  return fasync_helper(fd,file,mode,&gps_async_queue); 
} 
 
/*--------------------------------------------------------------------------- 
 * IRQ and Timer handler definitions 
 *--------------------------------------------------------------------------- 
 */ 
 
#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,5,0)
static void gps_irq_handler(int irq, void *dev_id, struct pt_regs *regs) 
#else
static irqreturn_t gps_irq_handler(int irq, void *dev_id, struct pt_regs *regs) 
#endif
{ 
  unsigned long flags; 
  int len1, len2; 
  char str[30];    /* shouldn't overflow? */ 
  struct timeval tv; 
 
  MSG("  <irq %d> received!\n", irq); 

#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,5,0)
  if(dev_id != deviceId)
    return; 
#else
  if(dev_id != deviceId)
    return IRQ_NONE;
#endif
 
  /* Clear the interrupt source */ 
  outb(0x00, ACU_DATA); 
 
  /* Format a timestamp */ 
  do_gettimeofday(&tv); 
  len1 = 0; 
  len2 = sprintf(str, "%08u.%06u\n",(int)tv.tv_sec, (int)tv.tv_usec); 
 
  MSG("  - timestamp is %s", str); 
	 
  /* Write the timestamp */ 
 
  spin_lock_irqsave(&gps_buf_lock,flags); 
 
  /* Clear the buffer if its full */ 
 
  if (gps_buf_cnt + len2 > BUFFER_SIZE) { 
    MSG("  - write buffer is full, clearing\n"); 
    gps_buf_cnt = 0; 
    gps_buf_rp = gps_buf; 
    gps_buf_wp = gps_buf; 
  } 
 
  MSG("  - adding %d chars to the buffer.\n", len2); 
 
  gps_buf_cnt += len2; 
 
  /* Wrap the string if necessary */ 
 
  if (gps_buf_wp + len2 >= gps_buf + BUFFER_SIZE) { 
    MSG("  - wrapping string in write buffer\n"); 
    len1 = gps_buf + BUFFER_SIZE - gps_buf_wp; /* first part of str */ 
    strncpy(gps_buf_wp, str, len1); 
    gps_buf_wp = gps_buf; 
    len2 -= len1; /* remaining chars */ 
  } 
  strncpy(gps_buf_wp, &str[len1], len2); /* all or remainder of string */ 
  gps_buf_wp += len2; 
 
  spin_unlock_irqrestore(&gps_buf_lock, flags); 
 
  /* Unblock the read/poll call */ 
 
  wake_up_interruptible(&gps_read_q);  

#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,5,0)
  return; 
#else
  return IRQ_HANDLED;
#endif
} 

// (EML) Inserting an instance if this macro is supposed to make the
// "tainted module" message go away

MODULE_LICENSE("GPL");

/*--------------------------------------------------------------------------- 
 * Module load/unload declaration and definition 
 *--------------------------------------------------------------------------- 
 */ 
int gps_init(void) 
{ 
  int status; 
	 
  MSG("<init> called.\n"); 
	 
#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,5,0)

  /* Get the parallel port I/O range */ 

  status = check_region(ACU_BASE, ACU_SIZE); 
  if (status) { 
    MSG(" - request for I/O ports 0x%X to 0x%X failed\n",  
	ACU_BASE, ACU_BASE+ACU_SIZE); 
    return status; 
  } 
  request_region(ACU_BASE, ACU_SIZE, name); 

#else

  /* Get the parallel port I/O range */ 

  MSG(" - About to request region\n");

  status= !request_region(ACU_BASE, ACU_SIZE, name); 
  if (status) { 
    MSG(" - request for I/O ports 0x%X to 0x%X failed\n",  
	ACU_BASE, ACU_BASE+ACU_SIZE); 
    return status; 
  } else {
    MSG(" - request for I/O ports 0x%X to 0x%X succeeded\n",  
	ACU_BASE, ACU_BASE+ACU_SIZE); 
  }

#endif
	 
  /* Get the parallel port IRQ */ 

  MSG(" - About to request interrupt\n");

#ifdef SHARED
  status = request_irq(ACU_IRQ, gps_irq_handler, 
		       SA_INTERRUPT|SA_SHIRQ, name, deviceId);
#else
  status = request_irq(ACU_IRQ, gps_irq_handler, 
		       SA_INTERRUPT, name, deviceId);
#endif

  if (status) { 
    MSG(" - request for irq %i failed\n", ACU_IRQ); 
    release_region(ACU_BASE, ACU_SIZE); 
    return status; 
  }

  /* Don't enable the interrupt on initialization for now */ 
#if 1 
  else {  

    //    enable_irq(ACU_IRQ);

    /* enable it */ 
    outb(0x00, ACU_DATA); 
    outb(0x10, ACU_CONTROL); 
    
    MSG(" - request for irq %i succeeded\n", ACU_IRQ); 
  }  
#endif 
	 
  /* Allocate the memory buffer */ 
  MSG(" - allocating kernel buffer\n"); 
  gps_buf = (char *)kmalloc(BUFFER_SIZE, GFP_KERNEL); 
  if (!gps_buf) { 
    MSG(" * memory allocation failed\n"); 
    free_irq(ACU_IRQ, deviceId);
    release_region(ACU_BASE, ACU_SIZE); 
    return -ENOMEM; 
  } 
  /* Setup the buffer read/write pointers, and counter */ 
  gps_buf_wp  = gps_buf; 
  gps_buf_rp  = gps_buf; 
  gps_buf_cnt = 0; 
 
  /* Initialize lock */ 
  spin_lock_init(&gps_buf_lock); 
 
  /* Register ourselves as a char device so that the file 
   * operations can be tested. 
   */ 
  major = register_chrdev(0, name, &gps_fops); 
  if (major < 0) { 
    MSG(" * major assignment failed with error %d\n", major); 
    free_irq(ACU_IRQ, deviceId);
    release_region(ACU_BASE, ACU_SIZE); 
    return major; 
  } 
  MSG(" * major = %d\n", major); 
 
  MSG(" * initializing kernel timer\n"); 

#if 0
  // Set up a timer to expire one second after module initialization
  {
    init_timer(&gps_timer); 
    gps_timer.expires = jiffies + 5 * HZ; // five seconds
    gps_timer.function = gps_timer_handler;
    gps_timer.data = 0x0;

    add_timer(&gps_timer);
  }
#endif 

  MSG("Module 'gps' loaded.\n"); 
  return 0; 
} 
 
void gps_exit(void) 
{ 
  MSG("<exit> called.\n"); 
 
  disable_irq(ACU_IRQ);

  free_irq(ACU_IRQ, deviceId);
  release_region(ACU_BASE, ACU_SIZE); 
  unregister_chrdev(major, name); 
 
  MSG(" * removing kernel timer\n"); 
  if(timer_pending(&gps_timer)) { 
    del_timer(&gps_timer); 
  } 
	 
  MSG("Module 'gps' unloaded\n"); 
} 
 
module_init(gps_init); 
module_exit(gps_exit); 
