/*---------------------------------------------------------------------
 * Filename: parport.c
 * Author:   Erik Leitch
 * Date:     Jan 2006
 * Platform: Linux
 * Purpose:  Driver for Symmetricomm bc635-U time and frequency 
 *           processor card
 *---------------------------------------------------------------------
 */

/*---------------------------------------------------------------------
 * Stand-alone module option
 *---------------------------------------------------------------------
 *
 * Uncomment this line to compile this as a standalone option
 * (or define the parameter in the makefile)
 */
#define PARPORT_COMPILE_AS_MODULE

/*---------------------------------------------------------------------
 * Includes
 *---------------------------------------------------------------------
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/version.h>

#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,5,0)
#else
#include <asm-i386/mach-default/irq_vectors_limits.h>
#include <asm-i386/mach-default/irq_vectors.h>
#endif

#include <linux/interrupt.h>	/* IRQ handling - ISRs                */ 

#include <linux/fs.h>		/* File and device operations        */
#include <linux/sched.h>        /* ISR structure definitions         */
#include <linux/pci.h>          /* PCI related macros and functions  */

#include <asm/uaccess.h>        /* get/put_user, copy_to/from_user   */
#include <asm/io.h>             /* ioremap, iounmap, memcpy_to/fromio*/

#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,5,0)
#else
#include <asm-i386/pgtable.h>
#define remap_page_range io_remap_page_range
#endif

#define PARPORT_VENDOR_ID 0x12e2 // vendor ID: First two bytes of lspci -xx output
#define PARPORT_DEVICE_ID 0x4013 // Second two bytes of lspci -xx output

#define PARPORT_MAX_DEVICES 1
#define PARPORT_DEBUG
#define DEBUG

/*--------------------------------------------------------------------------- 
 * Information extractable via `modinfo' command 
 *--------------------------------------------------------------------------- 
 * 
 * Use /sbin/modinfo -p gps_driver.o (i.e., on the driver file)  
 * to see the parameters. 
 *  
 */ 
 
MODULE_DESCRIPTION("Parport PCI/frequency processor driver module"); 
MODULE_AUTHOR("Erik Leitch"); 
 
// Inserting an instance if this macro makes the "tainted module" message go
// away

MODULE_LICENSE("GPL");

#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,5,0)
static char debug = 1; 
MODULE_PARM(debug, "1b"); 
#else
int debug = 1; 
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
 * Each driver can use a locally defined macro, so that debugging can
 * be off by default in parts of the driver that have been debugged.
 */

#ifdef PARPORT_DEBUG
static int parport_debug = 1;
#else
static int parport_debug = 0;
#endif

#ifdef DEBUG
#define MSG(string, args...) if (parport_debug) printk("PARPORT: " string, ##args)
#else
#define MSG(string, args...)
#endif

/*---------------------------------------------------------------------
 * Global parameters
 *---------------------------------------------------------------------
 *
 * Both the PARPORT control registers and DPRAM occupy 4096 bytes
 *
 */
#define CTL_SIZE        0x4000
#define DPRAM_SIZE      0x4000

#ifdef PARPORT_COMPILE_AS_MODULE
static int   parport_major;
#endif

static char *parport_name = "parport";

/* CVS version string (or user supplied) */

static char *parport_ver_str = "$Revision: 1.1.1.1 $";

/*---------------------------------------------------------------------
 * PARPORT Device information
 *---------------------------------------------------------------------
 */

/* 
 * The PARPORT implements at least 2 BARs
 *
 * - BAR0: PARPORT control registers (memory space)
 * - BAR1: DPRAM control interface (memory space)
 *
 * Currently, this driver ONLY implements mmap access to both
 */

/* PARPORT device information */

#define NBAR 2

struct parport_device {
    struct pci_dev *pdv;        /* PCI device info            */

    unsigned long   pbase[NBAR];   /* Physical address of PARPORT bars */
    char           *kbase[NBAR];   /* Kernel virtual address      */
    unsigned long   size[NBAR];    /* PCI region size             */
    
    int             mapped[NBAR]; /* Resource flag               */
    int             bus;        /* PCI bus number              */
    int             devsel;     /* PCI devsel number           */
    void           *user_data;  /* Users driver info           */
};

/*
 * Pointer to the device
 */
static struct parport_device* parport;

//-----------------------------------------------------------------------
// Interrupt routines
//-----------------------------------------------------------------------

#define PARPORT_IRQ 1

#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,5,0)
static void parport_irq_handler(int irq, void *dev_id, struct pt_regs *regs);
#else
static irqreturn_t parport_irq_handler(int irq, void *dev_id, struct pt_regs *regs);
#endif

static int parport_request_irq(void);
static void parport_release_irq(void);


DECLARE_WAIT_QUEUE_HEAD (parport_write_q); 
DECLARE_WAIT_QUEUE_HEAD (parport_read_q); 
 
static char *parport_buf = NULL; 
static char *parport_buf_wp = NULL; 
static char *parport_buf_rp = NULL; 
static int   parport_buf_cnt = 0; 
spinlock_t   parport_buf_lock; 
unsigned char parport_irq;

#define BUFFER_SIZE  (4*PAGE_SIZE) 

static int clientData;

#define SHARED

#ifdef SHARED
static void* dev_id = (void*)&clientData;
#else
static void* dev_id = NULL;
#endif

/*---------------------------------------------------------------------------
 * PARPORT file operations (fops) declarations
 *---------------------------------------------------------------------------
 */

/* Function prototypes */

static int parport_open(struct inode *inode,
		    struct file *file);

static int parport_mmap(struct file *file,
		    struct vm_area_struct *vma);

static int parport_release(struct inode *inode,
		       struct file *file);

/* File operations structure */

static struct file_operations parport_fops = {
  owner:	THIS_MODULE,
  open:		parport_open,
  mmap:		parport_mmap,
  release:	parport_release,
};

/*---------------------------------------------------------------------------
 * Local helper functions
 *---------------------------------------------------------------------------
 */

static int parport_driver_version(void)
{
  int status, major, minor;
  int version = 0;
	
  /* Extract the CVS revision number and return it as an int. */

  status = sscanf(parport_ver_str, "%*s %d.%d %*s", &major, &minor);
  if (status == 2) {
    version = ((major & 0xFF) << 8) | (minor & 0xFF);
  }
  return version;
}

/*---------------------------------------------------------------------------
 * PARPORT externally accessable API
 *---------------------------------------------------------------------------
 */

/* Valid device check */

int parport_device_is_valid(struct parport_device* parport)
{
  int valid = 0;
  if (parport != NULL) {
    valid = 1;
  }
  return valid;
}

/* Return the pci_dev pointer for accessing further PCI resources */

struct pci_dev *parport_pci_dev(struct parport_device* parport)
{
  struct pci_dev *pdv = NULL;
  if (parport != NULL) {
    pdv = parport->pdv;
  }
  return pdv;
}

/* Device physical location */

int parport_pci_bus(struct parport_device* parport)
{
  int bus = 0;
  if (parport != NULL) {
    bus = parport->bus;
  }
  return bus;
}

int parport_pci_devsel(struct parport_device* parport)
{
  int devsel = 0;
  if (parport != NULL) {
    devsel = parport->devsel;
  }
  return devsel;
}

/* User data access */
void parport_set_user_data(struct parport_device* parport, void *user_data)
{
  if (parport != NULL) {
    parport->user_data = user_data;
  }
}

void *parport_get_user_data(struct parport_device* parport)
{
  void *user_data = NULL;
  if (parport != NULL) {
    user_data = parport->user_data;
  }
  return user_data;
}

/*---------------------------------------------------------------------------
 * Device driver file operations (fops) definitions
 *---------------------------------------------------------------------------
 */

static int parport_open(struct inode *inode,
		    struct file *file)
{
  int status=0;

  /* Increment the use count (obsolete in later kernels) */

#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,5,0)
  MOD_INC_USE_COUNT;
#endif
  
  MSG("<parport_open> called by %s (pid = %u).\n",
      current->comm, current->pid);
  
  /* Check if the board exists */

  if(parport == NULL) {
    MSG(" - Open failed. No device.\n");

    /* Decrement the use count (obsolete in later kernels) */
    
#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,5,0)
    MOD_DEC_USE_COUNT;
#endif
    
    return -ENXIO;
  }

  status |= parport_request_irq();

  return status;
}

