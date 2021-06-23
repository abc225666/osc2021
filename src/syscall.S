.global getpid
getpid:
  mov x8, #0
  svc #0
  ret

.global kuart_read
kuart_read:
  mov x8, #1
  svc #0
  ret

.global kuart_write
kuart_write:
  mov x8, #2
  svc #0
  ret

.global exec
exec:
  mov x8, #3
  svc #0
  ret

.global exit
exit:
  mov x8, #4
  svc #0
  ret

.global schedule
schedule:
  mov x8, #5
  svc #0
  ret

.global fork
fork:
  mov x8, #6
  svc #0
  ret

.global open
open:
  mov x8, #7
  svc #0
  ret

.global close
close:
  mov x8, #8
  svc #0
  ret

.global read
read:
  mov x8, #9
  svc #0
  ret

.global write
write:
  mov x8, #10
  svc #0
  ret
