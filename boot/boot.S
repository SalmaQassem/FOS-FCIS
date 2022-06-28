#include <inc/mmu.h>
	
.set PROT_MODE_CSEG,0x8		# code segment selector
.set PROT_MODE_DSEG,0x10        # data segment selector
.set CR0_PE_ON,0x1		# protected mode enable flag
	
###############################################################################
# ENTRY POINT	
#   This code should be stored in the first sector of the hard disk.
#   After the BIOS initializes the hardware on startup or system reset,
#   it loads this code at physical address 0x7c00 - 0x7d00 (512 bytes).
#   Then the BIOS jumps to the beginning of it, address 0x7c00,
#   while running in 16-bit real-mode (8086 compatibility mode).
#   The Code Segment register (CS) is initially zero on entry.
#	
# This code switches into 32-bit protected mode so that all of
# memory can accessed, then calls into C.
###############################################################################
	
.globl start					# Entry point	
start:		.code16				# This runs in real mode
		cli				# Disable interrupts
		cld				# String operations increment

		# Set up the important data segment registers (DS, ES, SS).
		xorw	%ax,%ax			# Segment number zero
		movw	%ax,%ds			# -> Data Segment
		movw	%ax,%es			# -> Extra Segment
		movw	%ax,%ss			# -> Stack Segment

		# Set up the stack pointer, growing downward from 0x7c00.
		movw	$start,%sp         	# Stack Pointer
	
# Enable A20:
#   For fascinating historical reasons (related to the fact that
#   the earliest 8086-based PCs could only address 1MB of physical memory
#   and subsequent 80286-based PCs wanted to retain maximum compatibility),
#   physical address line 20 is tied to low when the machine boots.
#   Obviously this a bit of a drag for us, especially when trying to
#   address memory above 1MB.  This code undoes this.
	
seta20.1:	inb	$0x64,%al		# Get status
		testb	$0x2,%al		# Busy?
		jnz	seta20.1		# Yes
		movb	$0xd1,%al		# Command: Write
		outb	%al,$0x64		#  output port
seta20.2:	inb	$0x64,%al		# Get status
		testb	$0x2,%al		# Busy?
		jnz	seta20.2		# Yes
		movb	$0xdf,%al		# Enable
		outb	%al,$0x60		#  A20

# Switch from real to protected mode:
#   Up until now, there's been no protection, so we've gotten along perfectly
#   well without explicitly telling the processor how to translate addresses.
#   When we switch to protected mode, this is no longer true!
#   We need at least to set up some "segments" that tell the processor it's
#   OK to run code at any address, or write to any address.
#   The 'gdt' and 'gdtdesc' tables below define these segments.
#   This code loads them into the processor.
#   We need this setup to ensure the transition to protected mode is smooth.

real_to_prot:	cli			# Don't allow interrupts: mandatory,
					# since we didn't set up an interrupt
					# descriptor table for handling them
		lgdt	gdtdesc		# load GDT: mandatory in protected mode
		movl	%cr0, %eax	# Turn on protected mode
		orl	$CR0_PE_ON, %eax
		movl	%eax, %cr0

	        # CPU magic: jump to relocation, flush prefetch queue, and
		# reload %cs.  Has the effect of just jmp to the next
		# instruction, but simultaneously loads CS with
		# $PROT_MODE_CSEG.
		ljmp	$PROT_MODE_CSEG, $protcseg
	
		# we've switched to 32-bit protected mode; tell the assembler
		# to generate code for that mode
protcseg:	.code32
		# Set up the protected-mode data segment registers
		movw	$PROT_MODE_DSEG, %ax	# Our data segment selector
		movw	%ax, %ds		# -> DS: Data Segment
		movw	%ax, %es		# -> ES: Extra Segment
		movw	%ax, %fs		# -> FS
		movw	%ax, %gs		# -> GS
		movw	%ax, %ss		# -> SS: Stack Segment
	
		call cmain			# finish the boot load from C.
						# cmain() should not return
spin:		jmp spin			# ..but in case it does, spin
	
		.p2align 2			# force 4 byte alignment
gdt:		SEG_NULL				# null seg
		SEG(STA_X|STA_R, 0x0, 0xffffffff)	# code seg
		SEG(STA_W, 0x0, 0xffffffff)	        # data seg
	
gdtdesc:	.word	0x17			# sizeof(gdt) - 1
		.long	gdt			# address gdt

#include <inc/x86.h>
#include <inc/elf.h>

/**********************************************************************
 * This a dirt simple boot loader, whose sole job is to boot
 * an elf kernel image from the first IDE hard disk.
 *
 * DISK LAYOUT
 *  * This program(boot.S and main.c) is the bootloader.  It should
 *    be stored in the first sector of the disk.
 * 
 *  * The 2nd sector onward holds the kernel image.
 *	
 *  * The kernel image must be in ELF format.
 *
 * BOOT UP STEPS	
 *  * when the CPU boots it loads the BIOS into memory and executes it
 *
 *  * the BIOS intializes devices, sets of the interrupt routines, and
 *    reads the first sector of the boot device(e.g., hard-drive) 
 *    into memory and jumps to it.
 *
 *  * Assuming this boot loader is stored in the first sector of the
 *    hard-drive, this code takes over...
 *
 *  * control starts in bootloader.S -- which sets up protected mode,
 *    and a stack so C code then run, then calls cmain()
 *
 *  * cmain() in this file takes over, reads in the kernel and jumps to it.
 **********************************************************************/

