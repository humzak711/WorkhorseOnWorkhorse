#define ASM_FILE 1

#include <generated/autoconf.h>
#include <lib/multiboot2.h>
#include <ia32eAsm.h>
#include <ia32eCpu.h>

#define VBIOS_RESET_ADDR 0x1000
#define VBIOS_KERNEL_ADDR_OFFSET 0

.globl ia32eBiosWakeupBlobStart
.type ia32eBiosWakeupBlobStart, @object 

.globl ia32eBiosWakeupBlobEnd
.type ia32eBiosWakeupBlobEnd, @object

.section .data

.code16

.balign 64
ia32eBiosWakeupBlobStart:
    cli
    cld

    lgdt VBIOS_RESET_ADDR + (ia32eGdtr32 - ia32eBiosWakeupBlobStart)

    movl %cr0, %eax
    orl $(IA32E_CR0_PE_MASK), %eax
    movl %eax, %cr0

    xorl %edi, %edi
    ljmpl $IA32E_KCS_SELECTOR, $(VBIOS_RESET_ADDR + (ia32eWakeup32 - ia32eBiosWakeupBlobStart))

    hlt

.code32

.balign 64
ia32eWakeup32:

    movl $IA32E_KDS_SELECTOR, %eax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %ss

    movl $0x100080, %ecx
    jmpl *%ecx

    hlt

.balign 64
ia32eGdt32Base:
    .quad 0
    .quad 0x00CF9A000000FFFF
    .quad 0x00CF92000000FFFF
ia32eGdt32End:

ia32eGdtr32:
    .word ia32eGdt32End - ia32eGdt32Base - 1
    .long VBIOS_RESET_ADDR + (ia32eGdt32Base - ia32eBiosWakeupBlobStart)

ia32eBiosWakeupBlobEnd: