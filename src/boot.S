#include "sysregs.h"

.section ".text.boot"
.global _start
_start:
// use one cpu to boot
  mrs x4, mpidr_el1
  and x4, x4, #3
  cbz x4, bsszero
  b loop

bsszero:
  ldr x0, =__bss_begin
  ldr x1, =__bss_size

loopbsszero:
  cbz x1, 1f
  str xzr, [x0], #8
  sub x1, x1, #1
  cbnz x1, loopbsszero

1:
  ldr x1, =SCTLR_VALUE_MMU_DISABLED
  msr sctlr_el1, x1

  ldr x1, =CPACR_EL1_VALUE
  msr cpacr_el1, x1

  ldr x1, =HCR_EL2_VALUE
  msr hcr_el2, x1

  bl el1_start

  mrs     x1, CPACR_EL1
  orr     x1, x1, (3 << 20)
  msr     CPACR_EL1, x1

  
init:
  ldr x0, =__relocate_address
  mov sp, x0
  bl el0_start_orig
  bl kernel_main
  b loop

loop:
  wfe
  b loop

el1_start:
  ldr x1, =SPSR_EL2_VALUE
  msr spsr_el2, x1
  msr elr_el2, lr

  ldr x1, =0x60000
  msr sp_el1, x1

  adr x1, exception_handler
  msr vbar_el1, x1
  eret

.global el0_start_orig
el0_start_orig:
  ldr x1, =SPSR_EL1_VALUE
  msr spsr_el1, x1
  msr elr_el1, lr

  ldr x0, =0x40000
  msr sp_el0, x0
  eret
