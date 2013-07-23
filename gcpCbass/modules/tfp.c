/*---------------------------------------------------------------------
 * Filename: tfp.c
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
#define TFP_COMPILE_AS_MODULE

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

#define TFP_VENDOR_ID 0x12e2 // vendor ID: First two bytes of lspci -xx output
#define TFP_DEVICE_ID 0x4013 // Second two bytes of lspci -xx output

#define TFP_MAX_DEVICES 1
#define TFP_DEBUG
#define DEBUG

/*--------------------------------------------------------------------------- 
 * Information extractable via `modinfo' command 
 *--------------------------------------------------------------------------- 
 * 
 * Use /sbin/modinfo -p gps_driver.o (i.e., on the driver file)  
 * to see the parameters. 
 *  
 */ 
 
MODULE_DESCRIPTION("Symmetricomm time/frequency processor driver module"); 
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

#ifdef TFP_DEBUG
static int tfp_debug = 1;
#else
static int tfp_debug = 0;
#endif

#ifdef DEBUG
#define MSG(string, args...) if (tfp_debug) printk("TFP: " string, ##args)
#else
#define MSG(string, args...)
#endif

/*---------------------------------------------------------------------
 * Global parameters
 *---------------------------------------------------------------------
 *
 * Both the TFP control registers and DPRAM occupy 4096 bytes
 *
 */
#define CTL_SIZE        0x4000
#define DPRAM_SIZE      0x4000

#ifdef TFP_COMPILE_AS_MODULE
static int   tfp_major;
#endif

static char *tfp_name = "tfp";

/* CVS version string (or user supplied) */

static char *tfp_ver_str = "$Revision: 1.1.1.1 $";

static int clientData;

#define SHARED

#ifdef SHARED
static void* deviceId = (void*)&clientData;
#else
static void* deviceId = NULL;
#endif

/*---------------------------------------------------------------------
 * TFP Device information
 *---------------------------------------------------------------------
 */

/* 
 * The TFP implements at least 2 BARs
 *
 * - BAR0: TFP control registers (memory space)
 * - BAR1: DPRAM control interface (memory space)
 *
 * Currently, this driver ONLY implements mmap access to both
 */

/* TFP device information */

#define NBAR 2

struct tfp_device {
    struct pci_dev *pdv;        /* PCI device info            */

