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