static int parport_request_irq()
{
  int status;

  MSG("parport: requesting interrupt: %d\n", PARPORT_IRQ);

#ifdef SHARED
    status = request_irq(PARPORT_IRQ, parport_irq_handler, 
			 SA_INTERRUPT|SA_SHIRQ, parport_name, parport->pdv);
#else
    status = request_irq(PARPORT_IRQ, parport_irq_handler, 
			 SA_INTERRUPT, parport_name, parport->pdv);
#endif

    if (status) 
      MSG(" - request for irq %i failed\n", PARPORT_IRQ); 

    return status;
}

void parport_release_irq()
{
  if(parport != NULL)
    free_irq(PARPORT_IRQ, parport->pdv);
}

/* mmap notes:
 *
 * 1. The module usage count does not need to be incremented when an
 *    area of memory is mapped. For example, if you open a file (the
 *    usage count as shown via 'cat /proc/modules' is incrmented to 1),
 *    then mmap an area, then close the file, the module usage count
 *    remains 1 until you munmap the area. That is, the release file
 *    operation is not called on a user-space call to close, it is
 *    called when the usage count drops to zero.
 *
 *    These observations are inconsistent with the comments on
 *    p277 [Rub98], where he states that closing the file descriptor
 *    can lead to the module usage count falling to zero.
 *
 * 2. The VMA size does not need to be a multiple of page sizes, but
 *    the mapping must be.
 *
 * 3. vma->vm_offset is analogous to file->f_pos, i.e., it is an
 *    offset into the device relative to pbase (p272 [Rub98]).
 *    This offset plus the size of the mapped region must lie
 *    within the PCI device's region to be mmapped.
 *
 * 4. (2.2 Kernel) offset must be a multiple of page sizes.
 *
 *    (2.4 change) vma->offset became vma->pgoff
 *    offset was in bytes. pgoff is in pages.
 *    (see mm.h), drivers/char/drm/drmP.h uses a macro
 *    VM_OFFSET(vma) vma->offset    for 2.2
 *    vma->pgoff << PAGE_SHIFT      for 2.4
 *
 *    PAGE_SHIFT is defined in asm/page.h as 12, and
 *    PAGE_SIZE is (1UL << PAGE_SHIFT), i.e., 0x1000
 *    since the offset is always page aligned, there is no need
 *    to check for page alignment (which was necessary in 2.2).
 *
 * 5.  Mappings smaller than a page, or that go over a page should
 *     succeed (if appropriate), they'll just need to take up whole
 *     pages in the VMAs.
 *
 * Testing 7/02:
 *     - user-space mmap call with size = 256 propagates to the driver
 *       as a request for vm_size = 4096 (1 page).
 *     - user-space mmap call with size = 5000 becomes vm_size = 8192.
 *     -> looks like Linux is taking care of the sizes for us.
 *     - user-space mmap of 256 returns a pointer to the start of
 *       a valid page. Looping over all 1024 dwords is ok, looping to
 *       1025th causes core dump. The PARPORT registers were at dword offset
 *       200h, i.e., half-way through the page.
 *
 *       Hmmm, I really want mmap to return a pointer to the registers,
 *       not the start of the page ... how do I get that back to the
 *       user - modify vm_offset?
 *
 *     - the offset is because (pbase % PAGE_SIZE) is non-zero.
 */
static int parport_mmap(struct file *file,
		    struct vm_area_struct *vma)
{
    unsigned long vm_size, vm_offset;
    unsigned long offset=0, rel_offset=0;
    unsigned iBar=0; 

    MSG("<parport_mmap> called\n");

    /* VMA properties */

    vm_size   = vma->vm_end - vma->vm_start;
    vm_offset = vma->vm_pgoff << PAGE_SHIFT;
    
    MSG(" - requested map size is %ld, and offset is %ld.\n",
	vm_size, vm_offset);
    
    /* Increase vm_size to the nearest page */
    
    if (vm_size % PAGE_SIZE) {
	vm_size = (vm_size/PAGE_SIZE+1)*PAGE_SIZE;
	MSG(" - increasing map size to %ld.\n",vm_size);
    }
    
    /* Find the bar in which the requested offset lies */

    for(iBar=0; iBar < NBAR; iBar++) {

      if(vm_offset < (offset + parport->size[iBar]))
	break;

	offset += parport->size[iBar];
    }

