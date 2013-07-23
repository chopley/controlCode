/*---------------------------------------------------------------------
 * Filename: pmac_driver.c
 * Author:   Erik Leitch
 * Date:     July-2002
 * Platform: Linux
 * Purpose:  PMAC-9054 device driver
 *---------------------------------------------------------------------
 * Summary:
 * --------
 *
 * This file implements the file operations required to access the
 * control register regions of the PMAC-9054 Master/Target.
 *
 * The PMAC-9054 implements its control registers as memory at PCI BAR0
 * or I/O at PCI BAR1. This driver provides read, write, lseek, and
 * mmap file operations for the PMAC-9054 memory region.
 *
 * A typical use of this driver is to access the control registers on
 * the PMAC-9054 from user space. Read/write/lseek allow the registers
 * to be accessed using command line tools (which use read/write
 * typically), while mmap allows a user-space program to map the
 * device registers into the local process for direct manipulation via
 * pointers.
 *
 * The PMAC-9054 PCI Configuration space can be read via an IOCTL
 * call. The PMAC-9054 Configuration space extension registers contain
 * power-management, and EEPROM programming features that can also
 * be accessed via IOCTL calls.
 *
 * The driver has a unique major number, so any PMAC-9054 that the 
 * driver finds is assigned a unique device index. To allow the user 
 * to remap the concept of a PCI slot number, which is then encoded 
 * in the device node entry in /dev, the driver structure carries 
 * a 'slot' member, which is initialized to 'index' during driver 
 * load. ioctl calls are then made available so that the user can
 * setup the slot number as desired for a particular cPCI crate
 * and CPU combination.
 * 
 * Interrupt handling is done by the handlers in user drivers.
 * Helper functions specific to the PMAC-9054 are located in this file.
 *
 * Use 'cat /proc/ksyms | grep pmac' to see the functions exported
 * by this module (they'll match those defined in pmac_driver.h).
 *
 *---------------------------------------------------------------------
 * Device indexing verus slot numbering 
 * ------------------------------------
 *
 * The device 'index' is zero-based and is assigned to a device
 * as it is found on the PCI bus. The device 'slot' is associated
 * with the minor numbering sequence of the devices in the /dev 
 * directory. The device index and device slot numbers are separated 
 * so that the user can re-order /dev entries to match mechanical 
 * slot numbering, eg. 1 to 6 from left to right, whereas the driver
 * may find the devices from right to left. When the driver loads,
 * the device index and slots are the same. The user then calls
 * ioctl-calls to associate a particular slot number with a 
 * device bus-dev-fn triple (a PCI slot number).
 * 
 * The /dev device nodes encode the slot number. To ensure that
 * a device is always visible to the user, the device node
 * associated with minor number zero, eg. /dev/pmac0, always exists. 
 * If this devices slot number is reassigned to non-zero, then it 
 * can be opened via two device nodes, 0 and its assigned slot 
 * number.
 * 
 * Device naming is arbitrary, the driver only sees the minor
 * number, so 
 *
 * mknod /dev/pmac0 c 254  0
 * mknod /dev/pmac1 c 254  1
 * mknod /dev/pmac2 c 254  2 ...
 *
 * mknod /dev/pmac9054_00 c 254  0
 * mknod /dev/pmac9054_01 c 254  1
 * mknod /dev/pmac9054_02 c 254  2 ...
 *
 * identify the same devices. The recommended method of naming
 * devices is as follows. Assuming the PMAC major is 254 and
 * the user major is 253, and the user boards are called COBRA,
 * the following names could be used:
 *
 * mknod /dev/cobra_pmac0 c 254  0
 * mknod /dev/cobra_pmac1 c 254  1
 * mknod /dev/cobra_pmac2 c 254  2 ...
 * 
 * mknod /dev/cobra_control0 c 253  0
 * mknod /dev/cobra_data0    c 253  1
 * mknod /dev/cobra_control1 c 253  2
 * mknod /dev/cobra_data1    c 253  3 ...
 * 
 * where cobra_controlN and cobra_dataN are devices on a single 
 * board, and N = 0, 1, ... are the devices on multiple boards
 * (i.e., each board uses 1-bit of the minor number to indicate
 * the device accessed on that board). Note that for this
 * example, the user driver can support a maximum of 128 boards.
 * 
 *---------------------------------------------------------------------
 * Test:
 * -----
 *
 * The entire PMAC-9054 register contents can be viewed via:
 *
 *     od -Ax4 -tx4 -w16 -v -N256 /dev/cobra_pmac1
 *
 *---------------------------------------------------------------------
 * References:
 *
 * [Pom99] "The Linux Kernel Module Programming Guide", O. Pomerantz,
 *          1999.
 * [Rub98] "Linux Device Drivers", A. Rubini, 1998.
 * [Rub01] "Linux Device Drivers", 2nd Ed, A. Rubini and J. Corbet,
 *          2001.
 * [Mat99] "Beginning Linux Programming", N. Matthew and R. Stones,
 *          1999
 *
 *---------------------------------------------------------------------
 */

/*---------------------------------------------------------------------
 * Stand-alone module option
 *---------------------------------------------------------------------
 *
 * Uncomment this line to compile this as a standalone option
 * (or define the parameter in the makefile)
 */
#define PMAC_COMPILE_AS_MODULE

/*---------------------------------------------------------------------
 * Includes
 *---------------------------------------------------------------------
 *
 * See pp5-6 [Pom99] and Ch.21, p793 [Mat99]
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/version.h>

#include <linux/fs.h>			/* file and device operations        */
#include <linux/sched.h>        /* ISR structure definitions         */
#include <linux/pci.h>          /* PCI related macros and functions  */

#include <asm/uaccess.h>        /* get/put_user, copy_to/from_user   */
#include <asm/io.h>             /* ioremap, iounmap, memcpy_to/fromio*/

#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,5,0)
#else
#include <asm-i386/pgtable.h>
#define remap_page_range io_remap_page_range
#endif

#define PMAC_VENDOR_ID 0x1172 // (Altera vendor ID) First two bytes of lspci -xx output
#define PMAC_DEVICE_ID 0x0001 // Second two bytes of lspci -xx output

#define PMAC_MAX_DEVICES 1
#define PMAC_DEBUG
#define DEBUG

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

#ifdef PMAC_DEBUG
static int pmac_debug = 1;
#else
static int pmac_debug = 0;
#endif

#ifdef DEBUG
#define MSG(string, args...) if (pmac_debug) printk("PMAC: " string, ##args)
#else
#define MSG(string, args...)
#endif

/*---------------------------------------------------------------------
 * Global parameters
 *---------------------------------------------------------------------
 */
/* The PMAC-9054 control registers take 256 locations;
 * i.e., 64 32-bit words
 */
#define PMAC_SIZE        0x4000

#ifdef PMAC_COMPILE_AS_MODULE
static int   pmac_major;
#endif
static char *pmac_name = "pmac";

/* CVS version string (or user supplied) */

static char *pmac_ver_str = "$Revision: 1.1.1.1 $";

/*---------------------------------------------------------------------
 * PMAC-9054 Device information
 *---------------------------------------------------------------------
 */

/* The TURBO PMAC implements 2 of the 6 possible BARs
 *
 * - BAR0: PMAC control registers (memory space)
 * - BAR1: PMAC control registers (I/O space)
 *
 * This driver ONLY implements access to BAR0.
 */

/* PMAC device information */
struct pmac_device {
  struct pci_dev *pdv;       /* PCI device info             */
  unsigned long   pbase;     /* Physical address            */
  char           *kbase;     /* Kernel virtual address      */
  unsigned long   size;      /* PCI region size             */
  int             mapped;    /* Resource flag               */
  int             index;     /* Device index                */
  int             slot;      /* Slot number                 */
  int             bus;       /* PCI bus number              */
  int             devsel;    /* PCI devsel number           */
  void           *user_data; /* Users driver info           */
};

