EXTERN table

GLOBAL weirdo_asm:function

;; it's weird...
; rdi iteration count
; rsi output pointer
weirdo_asm:

    mov     rdx, rsi
    xor     eax, eax

align 16
.top:
    mov    [rdx], eax
    mov    [rsi], eax

    add    rdx, 16

    dec    rdi
    jne    .top

    ret

GLOBAL add_calibration:function
ALIGN 32
add_calibration:
sub rdi, 1
jnz add_calibration
ret

GLOBAL store_calibration_asm:function
ALIGN 32
store_calibration_asm:
mov [sink2], rax
sub rdi, 1
jnz add_calibration
ret

GLOBAL store_calibration_asm2:function
ALIGN 32
store_calibration_asm2:
mov [sink2], rdi
sub rdi, 1
jnz add_calibration
ret

section .data
sink2:
dq 0

