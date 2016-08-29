
#
# makefile for olios
#
#   2011-02-24 23:58:59 
#


# 内核加载基地址，必须与编译中定义的一样

IMAGEBASE   =   0x80100000
KERNEL_ENTRY=   0x801000a0

ASM     =   nasm
DASM    =   ndisasm
CC      =   gcc
LD      =   ld

ASM_FLAGS   =   -I inc/ -f elf
C_FLAGS     =   -I inc/ -c -fno-builtin -O1 -fno-stack-protector
LD_FLAGS    =   -s -Ttext-segment $(IMAGEBASE)
DASM_FLAGS  =   -u -o $(KERNEL_ENTRY) -e 0xa0


KERNEL_IMAGE    =   zhoskrnl
LOADER_IMAGE	=	zhldr
WIN_LDR_IMG     =   winldr
KRNL_DASM_OUT   =   zhoskrnl.asm

OBJS    =   ke/sysentry.o \
            ke/interrupt.o \
            ke/int.o \
            ke/clock.o \
            ke/ipc.o \
            ke/systask.o \
            ke/global.o \
            mm/mminit.o \
            mm/allocpage.o \
            mm/syspte.o \
            mm/pagefault.o \
            mm/pool.o \
            mm/mmsup.o \
            ps/psinit.o \
            ps/pscreate.o \
            ps/sched.o \
            ps/task.o \
            ps/pssup.o \
            ps/ctxswap.o \
            rtl/print.o \
            rtl/misc.o \
            rtl/olsupa.o \
            rtl/dispa.o \
            rtl/olsup.o \
            io/ata/atapi.o \
            io/pci/pci.o


.PHONY : everything kernel loader clean cleanall str

everything : kernel loader winloader image

kernel : $(KERNEL_IMAGE)

loader : $(LOADER_IMAGE)

winloader : $(WIN_LDR_IMG)

clean :
	rm -f $(OBJS)

cleanall :
	rm -f $(OBJS) $(KERNEL_IMAGE) $(LOADER_IMAGE) $(WIN_LDR_IMG) $(KRNL_DASM_OUT)

str :
	strings -a $(KERNEL_IMAGE)

image :
	sudo mount -o loop,offset=21037056 olios.vhd /mnt/olios
	sudo cp -fv $(LOADER_IMAGE) /mnt/olios
	sudo cp -fv $(KERNEL_IMAGE) /mnt/olios
	sudo umount /mnt/olios
	
disasm :
	$(DASM) $(DASM_FLAGS) $(KERNEL_IMAGE) > $(KRNL_DASM_OUT)


#--------------------------------------------

$(LOADER_IMAGE) : boot/zhldr.asm \
                  boot/loader.inc \
                  inc/common.inc
	$(ASM) -I inc/ -I boot/ $< -o $@



$(WIN_LDR_IMG) : boot/winldr.asm
	$(ASM) -I inc/ -I boot/ $< -o $@



$(KERNEL_IMAGE) : $(OBJS)
	$(LD) $(LD_FLAGS) $(OBJS) -o $@



ke/sysentry.o : ke/sysentry.asm \
                inc/ke.inc \
                inc/common.inc
	$(ASM) $(ASM_FLAGS) $< -o $@



ke/interrupt.o : ke/interrupt.asm \
                 inc/ke.inc \
                 inc/common.inc
	$(ASM) $(ASM_FLAGS) $< -o $@



ke/int.o : ke/int.c \
           inc/ke.h \
           inc/type.h \
           inc/proto.h
	$(CC) $(C_FLAGS) $< -o $@



ke/clock.o : ke/clock.c \
             inc/ke.h \
             inc/type.h \
             inc/proto.h
	$(CC) $(C_FLAGS) $< -o $@



ke/ipc.o : ke/ipc.c \
           inc/ke.h \
           inc/type.h \
           inc/proto.h
	$(CC) $(C_FLAGS) $< -o $@



ke/systask.o : ke/systask.c \
               inc/ke.h \
               inc/type.h \
               inc/proto.h
	$(CC) $(C_FLAGS) $< -o $@



ke/global.o : ke/global.c \
              inc/type.h
	$(CC) $(C_FLAGS) $< -o $@



mm/mminit.o : mm/mminit.c \
              inc/ke.h \
              inc/type.h \
              inc/proto.h
	$(CC) $(C_FLAGS) $< -o $@



mm/allocpage.o : mm/allocpage.c \
                 inc/ke.h \
                 inc/type.h \
                 inc/proto.h
	$(CC) $(C_FLAGS) $< -o $@



mm/syspte.o : mm/syspte.c \
              inc/ke.h \
              inc/type.h \
              inc/proto.h
	$(CC) $(C_FLAGS) $< -o $@



mm/pagefault.o : mm/pagefault.c \
                 inc/ke.h \
                 inc/type.h \
                 inc/proto.h
	$(CC) $(C_FLAGS) $< -o $@



mm/pool.o : mm/pool.c \
            inc/ke.h \
            inc/type.h \
            inc/proto.h
	$(CC) $(C_FLAGS) $< -o $@



mm/mmsup.o : mm/mmsup.c \
             inc/ke.h \
             inc/type.h \
             inc/proto.h
	$(CC) $(C_FLAGS) $< -o $@



ps/psinit.o : ps/psinit.c \
              inc/ke.h \
              inc/type.h \
              inc/proto.h
	$(CC) $(C_FLAGS) $< -o $@



ps/pscreate.o : ps/pscreate.c \
                inc/ke.h \
                inc/type.h \
                inc/proto.h
	$(CC) $(C_FLAGS) $< -o $@



ps/sched.o : ps/sched.c \
             inc/ke.h \
             inc/type.h \
             inc/proto.h
	$(CC) $(C_FLAGS) $< -o $@



ps/task.o : ps/task.c \
            inc/ke.h \
            inc/type.h \
            inc/proto.h
	$(CC) $(C_FLAGS) $< -o $@



ps/pssup.o : ps/pssup.c \
             inc/ke.h \
             inc/type.h \
             inc/proto.h
	$(CC) $(C_FLAGS) $< -o $@



ps/ctxswap.o : ps/ctxswap.asm \
               inc/ke.inc \
               inc/common.inc
	$(ASM) $(ASM_FLAGS) $< -o $@



rtl/print.o : rtl/print.c \
              inc/ke.h \
              inc/type.h \
              inc/proto.h
	$(CC) $(C_FLAGS) $< -o $@



rtl/misc.o : rtl/misc.c \
             inc/proto.h \
             inc/type.h \
             inc/ke.h
	$(CC) $(C_FLAGS) $< -o $@



rtl/olsupa.o : rtl/olsupa.asm \
               inc/ke.inc \
               inc/common.inc
	$(ASM) $(ASM_FLAGS) $< -o $@



rtl/dispa.o : rtl/dispa.asm \
              inc/ke.inc \
              inc/common.inc
	$(ASM) $(ASM_FLAGS) $< -o $@



rtl/olsup.o : rtl/olsup.c \
              inc/ke.h \
              inc/type.h \
              inc/proto.h
	$(CC) $(C_FLAGS) $< -o $@



io/ata/atapi.o : io/ata/atapi.c \
                 io/ata/atapi.h \
                 inc/ke.h \
                 inc/type.h \
                 inc/proto.h
	$(CC) $(C_FLAGS) $< -o $@



io/pci/pci.o : io/pci/pci.c \
               io/pci/pci.h \
               inc/ke.h \
               inc/type.h \
               inc/proto.h
	$(CC) $(C_FLAGS) $< -o $@