/* Array of pointers to device info for every device in the system.
 *
 *  - allocation for the structures occurs in init.
 *  - slots with no device found will have (pmac_devices[index] == NULL).
 *  - open only succeeds for devices that exist.
 *
 *  The 'index' member of PMAC_DEVICE indicates the index into this
 *  array. (The 'index' is provided for debugging, usually, the
 *  'slot' member is used for device access/control).
 *
 *  A fixed array is used, since 256 is the maximum possible,
 *  and user drivers are likely to support less nodes.
 */
static struct pmac_device* pmac_devices[PMAC_MAX_DEVICES];
static unsigned int pmac_device_count = 0;

/* When compiled as part of another driver, the minor number
 * will be split between device nodes on each board. This
 * integer represents the number of consecutive minor numbers
 * used per device. Its used when calculating the slot number
 * where it assumed that slot = minor/block_size;
 */
static int pmac_minor_block = 1;

/*---------------------------------------------------------------------------
 * PMAC-9054 file operations (fops) declarations
 *---------------------------------------------------------------------------
 */

/* Function prototypes */
static loff_t pmac_llseek(
			  struct file *file,
			  loff_t offset,
			  int mode);
static ssize_t pmac_read(
			 struct file *file,
			 char *buf,
			 size_t count,
			 loff_t *offset);
static ssize_t pmac_write(
			  struct file *file,
			  const char *buf,
			  size_t count,
			  loff_t *offset);
static int pmac_ioctl(
		      struct inode *inode,
		      struct file *file,
		      unsigned int cmd,
		      unsigned long arg);
static int pmac_mmap(
		     struct file *file,
		     struct vm_area_struct *vma);
static int pmac_open(
		     struct inode *inode,
		     struct file *file);
static int pmac_release(
			struct inode *inode,
			struct file *file);

/* File operations structure (not static, as its used by COBRA) */
struct file_operations pmac_fops = {
  owner:	THIS_MODULE,
  llseek:	pmac_llseek,
  read:		pmac_read,
  write:	pmac_write,
  ioctl:	pmac_ioctl,
  mmap:		pmac_mmap,
  open:		pmac_open,
  release:	pmac_release,
};

/*---------------------------------------------------------------------------
 * Local helper functions
 *---------------------------------------------------------------------------
 */

/* Get the device with a given slot number (/dev nodes encode 'slot') */
static struct pmac_device* pmac_device_by_slot(int slot)
{
  int i = 0;
  struct pmac_device* pmac = NULL;
  
  /* Request for slot 0 always returns device at index 0. */
  if (slot == 0) {
    pmac = pmac_devices[0];
  } else {
    /* Search for 'slot' */
    while (pmac_devices[i] != NULL) {
      if (pmac_devices[i]->slot == slot) {
	pmac = pmac_devices[i];
	break;
      }
      i++;
    }
  }
  return pmac;
}

/* Provide this as a function, as opposed to a macro in case
 * this mapping needs to be 'dynamic' (controlled through an
 * ioctl call).
 */
static int pmac_minor_to_slot(int minor)
{
  return (int)(minor/pmac_minor_block);
}

static int pmac_driver_version(void)
{
  int status, major, minor;
  int version = 0;
	
  /* Extract the CVS revision number and return it as an int. */

  status = sscanf(pmac_ver_str, "%*s %d.%d %*s", &major, &minor);
  if (status == 2) {
    version = ((major & 0xFF) << 8) | (minor & 0xFF);
  }
  return version;
}

/*---------------------------------------------------------------------------
 * PMAC externally accessable API
 *---------------------------------------------------------------------------
 */

/* Valid device check */

int pmac_device_is_valid(struct pmac_device* pmac)
{
  int valid = 0;
  if (pmac != NULL) {
    valid = 1;
  }
  return valid;
}

/* Return the pci_dev pointer for accessing further PCI resources */

struct pci_dev *pmac_pci_dev(struct pmac_device* pmac)
{
  struct pci_dev *pdv = NULL;
  if (pmac != NULL) {
    pdv = pmac->pdv;
  }
  return pdv;
}