    unsigned long   pbase[NBAR];   /* Physical address of TFP bars */
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
static struct tfp_device* tfp;

//-----------------------------------------------------------------------
// Interrupt routines
//-----------------------------------------------------------------------

#define TFP_IRQ 1

#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,5,0)
static void tfp_irq_handler(int irq, void *dev_id, struct pt_regs *regs);
#else
static irqreturn_t tfp_irq_handler(int irq, void *dev_id, struct pt_regs *regs);
#endif

static int tfp_request_irq(void);
static void tfp_release_irq(void);


DECLARE_WAIT_QUEUE_HEAD (tfp_write_q); 
DECLARE_WAIT_QUEUE_HEAD (tfp_read_q); 
 
static char *tfp_buf = NULL; 
static char *tfp_buf_wp = NULL; 
static char *tfp_buf_rp = NULL; 
static int   tfp_buf_cnt = 0; 
spinlock_t   tfp_buf_lock; 
unsigned char tfp_irq;

#define BUFFER_SIZE  (4*PAGE_SIZE) 

/*---------------------------------------------------------------------------
 * TFP file operations (fops) declarations
 *---------------------------------------------------------------------------
 */

/* Function prototypes */

static int tfp_open(struct inode *inode,
		    struct file *file);

static int tfp_mmap(struct file *file,
		    struct vm_area_struct *vma);

static int tfp_release(struct inode *inode,
		       struct file *file);

/* File operations structure */

static struct file_operations tfp_fops = {
  owner:	THIS_MODULE,
  open:		tfp_open,
  mmap:		tfp_mmap,
  release:	tfp_release,
};

/*---------------------------------------------------------------------------
 * Local helper functions
 *---------------------------------------------------------------------------
 */

static int tfp_driver_version(void)
{
  int status, major, minor;
  int version = 0;
	
  /* Extract the CVS revision number and return it as an int. */

  status = sscanf(tfp_ver_str, "%*s %d.%d %*s", &major, &minor);
  if (status == 2) {
    version = ((major & 0xFF) << 8) | (minor & 0xFF);
  }
  return version;
}

/*---------------------------------------------------------------------------
 * TFP externally accessable API
 *---------------------------------------------------------------------------
 */

/* Valid device check */

int tfp_device_is_valid(struct tfp_device* tfp)
{
  int valid = 0;
  if (tfp != NULL) {
    valid = 1;
  }
  return valid;
}

/* Return the pci_dev pointer for accessing further PCI resources */

struct pci_dev *tfp_pci_dev(struct tfp_device* tfp)
{
  struct pci_dev *pdv = NULL;
  if (tfp != NULL) {
    pdv = tfp->pdv;
  }
  return pdv;
}

/* Device physical location */

int tfp_pci_bus(struct tfp_device* tfp)
{
  int bus = 0;
  if (tfp != NULL) {
    bus = tfp->bus;
  }
  return bus;
}

int tfp_pci_devsel(struct tfp_device* tfp)
{
  int devsel = 0;
  if (tfp != NULL) {
    devsel = tfp->devsel;
  }
  return devsel;
}

/* User data access */
void tfp_set_user_data(struct tfp_device* tfp, void *user_data)
{
  if (tfp != NULL) {
    tfp->user_data = user_data;
  }
}

void *tfp_get_user_data(struct tfp_device* tfp)
{
  void *user_data = NULL;
  if (tfp != NULL) {
    user_data = tfp->user_data;
  }
  return user_data;
}

/*---------------------------------------------------------------------------
 * Device driver file operations (fops) definitions
 *---------------------------------------------------------------------------
 */

static int tfp_open(struct inode *inode,
		    struct file *file)
{
  int status=0;

  /* Increment the use count (obsolete in later kernels) */

#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,5,0)
  MOD_INC_USE_COUNT;
#endif
  
  MSG("<tfp_open> called by %s (pid = %u).\n",
      current->comm, current->pid);
  
  /* Check if the board exists */

  if(tfp == NULL) {
    MSG(" - Open failed. No device.\n");

    /* Decrement the use count (obsolete in later kernels) */
    
#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,5,0)
    MOD_DEC_USE_COUNT;
#endif
    
    return -ENXIO;
  }

  status |= tfp_request_irq();

  return status;
}

static int tfp_request_irq()
{
  int status;

  MSG("tfp: requesting interrupt: %d\n", TFP_IRQ);

#ifdef SHARED
    status = request_irq(TFP_IRQ, tfp_irq_handler, 
			 SA_INTERRUPT|SA_SHIRQ, tfp_name, tfp->pdv);
#else
    status = request_irq(TFP_IRQ, tfp_irq_handler, 
			 SA_INTERRUPT, tfp_name, tfp->pdv);
#endif

    if (status) 
      MSG(" - request for irq %i failed\n", TFP_IRQ); 

    return status;
}

void tfp_release_irq()
{
  if(tfp != NULL)
    free_irq(TFP_IRQ, tfp->pdv);
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
 *       1025th causes core dump. The TFP registers were at dword offset
 *       200h, i.e., half-way through the page.
 *
 *       Hmmm, I really want mmap to return a pointer to the registers,
 *       not the start of the page ... how do I get that back to the
 *       user - modify vm_offset?
 *
 *     - the offset is because (pbase % PAGE_SIZE) is non-zero.
 */
static int tfp_mmap(struct file *file,
		    struct vm_area_struct *vma)
{
    unsigned long vm_size, vm_offset;
    unsigned long offset=0, rel_offset=0;
    unsigned iBar=0; 

    MSG("<tfp_mmap> called\n");

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

      if(vm_offset < (offset + tfp->size[iBar]))
	break;

	offset += tfp->size[iBar];
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

	if((rel_offset + vm_size) > tfp->size[iBar]) {
	    MSG(" - requested size (%ld) is too large for memory region %d, which is of size: %ld\n",
		vm_size, iBar, tfp->size[iBar]);
	    return -EAGAIN;
	}
    }

    /* Remap the page range */

    MSG(" - mapping memory at offset %ld into bar %d\n",
	rel_offset, iBar);

    if(remap_page_range(vma,
			vma->vm_start,                 /* start of user
						        * memory  */
			tfp->pbase[iBar] + rel_offset, /* start of TFP regs
							* physical mem */
			vm_size,                       /* size */
			vma->vm_page_prot)) {          /* permissions */
	MSG(" - remap_page_range failed!\n");
	return -EAGAIN;
    }

    MSG(" - region successfully mmapped.\n");

    return 0;
}

static int tfp_release(struct inode *inode,
		       struct file *file)
{
  MSG("<tfp_release> called.\n");

