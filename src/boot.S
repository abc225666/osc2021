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
  ldr x1, =__bss_end
  sub x1, x1, x0

loopbsszero:
  cbz x1, 1f
  str xzr, [x0], #8
  sub x1, x1, #8
  cbnz x1, loopbsszero

1:
  ldr x1, =SCTLR_VALUE_MMU_DISABLED
  msr sctlr_el1, x1

  ldr x1, =CPACR_EL1_VALUE
  msr cpacr_el1, x1

  ldr x1, =HCR_EL2_VALUE
  msr hcr_el2, x1

  bl el1_start
  bl el0_start
  
init:
  ldr x0, =__relocate_address
  mov sp, x0
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
  mov sp, x1
  adr x1, exception_handler
  msr vbar_el1, x1


  eret

el0_start:
  ldr x1, =SPSR_EL1_VALUE
  msr spsr_el1, x1
  msr elr_el1, lr

  ldr x1, =0x40000
  mov sp, x1

  eret