/* Device physical location */

int pmac_pci_bus(struct pmac_device* pmac)
{
  int bus = 0;
  if (pmac != NULL) {
    bus = pmac->bus;
  }
  return bus;
}

int pmac_pci_devsel(struct pmac_device* pmac)
{
  int devsel = 0;
  if (pmac != NULL) {
    devsel = pmac->devsel;
  }
  return devsel;
}

/* ISRs need to access the PMAC registers. These functions 
 * provide read/write access to the registers. Alternatively,
 * implementing an API for controlling PMAC registers would
 * eliminate the need for this function.
 */
/* PMAC register access (reg = BYTE address offset) */
unsigned int pmac_readb(struct pmac_device* pmac, unsigned int reg)
{
  return readb((char *)pmac->kbase+reg);
}
unsigned int pmac_readw(struct pmac_device* pmac, unsigned int reg)
{
  return readw((char *)pmac->kbase+reg);
}
unsigned int pmac_readl(struct pmac_device* pmac, unsigned int reg)
{
  return readl((char *)pmac->kbase+reg);
}

void pmac_writeb(struct pmac_device* pmac, unsigned int reg, unsigned int val)
{
  writeb(val, (char *)pmac->kbase+reg);
}
void pmac_writew(struct pmac_device* pmac, unsigned int reg, unsigned int val)
{
  writew(val, (char *)pmac->kbase+reg);
}
void pmac_writel(struct pmac_device* pmac, unsigned int reg, unsigned int val)
{
  writel(val, (char *)pmac->kbase+reg);
}

/* When compiled as part of a user driver, the user driver
 * needs to setup the spacing between device nodes per board.
 */
void pmac_minor_block_size(int size)
{
  pmac_minor_block = size;
}

/* User data access */
void pmac_set_user_data(struct pmac_device* pmac, void *user_data)
{
  if (pmac != NULL) {
    pmac->user_data = user_data;
  }
}

void *pmac_get_user_data(struct pmac_device* pmac)
{
  void *user_data = NULL;
  if (pmac != NULL) {
    user_data = pmac->user_data;
  }
  return user_data;
}

/*---------------------------------------------------------------------------
 * Device driver file operations (fops) definitions
 *---------------------------------------------------------------------------
 */

static loff_t pmac_llseek(
			  struct file *file,
			  loff_t offset,
			  int mode)
{
  switch(mode)
  {
  case 0:                         /* SEEK_SET */
    file->f_pos = offset;
    return file->f_pos;
  case 1:                         /* SEEK_CUR */
    file->f_pos += offset;
    return file->f_pos;
  case 2:                         /* SEEK_END */
    file->f_pos = PMAC_SIZE + offset;
    return file->f_pos;
  default:                        /* Cannot happen */
    return -EINVAL;
  }
}

static int pmac_ioctl(
		      struct inode *inode,
		      struct file *file,
		      unsigned int cmd,
		      unsigned long arg)
{
  return 0;
}

/* NOTE:
 *
 * The following implementations of read and write use an intermediate
 * kernel buffer to read/write data from/to the PMAC-9054 registers
 * (using memcpy_fromio/memcpy_toio), and then copies the result to/from
 * user-space (using copy_to_user/copy_from_user).
 *
 * The kernel buffer could be eliminated using kiobufs. Kiobuf's
 * allow the mapping of the user-space buffer into the kernel
 * address space, allowing transfers between the user-space
 * buffer and device memory directly. However, since access
 * to the PMAC registers by user-space code is not time critical,
 * this buffering is not really an issue.
 *
 */