    if(iBar == NBAR) {
	MSG(" - requested offset %ld doesn't lie within any recognized bars\n",
	    vm_offset);
	return -EAGAIN;
	
	/* 
	 * Else check that the requested size won't overflow the memory region
	 * for this bar 
	 */

    } else {

	/* The offset relative to the start of this bar */

	rel_offset = vm_offset - offset;

	if((rel_offset + vm_size) > parport->size[iBar]) {
	    MSG(" - requested size (%ld) is too large for memory region %d, which is of size: %ld\n",
		vm_size, iBar, parport->size[iBar]);
	    return -EAGAIN;
	}
    }

    /* Remap the page range */

    MSG(" - mapping memory at offset %ld into bar %d\n",
	rel_offset, iBar);

    if(remap_page_range(vma,
			vma->vm_start,                 /* start of user
						        * memory  */
			parport->pbase[iBar] + rel_offset, /* start of PARPORT regs
							* physical mem */
			vm_size,                       /* size */
			vma->vm_page_prot)) {          /* permissions */
	MSG(" - remap_page_range failed!\n");
	return -EAGAIN;
    }

    MSG(" - region successfully mmapped.\n");

    return 0;
}

static int parport_release(struct inode *inode,
		       struct file *file)
{
  MSG("<parport_release> called.\n");

  /* Decrement the use count (obsolete in later kernels) */
  
#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,5,0)
  MOD_DEC_USE_COUNT;
#endif

  parport_release_irq();

  return 0;
}

/*---------------------------------------------------------------------------
 * Module load/unload declaration and definition
 *---------------------------------------------------------------------------
 *
 * Per device initialization routine. The parport parameter must be passed
 * as a pointer-to-a-pointer-to-a-PARPORT_DEVICE, since you want to adjust
 * the address of the pointer. (Otherwise the pointer is passed by
 * value and you can't return the address of the newly constructed
 * object) - yeah, I did it wrong first and got an oops :)
 *
 * During initialization if some devices are successfully initialized,
 * and a subsequent device fails, then the driver just handles the
 * ones that were successful.
 */
static int parport_device_init(struct pci_dev *pdv);

static int parport_obtain_resources(struct pci_dev* pdv, unsigned iBar, unsigned resourceFlag);

/* resource deallocation for a single device */

static void parport_release_resources(void);

/* The parport_init() routine performs the following operations:
 *
 *  - finds a single PARPORT device on the PCI bus
 *  - calls parport_device_init() to initialize the PARPORT device
 *    structure containing information for that device
 *
 * Optionally (#ifdef PARPORT_COMPILE_AS_MODULE)
 *	- registers a character device driver to provide user-space
 * 	  access to the PARPORT control registers.
 */
int parport_init(void)
{
  struct pci_dev*    pdv = NULL;  /* Needs to start out NULL */
  int status;
  int version;

  MSG("<parport_init> called.\n");

  /* Try registering the character device first, as if this
   * fails, its not worth looking for devices!
   */
#ifdef PARPORT_COMPILE_AS_MODULE

  /* Register the char device and use dynamic major allocation */

  MSG(" - register the device.\n");

  parport_major = register_chrdev(0, parport_name, &parport_fops);
  if (parport_major < 0) {
    MSG(" - major assignment failed with error %d\n", parport_major);
    /* no resources to release */
    return parport_major;
  }

  MSG(" - major = %d\n", parport_major);
#endif /* PARPORT_COMPILE_AS_MODULE */

#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,5,0)
  if(!pci_present()) {
    return -ENODEV;
  }
#endif

  /* 
   * Set the device to NULL.  Memory for the device structure is only
   * allocated if a device exists.
   */
  parport = NULL;

  MSG(" - searching for PARPORT devices...\n");

  /* Search through the PCI bus for PARPORT devices */

  if((pdv = pci_find_device(PARPORT_VENDOR_ID, PARPORT_DEVICE_ID, pdv)) != NULL) {

    /* Initialize the board */

      if((status = parport_device_init(pdv))) {

      /* Initialization of a device failed for some reason */

      printk("Initialization of device failed "\
	     "with status %d.\n", status);
      
      return -ENODEV;
    }

    MSG(" - found board\n");
	
    version = parport_driver_version();


#ifdef PARPORT_COMPILE_AS_MODULE
    printk("%s: module version %d.%d loaded (major = %d).\n", 
	   parport_name, 
	   (version >> 8) & 0xFF,
	   (version & 0xFF),
	   parport_major);

    printk(" - device: bus %2d devsel %2d\n",
	   parport->bus, 
	   parport->devsel);
  }	
#endif
  return 0;
}

