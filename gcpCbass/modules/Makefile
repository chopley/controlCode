# Needed for 2.4 compilation

CC=gcc
LD=ld

KERNELDIR = /usr/include/linux

MODCFLAGS = -D__KERNEL__ -DMODULE -O2 -Wall

INCLUDES=-I/usr/src/linux/include

%.o: %.c
	$(CC) $(MODCFLAGS) -c $(INCLUDES) $<

# Needed for 2.6 compilation

obj-m += gps.o tfp.o inter.o

# If the kernel version starts with 2.6, compile modules the 2.6 way,
# else do the old-fashioned thing

ifneq (,$(findstring 2.6,$(shell uname -r)))
all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
else
all: gps.o tfp.o inter.o

clean:
	-rm -f *~ *.o *~
endif