static ssize_t pmac_read(
			 struct file *file,
			 char *buf,
			 size_t count,
			 loff_t *offset)
{
  struct pmac_device* pmac;
  int slot;
  char *kbuf;

  /* Find the slot */
  slot  = pmac_minor_to_slot(MINOR(file->f_dentry->d_inode->i_rdev));

  MSG("<pmac_read> called with slot %d, count %d.\n",
      slot, count);

  /* Device specific info */
  pmac = pmac_device_by_slot(slot);

  /* Check we haven't reached the EOF */
  if (file->f_pos >= PMAC_SIZE) {
    return 0;
  }

  MSG("<pmac_read> called to read from 0x%X\n", (int)(pmac->kbase + file->f_pos));

  /* Don't allow reads larger than the register region.
   * But since 'od -Ax4 -tx4 -w16 -v -N256 /dev/pmac1' will
   * request 4096, just reduce the count request, rather
   * than outright denying it.
   */
  if (count > PMAC_SIZE) {
    count = PMAC_SIZE;
  }

  /* Check for reads past the end of memory */
  if (file->f_pos + count > PMAC_SIZE) {
    count = PMAC_SIZE - file->f_pos;
  }

  /* Create an intermediate kernel buffer */
  kbuf = (char *) kmalloc(count, GFP_KERNEL);
  if (!kbuf) {
    return -ENOMEM;
  }

  /* Copy the PCI memory into the kernel buffer */
  memcpy_fromio(kbuf, pmac->kbase + file->f_pos, count);

  /* Copy the kernel buffer back to the user application */
  if(copy_to_user(buf, kbuf, count)){
    kfree(kbuf);
    return -EFAULT;
  }

  /* Update the file position and cleanup */
  file->f_pos += count;

  kfree(kbuf);

  /* Return the number of bytes sucessfully read */
  return count;
}