/**.......................................................................
 * Create and initialize an instance of a parport_device structure
 * based on the PCI_DEV info passed to us.
 *
 * Return an int so that error codes can propagate back.
 */
static int parport_device_init(struct pci_dev *pdv)
{
    int status=0;
    unsigned iBar;

  MSG("<parport_device_init> called.\n");

  MSG(" - allocate PARPORT_DEVICE structure, ");

  /* Obtain memory and fill in the device info */

  if((parport = (struct parport_device*)
      kmalloc(sizeof(struct parport_device), GFP_KERNEL)) == NULL) {
      MSG(" - kmalloc for parport_device_t failed.\n");
      return -ENOMEM;
  }
  
  MSG("got %p\n",parport);

  memset(parport, 0, sizeof(struct parport_device));

  /* Point to our PCI device info */

  parport->pdv  = pdv;

  /* The purpose of this driver is for control of the PARPORT memory-mapped
   * control registers. These registers are in bars 0-1, and we obtain
   * resources for both.
   *
   * It is then the responsibility of the user driver to check for the
   * application-specific BAR, and acquires other resources such as memory and
   * IRQs.
   */
  for(iBar=0; iBar < NBAR; iBar++) {
      if((status=parport_obtain_resources(pdv, iBar, IORESOURCE_MEM)) != 0)
	  return status;
  }

  /* Electrical location */

  parport->bus    = pdv->bus->number;      /* Physical bus number */
  parport->devsel = PCI_SLOT(pdv->devfn);  /* Devsel number       */

  
  /* Allocate the memory buffer */ 
  MSG(" - allocating kernel buffer\n"); 
  parport_buf = (char *)kmalloc(BUFFER_SIZE, GFP_KERNEL); 
  if (!parport_buf) { 
    MSG(" * memory allocation failed\n"); 
    parport_release_resources();
    return -ENOMEM; 
  } 
  /* Setup the buffer read/write pointers, and counter */ 
  parport_buf_wp  = parport_buf; 
  parport_buf_rp  = parport_buf; 
  parport_buf_cnt = 0; 
 
  /* Initialize lock */ 
  spin_lock_init(&parport_buf_lock); 


  /* Enable the device (new in 2.4)
   *
   * PCI devices should be 'enabled' before any registers
   * are accessed (this probably enables the I/O and mem
   * flags in the PCI control and status registers).
   */
  MSG(" - enabling the device.\n");

  if(pci_enable_device(pdv)) {

      MSG(" - PCI enable for device failed.\n");

    /* Free resources */

    parport_release_resources();
    return -EIO;
  }

  MSG(" - found PARPORT card on bus %d, devsel %d\n",
      parport->bus, parport->devsel);

  for(iBar=0; iBar < NBAR; iBar++) {
      MSG(" - BAR%d physical/kernel = %lx/%p, size = 0x%lx\n", iBar,
	  parport->pbase[iBar], parport->kbase[iBar], parport->size[iBar]);
  }

  return 0;
}

static int parport_obtain_resources(struct pci_dev* pdv, unsigned iBar, unsigned resourceFlag) 
{
    
    unsigned long pci_flags;
    
    MSG(" - obtaining BAR%d resources.\n", iBar);
    
    parport->pbase[iBar] = pci_resource_start(pdv, iBar);
    parport->size[iBar]  =   pci_resource_len(pdv, iBar);
    
    pci_flags  = pci_resource_flags(pdv, iBar);
    
    MSG(" - BAR%d start 0x%X, size %d\n", iBar, (int)parport->pbase[iBar], (int)parport->size[iBar]);
    
    /* Check that this BAR is memory, and then acquire the memory resource */
    
    if ((pci_flags & resourceFlag) && (parport->size[iBar])) {
	if(!request_mem_region(parport->pbase[iBar], parport->size[iBar], parport_name)) {
	    MSG(" - PCI memory request for BAR%d failed.\n", iBar);
	    
	    /* Free resources */
	    
	    parport->size[iBar] = 0;
	    parport_release_resources();
	    
	    return -EIO;
	}
	
	/* 
	 * Map physical address to a kernal virtual address
	 */
	parport->kbase[iBar]  = ioremap(parport->pbase[iBar], parport->size[iBar]);
	parport->mapped[iBar] = 1;
	
    } else {
	
	/* 
	 * If its not memory, or the size is zero,
	 * then something's wrong!
	 */
	parport->size[iBar] = 0;
	parport_release_resources();
	return -ENODEV;
    }
    return 0;
}

