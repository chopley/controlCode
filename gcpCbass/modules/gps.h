/*---------------------------------------------------------------------------
 * Filename: gps_driver.h
 * Author:   Erik Leitch
 * Date:     July-2002
 * Platform: Linux
 * Purpose:  Test device driver
 *---------------------------------------------------------------------------
 * References:
 *
 * [Pom99] "The Linux Kernel Module Programming Guide", O. Pomerantz, 1999.
 * [Rub98] "Linux Device Drivers", A. Rubini, 1998.
 * [Mat99] "Beginning Linux Programming", N. Matthew and R. Stones, 1999
 * 
 *---------------------------------------------------------------------------
 */

/*---------------------------------------------------------------------------
 * IOCTL code definitions 
 *---------------------------------------------------------------------------
 *
 * DEBUG    - if the code is compiled with the flag DEBUG defined, then
 *            this IOCTL call can be used to turn debugging messages
 *            on (1 = default) or off (write 0).
 *
 */

#define ACU_IOCTL_BASE       0xbb
#define ACU_IOCTL_DEBUG_GET  _IOR(ACU_IOCTL_BASE, 0, char)
#define ACU_IOCTL_DEBUG_SET  _IOW(ACU_IOCTL_BASE, 1, char)
#define ACU_IOCTL_SIGIO      _IOW(ACU_IOCTL_BASE, 2, char)
#define ACU_IOCTL_CLEAR      _IOW(ACU_IOCTL_BASE, 3, char)