  /* Decrement the use count (obsolete in later kernels) */
  
#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,5,0)
  MOD_DEC_USE_COUNT;
#endif

  tfp_release_irq();

  return 0;
}

/*---------------------------------------------------------------------------
 * Module load/unload declaration and definition
 *---------------------------------------------------------------------------
 *
 * Per device initialization routine. The tfp parameter must be passed
 * as a pointer-to-a-pointer-to-a-TFP_DEVICE, since you want to adjust
 * the address of the pointer. (Otherwise the pointer is passed by
 * value and you can't return the address of the newly constructed
 * object) - yeah, I did it wrong first and got an oops :)
 *
 * During initialization if some devices are successfully initialized,
 * and a subsequent device fails, then the driver just handles the
 * ones that were successful.
 */
static int tfp_device_init(struct pci_dev *pdv);

static int tfp_obtain_resources(struct pci_dev* pdv, unsigned iBar, unsigned resourceFlag);

/* resource deallocation for a single device */

static void tfp_release_resources(void);

/* The tfp_init() routine performs the following operations:
 *
 *  - finds a single TFP device on the PCI bus
 *  - calls tfp_device_init() to initialize the TFP device
 *    structure containing information for that device
 *
 * Optionally (#ifdef TFP_COMPILE_AS_MODULE)
 *	- registers a character device driver to provide user-space
 * 	  access to the TFP control registers.
 */
int tfp_init(void)
{
  struct pci_dev*    pdv = NULL;  /* Needs to start out NULL */
  int status;
  int version;

  MSG("<tfp_init> called.\n");

  /* Try registering the character device first, as if this
   * fails, its not worth looking for devices!
   */
#ifdef TFP_COMPILE_AS_MODULE

  /* Register the char device and use dynamic major allocation */

  MSG(" - register the device.\n");

  tfp_major = register_chrdev(0, tfp_name, &tfp_fops);
  if (tfp_major < 0) {
    MSG(" - major assignment failed with error %d\n", tfp_major);
    /* no resources to release */
    return tfp_major;
  }

  MSG(" - major = %d\n", tfp_major);
#endif /* TFP_COMPILE_AS_MODULE */

#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,5,0)
  if(!pci_present()) {
    return -ENODEV;
  }
#endif

  /* 
   * Set the device to NULL.  Memory for the device structure is only
   * allocated if a device exists.
   */
  tfp = NULL;

  MSG(" - searching for TFP devices...\n");

  /* Search through the PCI bus for TFP devices */

  if((pdv = pci_find_device(TFP_VENDOR_ID, TFP_DEVICE_ID, pdv)) != NULL) {

    /* Initialize the board */

      if((status = tfp_device_init(pdv))) {

      /* Initialization of a device failed for some reason */

      printk("Initialization of device failed "\
	     "with status %d.\n", status);
      
      return -ENODEV;
    }

    MSG(" - found board\n");
	
    version = tfp_driver_version();


#ifdef TFP_COMPILE_AS_MODULE
    printk("%s: module version %d.%d loaded (major = %d).\n", 
	   tfp_name, 
	   (version >> 8) & 0xFF,
	   (version & 0xFF),
	   tfp_major);

    printk(" - device: bus %2d devsel %2d\n",
	   tfp->bus, 
	   tfp->devsel);
  }	
#endif
  return 0;
}

/**.......................................................................
 * Create and initialize an instance of a tfp_device structure
 * based on the PCI_DEV info passed to us.
 *
 * Return an int so that error codes can propagate back.
 */
static int tfp_device_init(struct pci_dev *pdv)
{
    int status=0;
    unsigned iBar;

  MSG("<tfp_device_init> called.\n");

  MSG(" - allocate TFP_DEVICE structure, ");

  /* Obtain memory and fill in the device info */

  if((tfp = (struct tfp_device*)
      kmalloc(sizeof(struct tfp_device), GFP_KERNEL)) == NULL) {
      MSG(" - kmalloc for tfp_device_t failed.\n");
      return -ENOMEM;
  }
  
  MSG("got %p\n",tfp);

  memset(tfp, 0, sizeof(struct tfp_device));

  /* Point to our PCI device info */

  tfp->pdv  = pdv;

  /* The purpose of this driver is for control of the TFP memory-mapped
   * control registers. These registers are in bars 0-1, and we obtain
   * resources for both.
   *
   * It is then the responsibility of the user driver to check for the
   * application-specific BAR, and acquires other resources such as memory and
   * IRQs.
   */
  for(iBar=0; iBar < NBAR; iBar++) {
      if((status=tfp_obtain_resources(pdv, iBar, IORESOURCE_MEM)) != 0)
	  return status;
  }

  /* Electrical location */

  tfp->bus    = pdv->bus->number;      /* Physical bus number */
  tfp->devsel = PCI_SLOT(pdv->devfn);  /* Devsel number       */

  
  /* Allocate the memory buffer */ 
  MSG(" - allocating kernel buffer\n"); 
  tfp_buf = (char *)kmalloc(BUFFER_SIZE, GFP_KERNEL); 
  if (!tfp_buf) { 
    MSG(" * memory allocation failed\n"); 
    tfp_release_resources();
    return -ENOMEM; 
  } 
  /* Setup the buffer read/write pointers, and counter */ 
  tfp_buf_wp  = tfp_buf; 
  tfp_buf_rp  = tfp_buf; 
  tfp_buf_cnt = 0; 
 
  /* Initialize lock */ 
  spin_lock_init(&tfp_buf_lock); 


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

    tfp_release_resources();
    return -EIO;
  }