static ssize_t pmac_write(
			  struct file *file,
			  const char *buf,
			  size_t count,
			  loff_t *offset)
{
  struct pmac_device* pmac;
  int slot;
  char *kbuf;

  /* Find the slot */
  slot  = pmac_minor_to_slot(MINOR(file->f_dentry->d_inode->i_rdev));

  MSG("<pmac_write> called with slot %d, count %d.\n",
      slot, count);

  /* Device specific info */
  pmac = pmac_device_by_slot(slot);

  /* Check we haven't reached the EOF */
  if (file->f_pos >= PMAC_SIZE) {
    return 0;
  }

  /* Reduce write counts larger than the register region */
  if (count > PMAC_SIZE) {
    count = PMAC_SIZE;
  }

  /* Check for writes past the end of memory */
  if (file->f_pos + count > PMAC_SIZE) {
    count = PMAC_SIZE - file->f_pos;
  }

  /* Create an intermediate kernel buffer */
  kbuf = (char *) kmalloc(count, GFP_KERNEL);
  if (!kbuf) {
    return -ENOMEM;
  }

  /* Copy data from the user application to the kernel buffer */
  if(copy_from_user(kbuf, buf, count)){
    kfree(kbuf);
    return -EFAULT;
  }

  /* Copy the kernel buffer to PCI memory */
  memcpy_toio(pmac->kbase + file->f_pos, kbuf, count);

  /* Update the file position and cleanup */
  file->f_pos += count;
  kfree(kbuf);

  /* Return the number of bytes successfully written */
  return count;
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
 *       1025th causes core dump. The PMAC registers were at dword offset
 *       200h, i.e., half-way through the page.
 *
 *       Hmmm, I really want mmap to return a pointer to the registers,
 *       not the start of the page ... how do I get that back to the
 *       user - modify vm_offset?
 *
 *     - the offset is because (pbase % PAGE_SIZE) is non-zero.
 */
static int pmac_mmap(
		     struct file *file,
		     struct vm_area_struct *vma)
{
  struct pmac_device* pmac;
  int slot;
  unsigned long vm_size,vm_offset;

  /* Find the slot */
  slot  = pmac_minor_to_slot(MINOR(file->f_dentry->d_inode->i_rdev));

  MSG("<pmac_mmap> called with slot %d\n",
      slot);

  /* Device specific info */
  pmac = pmac_device_by_slot(slot);

  /* VMA properties */
  vm_size   = vma->vm_end - vma->vm_start;
  vm_offset = vma->vm_pgoff << PAGE_SHIFT;

  MSG(" - requested map size is %ld, and offset is %ld.\n",
      vm_size,vm_offset);

  /* Increase vm_size to the nearest page */
  if (vm_size % PAGE_SIZE) {
    vm_size = (vm_size/PAGE_SIZE+1)*PAGE_SIZE;
    MSG(" - increasing map size to %ld.\n",vm_size);
  }

  /* Had to remove the following test, since the request is
   * for PAGE_SIZE (> PMAC_SIZE)!
   */

  /* Check that the request doesn't exceed the device size */
  /* the vm_size is now a nearest-page size ... */
  //	if ((vm_offset + vm_size) > PMAC_SIZE) {
  //		return -EINVAL;
  //	}

  /* Remap the page range */
  if(remap_page_range(
		      //#if REMAP_PAGE_RANGE_FIX
		      vma,
		      //#endif
		      vma->vm_start,            /* start of user memory           */
		      pmac->pbase + vm_offset,   /* start of PMAC regs physical mem */
		      vm_size,                  /* size                           */
		      vma->vm_page_prot)) {     /* permissions                    */
    MSG(" - remap_page_range failed!\n");
    return -EAGAIN;
  }

  MSG(" - region successfully mmapped.\n");
  return 0;
}

static int pmac_open(
		struct inode *inode,
		struct file *file)
{
  struct pmac_device* pmac;
  int slot;
  
  /* Increment the use count (obsolete in later kernels) */

#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,5,0)
  MOD_INC_USE_COUNT;
#endif
  
  /* Find the slot */
  slot  = pmac_minor_to_slot(MINOR(inode->i_rdev));
  
  MSG("<pmac_open> called with slot %d by %s (pid = %u).\n",
      slot, current->comm, current->pid);
  
  /* Device specific info */
  pmac = pmac_device_by_slot(slot);
  
  /* Check if the board exists */
  if(pmac == NULL) {
    MSG(" - Open failed. No device.\n");

    /* Decrement the use count (obsolete in later kernels) */
    
#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,5,0)
    MOD_DEC_USE_COUNT;
#endif
    
    return -ENXIO;
  }
  return 0;
}

static int pmac_release(
			struct inode *inode,
			struct file *file)
{
  MSG("<pmac_release> called.\n");

  /* Decrement the use count (obsolete in later kernels) */
  
#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,5,0)
  MOD_DEC_USE_COUNT;
#endif

  return 0;
}

/*---------------------------------------------------------------------------
 * Module load/unload declaration and definition
 *---------------------------------------------------------------------------
 *
 * Per device initialization routine. The pmac parameter must be passed
 * as a pointer-to-a-pointer-to-a-PMAC_DEVICE, since you want to adjust
 * the address of the pointer. (Otherwise the pointer is passed by
 * value and you can't return the address of the newly constructed
 * object) - yeah, I did it wrong first and got an oops :)
 *
 * During initialization if some devices are successfully initialized,
 * and a subsequent device fails, then the driver just handles the
 * ones that were successful.
 */
static int pmac_device_init(int index, struct pci_dev *pdv, struct pmac_device** ppmac);

/* resource deallocation for a single device */
static void pmac_release_resources(struct pmac_device* pmac);

/* The pmac_init() routine performs the following operations:
 *
 *  - finds all the PMAC-9054 devices on the PCI bus
 *  - calls pmac_device_init() to initialize each PMAC_DEVICE
 *    structure containing information for each device found.
 *
 * Optionally (#ifdef PMAC_COMPILE_AS_MODULE)
 *	- registers a character device driver to provide user-space
 * 	  access to the PMAC-9054 control registers.
 *
 * When pmac_driver.c is included as part of the COBRA device driver,
 * pmac_init() is used to find all the PMAC-9054 devices on the
 * bus, then pmac_pci_dev(slot) is used to access the PCI_DEV
 * structure to get further info. The PMAC_DEVICE does not
 * need to be visible outside of this module. When the
 * COBRA device is unloaded, it releases its resources,
 * then calls pmac_exit() to release the PMAC-9054 resources.
 *
 */
int pmac_init(void)
{
  struct pci_dev*     pdv  = NULL;  /* Needs to start out NULL */
  struct pmac_device* pmac = NULL;
  int i;
  int status;
  int index;
  int version;

  MSG("<pmac_init> called.\n");

  /* Try registering the character device first, as if this
   * fails, its not worth looking for devices!
   */
#ifdef PMAC_COMPILE_AS_MODULE
  /* Register the char device and use dynamic major allocation */
  MSG(" - register the device.\n");
  pmac_major = register_chrdev(0, pmac_name, &pmac_fops);
  if (pmac_major < 0) {
    MSG(" - major assignment failed with error %d\n", pmac_major);
    /* no resources to release */
    return pmac_major;
  }
  MSG(" - major = %d\n", pmac_major);
#endif /* PMAC_COMPILE_AS_MODULE */

#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,5,0)
  if(!pci_present()) {
    return -ENODEV;
  }
#endif

  /* Set all entries in the device array to NULL. Devices that
   * are not present on the bus will have this field left as NULL.
   * The device existence is tested during open.
   *
   * Memory for the device structure is only allocated if a device
   * exists.
   */
  for(i = 0; i < PMAC_MAX_DEVICES; i++) {
    pmac_devices[i] = NULL;
  }

  /* OLD 2.2 METHOD - CHANGE TO PROBE() / PCI_DRIVER
   *-------------------------------------------------
   * lots of this stuff goes into the probe function.
   * epca.c epca_init_one () probe function is pretty clean
   * and shows the use of the pci_device_id user data field.
   *
   */
  /* Search through all PCI devices for the PMAC-9054 */
  MSG(" - searching for PMAC devices...\n");

  /* Search through the PCI bus for PMAC-9054 devices */
  index = 0;

  if((pdv = pci_find_device(PMAC_VENDOR_ID, PMAC_DEVICE_ID, pdv)) != NULL) {

    /* Initialize the board */

    if((status = pmac_device_init(index, pdv, &pmac))) {

      /* Initialization of a device failed for some reason */

      printk("Initialization of a device %d failed "\
	     "with status %d.\n", index, status);
      
      return -ENODEV;

    } else {
      /* Add the board to the global array (devices are stored 
       * in the array based on the order they are found).
       */
      pmac_devices[index] = pmac;
      ++index;
    }
  }

  MSG(" - found %d board(s)\n", index);
  pmac_device_count = index;
	
  version = pmac_driver_version();

#ifdef PMAC_COMPILE_AS_MODULE
  printk("%s: module version %d.%d loaded (major = %d, %d devices).\n", 
	 pmac_name, 
	 (version >> 8) & 0xFF,
	 (version & 0xFF),
	 pmac_major, pmac_device_count);
  for (i = 0; i < pmac_device_count; i++) {
    printk(" - device %2d: bus %2d devsel %2d\n",
	   i, 
	   pmac_devices[i]->bus, 
	   pmac_devices[i]->devsel);
  }	
#endif
  return 0;
}

/* Create and initialize an instance of a pmac_device structure
 * based on the PCI_DEV info passed to us.
 *
 * Return an int so that error codes can propagate back.
 */
static int pmac_device_init(int index, struct pci_dev *pdv, struct pmac_device** ppmac)
{
  struct pmac_device* pmac; /* The new device */
  unsigned long pci_flags;

  MSG("<pmac_device_init> called.\n");

  MSG(" - allocate PMAC_DEVICE structure, ");
  /* Obtain memory and fill in the device info */
  if((pmac = (struct pmac_device*)
      kmalloc(sizeof(struct pmac_device), GFP_KERNEL)) == NULL) {
    MSG(" - kmalloc for pmac_device_t failed.\n");
    return -ENOMEM;
  }
  MSG("got %p\n",pmac);
  memset(pmac, 0, sizeof(struct pmac_device));

  /* Point to our PCI device info */
  pmac->pdv  = pdv;

  /* Initialize slot/index info */
  pmac->slot  = index;
  pmac->index = index;

  /* The purpose of this driver is for control of the PMAC-9054
   * memory mapped control registers. These registers are always
   * in BAR0, so only obtain those resources.
   *
   * It is then the responsibility of the user driver to checks 
   * for the application-specific BAR, and acquires other 
   * resources such as memory and IRQs.
   */
  MSG(" - obtaining BAR0 resources.\n");
  pmac->pbase = pci_resource_start(pdv,0);
  pmac->size  = pci_resource_len(pdv,0);
  pci_flags   = pci_resource_flags(pdv,0);

  MSG(" - BAR0 start 0x%X, size %d\n", (int)pmac->pbase, (int)pmac->size);

  /* Check that BAR0 is memory, and then acquire the memory resource */
  if ((pci_flags & IORESOURCE_MEM) && (pmac->size)) {
    if(!request_mem_region(pmac->pbase, pmac->size, pmac_name)) {
      MSG(" - PCI memory request for BAR0 failed.\n");
      /* Free resources */
      pmac->size = 0;
      pmac_release_resources(pmac);
      return -EIO;
    }
    /* Map physical address to a kernal virtual address
     * p833 Mat00
     */
    pmac->kbase  = ioremap(pmac->pbase, pmac->size);
    pmac->mapped = 1;
  } else {
    /* If its not memory, or the size is zero,
     * then something's wrong!
     */
    pmac->size = 0;
    pmac_release_resources(pmac);
    return -ENODEV;
  }

  /* Electrical location */
  pmac->bus    = pdv->bus->number;      /* Physical bus number */
  pmac->devsel = PCI_SLOT(pdv->devfn);  /* Devsel number       */

  /* Enable the device (new in 2.4)
   *
   * PCI devices should be 'enabled' before any registers
   * are accessed (this probably enables the I/O and mem
   * flags in the PCI control and status registers).
   */
  MSG(" - enabling the device.\n");
  if (pci_enable_device(pdv)) {
    MSG(" - PCI enable for device in slot %d failed.\n", pmac->slot);
    /* Free resources */
    pmac_release_resources(pmac);
    return -EIO;
  }

  MSG(" - found PMAC-9054 on bus %d, devsel %d (slot %d)\n",
      pmac->bus, pmac->devsel, pmac->slot);
  MSG(" - BAR0 physical/kernel = %lx/%p, size = %lx\n",
      pmac->pbase, pmac->kbase, pmac->size);

  /* Adjust the pointer passed in, to look at the new device */
  *ppmac = pmac;

  return 0;
}

/* Release the allocated resources for a device */
static void pmac_release_resources(struct pmac_device* pmac)
{
  if (pmac != NULL) {
    /* PCI device resources */
    if (pmac->size) {
      if (pmac->mapped) {
	MSG(" - unmapping I/O memory at kernel address %p\n",
	    pmac->kbase);
	iounmap(pmac->kbase);
      }
      MSG(" - releasing I/O memory at physical address %lX\n",
	  pmac->pbase);
      release_mem_region(pmac->pbase,pmac->size);
    }
    /* The device structure */
    MSG(" - releasing device memory at %p\n",pmac);
    kfree(pmac);
    pmac = NULL;
  }
}

void pmac_exit(void)
{
  int i;
  MSG("<pmac-exit> called.\n");

  /* Release the resources */
  MSG(" - release resources.\n");
  for (i = 0; i < pmac_device_count; i++) {
    pmac_release_resources(pmac_devices[i]);
  }

#ifdef PMAC_COMPILE_AS_MODULE
  /* Unregister the device */
  MSG(" - unregister the device.\n");
  unregister_chrdev(pmac_major, pmac_name);

  printk("%s: module unloaded\n", pmac_name);
#endif /* PMAC_COMPILE_AS_MODULE */
}

#ifdef PMAC_COMPILE_AS_MODULE
module_init(pmac_init);
module_exit(pmac_exit);
#endif /* PMAC_COMPILE_AS_MODULE */