/**.......................................................................
 * Release the allocated resources for a device 
 */
static void parport_release_resources()
{
    unsigned iBar;

    if (parport != NULL) {

      parport_release_irq();

	/* PCI device resources */

	for(iBar=0; iBar < NBAR; iBar++) {
	    if(parport->size[iBar]) {
		if(parport->mapped[iBar]) {
		    MSG(" - unmapping I/O memory at kernel address %p\n",
			parport->kbase[iBar]);
		    iounmap(parport->kbase[iBar]);
		}
		MSG(" - releasing I/O memory at physical address %lX\n",
		    parport->pbase[iBar]);
		release_mem_region(parport->pbase[iBar],parport->size[iBar]);
	    }
	}

	/* The device structure */
	
	MSG(" - releasing device memory at %p\n",parport);
	kfree(parport);
	parport = NULL;
    }
}

void parport_exit(void)
{
  MSG("<parport-exit> called.\n");

  /* Release the resources */
  MSG(" - release resources.\n");

  parport_release_resources();

#ifdef PARPORT_COMPILE_AS_MODULE
  /* Unregister the device */

  MSG(" - unregister the device.\n");
  unregister_chrdev(parport_major, parport_name);

  printk("%s: module unloaded\n", parport_name);
#endif /* PARPORT_COMPILE_AS_MODULE */
}

#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,5,0)
static void parport_irq_handler(int irq, void *dev_id, struct pt_regs *regs) 
#else
static irqreturn_t parport_irq_handler(int irq, void *dev_id, struct pt_regs *regs) 
#endif
{ 
  unsigned long flags; 
  int len1, len2; 
  char str[30];    /* shouldn't overflow? */ 
  struct timeval tv; 
 
  MSG("  <irq %d> received!\n", irq); 
 
  /* Format a timestamp */ 
  do_gettimeofday(&tv); 
  len1 = 0; 
  len2 = sprintf(str, "%08u.%06u\n",(int)tv.tv_sec, (int)tv.tv_usec); 
 
  MSG("  - timestamp is %s", str); 
	 
  /* Write the timestamp */ 
 
  spin_lock_irqsave(&parport_buf_lock,flags); 
 
  /* Clear the buffer if its full */ 
 
  if (parport_buf_cnt + len2 > BUFFER_SIZE) { 
    MSG("  - write buffer is full, clearing\n"); 
    parport_buf_cnt = 0; 
    parport_buf_rp = parport_buf; 
    parport_buf_wp = parport_buf; 
  } 
 
  MSG("  - adding %d chars to the buffer.\n", len2); 
 
  parport_buf_cnt += len2; 
 
  /* Wrap the string if necessary */ 
 
  if (parport_buf_wp + len2 >= parport_buf + BUFFER_SIZE) { 
    MSG("  - wrapping string in write buffer\n"); 
    len1 = parport_buf + BUFFER_SIZE - parport_buf_wp; /* first part of str */ 
    strncpy(parport_buf_wp, str, len1); 
    parport_buf_wp = parport_buf; 
    len2 -= len1; /* remaining chars */ 
  } 
  strncpy(parport_buf_wp, &str[len1], len2); /* all or remainder of string */ 
  parport_buf_wp += len2; 
 
  spin_unlock_irqrestore(&parport_buf_lock, flags); 
 
  /* Unblock the read/poll call */ 
 
  wake_up_interruptible(&parport_read_q);  

#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,5,0)
  return; 
#else
  return IRQ_HANDLED;
#endif
} 

#ifdef PARPORT_COMPILE_AS_MODULE
module_init(parport_init);
module_exit(parport_exit);
#endif /* PARPORT_COMPILE_AS_MODULE */