#define SECTSIZE	512
#define ELFHDR		((struct Elf *) 0x10000) // scratch space

void readsect(void*, uint32);
void readseg(uint32, uint32, uint32);

void
cmain(void)
{
	struct Proghdr *ph, *eph;

	// read 1st page off disk
	readseg((uint32) ELFHDR, SECTSIZE*8, 0);

	// is this a valid ELF?
	if (ELFHDR->e_magic != ELF_MAGIC)
		goto bad;

	// load each program segment (ignores ph flags)
	ph = (struct Proghdr *) ((uint8 *) ELFHDR + ELFHDR->e_phoff);
	eph = ph + ELFHDR->e_phnum;
	for (; ph < eph; ph++)
		readseg(ph->p_va, ph->p_memsz, ph->p_offset);

	// call the entry point from the ELF header
	// note: does not return!
	((void (*)(void)) (ELFHDR->e_entry & 0xFFFFFF))();

bad:
	outw(0x8A00, 0x8A00);
	outw(0x8A00, 0x8E00);
	while (1)
		/* do nothing */;
}

// Read 'count' bytes at 'offset' from kernel into virtual address 'va'.
// Might copy more than asked
void
readseg(uint32 va, uint32 count, uint32 offset)
{
	uint32 end_va;

	va &= 0xFFFFFF;
	end_va = va + count;
	
	// round down to sector boundary
	va &= ~(SECTSIZE - 1);

	// translate from bytes to sectors, and kernel starts at sector 1
	offset = (offset / SECTSIZE) + 1;

	// If this is too slow, we could read lots of sectors at a time.
	// We'd write more to memory than asked, but it doesn't matter --
	// we load in increasing order.
	while (va < end_va) {
		readsect((uint8*) va, offset);
		va += SECTSIZE;
		offset++;
	}
}

void
waitdisk(void)
{
	// wait for disk reaady
	while ((inb(0x1F7) & 0xC0) != 0x40)
		/* do nothing */;
}

void
readsect(void *dst, uint32 offset)
{
	// wait for disk to be ready
	waitdisk();

	outb(0x1F2, 1);		// count = 1
	outb(0x1F3, offset);
	outb(0x1F4, offset >> 8);
	outb(0x1F5, offset >> 16);
	outb(0x1F6, (offset >> 24) | 0xE0);
	outb(0x1F7, 0x20);	// cmd 0x20 - read sectors

	// wait for disk to be ready
	waitdisk();

	// read a sector
	insl(0x1F0, dst, SECTSIZE/4);
}


#
# Makefile fragment for the FOS kernel.
# This is NOT a complete makefile;
# you must run GNU make in the top-level directory
# where the GNUmakefile is located.
#

OBJDIRS += boot

BOOT_OBJS := $(OBJDIR)/boot/boot.o $(OBJDIR)/boot/main.o

$(OBJDIR)/boot/%.o: boot/%.c
	@echo + cc -Os $<
	@mkdir -p $(@D)
	$(V)$(CC) -nostdinc $(KERN_CFLAGS) -Os -c -o $@ $<

$(OBJDIR)/boot/%.o: boot/%.S
	@echo + as $<
	@mkdir -p $(@D)
	$(V)$(CC) -nostdinc $(KERN_CFLAGS) -c -o $@ $<

$(OBJDIR)/boot/main.o: boot/main.c
	@echo + cc -Os $<
	$(V)$(CC) -nostdinc $(KERN_CFLAGS) -Os -c -o $(OBJDIR)/boot/main.o boot/main.c

$(OBJDIR)/boot/boot: $(BOOT_OBJS)
	@echo + ld -m elf_i386 boot/boot
	$(V)$(LD) --oformat=elf32-i386 -melf_i386 -N -e start -Ttext 0x7C00 -o $@.out $^
	$(V)$(OBJDUMP) -S $@.out >$@.asm
	$(V)$(OBJCOPY) -S -R ".eh_frame" -O binary $@.out $@
	$(V)perl boot/sign.pl $(OBJDIR)/boot/boot


#!/bin/perl

open(SIG, $ARGV[0]) || die "open $ARGV[0]: $!";

$n = sysread(SIG, $buf, 1000);

if($n > 510){
	print STDERR "boot block too large: $n bytes (max 510)\n";
	exit 1;
}

print STDERR "boot block is $n bytes (max 510)\n";

$buf .= "\0" x (510-$n);
$buf .= "\x55\xAA";

open(SIG, ">$ARGV[0]") || die "open >$ARGV[0]: $!";
print SIG $buf;
close SIG;
