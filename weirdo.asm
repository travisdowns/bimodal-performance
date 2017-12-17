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