  MSG(" - found TFP card on bus %d, devsel %d\n",
      tfp->bus, tfp->devsel);

  for(iBar=0; iBar < NBAR; iBar++) {
      MSG(" - BAR%d physical/kernel = %lx/%p, size = 0x%lx\n", iBar,
	  tfp->pbase[iBar], tfp->kbase[iBar], tfp->size[iBar]);
  }

  return 0;
}

static int tfp_obtain_resources(struct pci_dev* pdv, unsigned iBar, unsigned resourceFlag) 
{
    
    unsigned long pci_flags;
    
    MSG(" - obtaining BAR%d resources.\n", iBar);
    
    tfp->pbase[iBar] = pci_resource_start(pdv, iBar);
    tfp->size[iBar]  =   pci_resource_len(pdv, iBar);
    
    pci_flags  = pci_resource_flags(pdv, iBar);
    
    MSG(" - BAR%d start 0x%X, size %d\n", iBar, (int)tfp->pbase[iBar], (int)tfp->size[iBar]);
    
    /* Check that this BAR is memory, and then acquire the memory resource */
    
    if ((pci_flags & resourceFlag) && (tfp->size[iBar])) {
	if(!request_mem_region(tfp->pbase[iBar], tfp->size[iBar], tfp_name)) {
	    MSG(" - PCI memory request for BAR%d failed.\n", iBar);
	    
	    /* Free resources */
	    
	    tfp->size[iBar] = 0;
	    tfp_release_resources();
	    
	    return -EIO;
	}
	
	/* 
	 * Map physical address to a kernal virtual address
	 */
	tfp->kbase[iBar]  = ioremap(tfp->pbase[iBar], tfp->size[iBar]);
	tfp->mapped[iBar] = 1;
	
    } else {
	
	/* 
	 * If its not memory, or the size is zero,
	 * then something's wrong!
	 */
	tfp->size[iBar] = 0;
	tfp_release_resources();
	return -ENODEV;
    }
    return 0;
}

