GLOBAL weirdo_write:function,weirdo_write_pf:function,weirdo_read1:function,weirdo_read2:function

%define UNROLL_SHIFT  0
%define UNROLL (1 << UNROLL_SHIFT)

%macro unrollw 1
%assign off 0
%rep %1
    mov     DWORD [rdx + off + FIRSTO], eax
    mov     DWORD [rdx + off + SECONDO], eax
%assign off off + STRIDE
%endrep
%endmacro

%macro unrollr1 1
%assign off 0
%rep %1
    mov     eax, DWORD [rdx + off + FIRSTO]
%assign off off + STRIDE
%endrep
%endmacro

%macro unrollr2 1
%assign off 0
%rep %1
    mov     eax, DWORD [rdx + off + FIRSTO]
    mov     eax, DWORD [rdx + off + SECONDO]
%assign off off + STRIDE
%endrep
%endmacro

;; it's weird...
; rdi iteration count
; rsi output pointer
%macro write_func 2
%1:
    mov     rcx, rsi
    mov     rdx, rsi
    mov     eax, 1

    shr     rdi, UNROLL_SHIFT

align 16
.top:
    unrollw     UNROLL

    add     rdx, UNROLL * STRIDE

    %2

    sub    rdi,1
    jne    .top

    ret
%endmacro

%if FIRSTO < SECONDO
%define PF_DIST SECONDO
%else
%define PF_DIST FIRSTO
%endif

write_func weirdo_write, times 0 nop
write_func weirdo_write_pf, PREFETCHT0 [rdx + PF_DIST]

GLOBAL weirdo_write2:function
weirdo_write2:
    mov     rcx, rsi
    mov     rdx, rsi
    mov     eax, 1

    shr     rdi, UNROLL_SHIFT

align 16
.top:
    mov     DWORD [rdx], eax
    mov     DWORD [rdx + 15872], eax
    mov     DWORD [rdx + 31744], eax
    mov     DWORD [rdx + 47616], eax

    add     rdx, UNROLL * STRIDE

    sub    rdi,1
    jne    .top

    ret

GLOBAL weirdo_write3:function
weirdo_write3:
    mov     rcx, rsi
    mov     rdx, rsi
    mov     eax, 1

align 16
.top:
    mov     DWORD [rdx], eax
    mov     BYTE  [rcx],  al

    add     rdx, UNROLL * STRIDE
    add     rcx, 1
    sub    rdi,1
    jne    .top

    ret

weirdo_read1:
    mov     rdx, rsi
    mov     rax, 0

    shr     rdi, UNROLL_SHIFT

align 16
.top:
    unrollr1     UNROLL

    add     rdx, UNROLL * STRIDE

    sub    rdi,1
    jne    .top

    ret

weirdo_read2:
    mov     rdx, rsi
    mov     rax, 0

    shr     rdi, UNROLL_SHIFT

align 16
.top:
    unrollr2     UNROLL

    add     rdx, UNROLL * STRIDE

    sub    rdi,1
    jne    .top

    ret

