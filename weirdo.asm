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

    add     rdx, UNROLL * STRIDE
    add     rcx, 1
    sub    rdi,1
    jne    .top

    ret

; this guy writes to alternately to L1 and L2, with the L1 location fixed and
; unmoving
GLOBAL weirdo_write4:function
weirdo_write4:
    mov     rcx, rsi
    mov     rdx, rsi
    mov     eax, 1

align 16
.top:
    mov     DWORD [rdx], eax
    mov     DWORD [rcx], eax

    add     rdx, STRIDE
    sub     rdi,1
    jne     .top

    ret

; no interleaving at all, only a single STRIDEd stream of 32-bit writes
GLOBAL linear:function
linear:
    mov     rcx, rsi
    mov     rdx, rsi
    mov     eax, 1

align 16
.top:
    mov     DWORD [rdx], eax

    add     rdx, STRIDE
    sub     rdi,1
    jne     .top

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

GLOBAL rand_asm:FUNCTION
rand_asm:
    mov     rax, rdi                            
    shl     rax, 6                              
    blsr    rdx, rax                            
    test    rdx, rdx                            
    jnz     bad_size                           
    mov     r8d, 0               
    shr     rax, 2                              
    mov     ecx, 1235                           
    mov     r9, qword 5851F42D4C957F2DH          
    lea     r10d, [rax-1H]                      
                                             
.top:
    mov     rdx, rcx                        
    imul    rcx, r9                             
    and     rdx, r10                            
    mov     dword [rsi + rdx*4],    r8d              
    mov     dword [rsi + rdx*4-64], r8d          
    sub     rdi, 1                              
    jnz     .top                               
    ret                                         


%ifndef UNROLL2
%define UNROLL2 1
%endif

%macro asm2_body 0
    and     rax, r10
    mov     dword [rsi + rax], 0
    mov     dword [rcx + r8],  1
    add     rax, 64
%endmacro

GLOBAL rand_asm2:FUNCTION
rand_asm2:
    mov     rcx, rsi
    add     rsi, 4096
    mov     rax, rdi                            
    shl     rax, 6                              
    blsr    rdx, rax                            
    test    rdx, rdx                            
    jnz     bad_size                           
    lea     r10d, [rax - 1]
    mov     eax, 0
    mov     edx, 0

.top:
    %rep UNROLL2
    asm2_body
    %endrep
    ;sfence
    sub     rdi, UNROLL2                              
    jns     .top                               
    ret       


bad_size: ; size not a power of two
ud2