/**.......................................................................
 * Release the allocated resources for a device 
 */
static void tfp_release_resources()
{
    unsigned iBar;

    if (tfp != NULL) {

      tfp_release_irq();

	/* PCI device resources */

	for(iBar=0; iBar < NBAR; iBar++) {
	    if(tfp->size[iBar]) {
		if(tfp->mapped[iBar]) {
		    MSG(" - unmapping I/O memory at kernel address %p\n",
			tfp->kbase[iBar]);
		    iounmap(tfp->kbase[iBar]);
		}
		MSG(" - releasing I/O memory at physical address %lX\n",
		    tfp->pbase[iBar]);
		release_mem_region(tfp->pbase[iBar],tfp->size[iBar]);
	    }
	}

	/* The device structure */
	
	MSG(" - releasing device memory at %p\n",tfp);
	kfree(tfp);
	tfp = NULL;
    }
}

void tfp_exit(void)
{
  MSG("<tfp-exit> called.\n");

  /* Release the resources */
  MSG(" - release resources.\n");

  tfp_release_resources();

#ifdef TFP_COMPILE_AS_MODULE
  /* Unregister the device */

  MSG(" - unregister the device.\n");
  unregister_chrdev(tfp_major, tfp_name);

  printk("%s: module unloaded\n", tfp_name);
#endif /* TFP_COMPILE_AS_MODULE */
}

#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,5,0)
static void tfp_irq_handler(int irq, void *dev_id, struct pt_regs *regs) 
#else
static irqreturn_t tfp_irq_handler(int irq, void *dev_id, struct pt_regs *regs) 
#endif
{ 
  unsigned long flags; 
  int len1, len2; 
  char str[30];    /* shouldn't overflow? */ 
  struct timeval tv; 
 
  MSG("  <irq %d> received!\n", irq); 
 
#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,5,0)
  if(dev_id != (void*)tfp->pdv)
    return;
#else
  if(dev_id != (void*)tfp->pdv)
    return IRQ_NONE;
#endif

  /* Format a timestamp */ 
  do_gettimeofday(&tv); 
  len1 = 0; 
  len2 = sprintf(str, "%08u.%06u\n",(int)tv.tv_sec, (int)tv.tv_usec); 
 
  MSG("  - timestamp is %s", str); 
	 
  /* Write the timestamp */ 
 
  spin_lock_irqsave(&tfp_buf_lock,flags); 
 
  /* Clear the buffer if its full */ 
 
  if (tfp_buf_cnt + len2 > BUFFER_SIZE) { 
    MSG("  - write buffer is full, clearing\n"); 
    tfp_buf_cnt = 0; 
    tfp_buf_rp = tfp_buf; 
    tfp_buf_wp = tfp_buf; 
  } 
 
  MSG("  - adding %d chars to the buffer.\n", len2); 
 
  tfp_buf_cnt += len2; 
 
  /* Wrap the string if necessary */ 
 
  if (tfp_buf_wp + len2 >= tfp_buf + BUFFER_SIZE) { 
    MSG("  - wrapping string in write buffer\n"); 
    len1 = tfp_buf + BUFFER_SIZE - tfp_buf_wp; /* first part of str */ 
    strncpy(tfp_buf_wp, str, len1); 
    tfp_buf_wp = tfp_buf; 
    len2 -= len1; /* remaining chars */ 
  } 
  strncpy(tfp_buf_wp, &str[len1], len2); /* all or remainder of string */ 
  tfp_buf_wp += len2; 
 
  spin_unlock_irqrestore(&tfp_buf_lock, flags); 
 
  /* Unblock the read/poll call */ 
 
  wake_up_interruptible(&tfp_read_q);  

#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,5,0)
  return; 
#else
  return IRQ_HANDLED;
#endif
} 

#ifdef TFP_COMPILE_AS_MODULE
module_init(tfp_init);
module_exit(tfp_exit);
#endif /* TFP_COMPILE_AS_MODULE */


