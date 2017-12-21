GLOBAL weirdo_asm:function,weirdo_read:function


%macro unrollr 1
%assign off 0
%rep %1
    ;mov     rax, [rdx + off]
    mov      rax, [rdx + off]
    mov      rax, [rdx + 65536/2 + 256 + off]
%assign off off + STRIDE
%endrep
%endmacro

%macro unrollrA 1
%assign off 0
%rep %1
    ;mov     rax, [rdx + off]
    ;mov     rax, [rdx + off + 8]
    vmovups   ymm0, [rdx + off]
    vmovups   ymm1, [rdx + off + 64]
    vmovaps   ymm2, [rdx + off + 32]
    vmovaps   ymm3, [rdx + off + 96]
    ;vpaddb   ymm1, ymm1, [rdx + off + 32]
%assign off off + STRIDE
%endrep
%endmacro

%macro unrollrB 1
%assign off 0
%rep %1
    ;mov     rax, [rdx + off]
    add      rax, [rdx + 65536/2 + 256 + off]
%assign off off + STRIDE
%endrep
%endmacro

%macro unroll 1
%assign off 0
%rep %1
    mov     [rdx + off], eax
    mov     [rsi], eax

    ;add     rdx, 8
%assign off off + STRIDE
%endrep
%endmacro

%macro unrollA 1
%assign off 0
%rep %1
    mov     DWORD [rdx + off], eax
    mov     DWORD [rsp - 8], eax
    ; mov     DWORD [rdx + off + SECONDO + 48], eax
    ;mov     QWORD [rdx + off + 2], rax
    ;vmovups  [rdx + off], ymm0
    ;vmovups  [rdx + off + 32], ymm0
    ;mov     BYTE [rdx + off + 0],    al
    ;mov     BYTE [rdx + off + 0],    al
    ;mov     BYTE [rdx + off + 0],    al
    ;mov     BYTE [rdx + off + 0],    al
%assign off off + STRIDE
%endrep
%endmacro

%macro unrollB 1
%assign off 0
%rep %1
    mov     [rsp - 8], rax
%assign off off + STRIDE
%endrep
%endmacro


;; it's weird...
; rdi iteration count
; rsi output pointer
weirdo_asm:

%define UNROLL_SHIFT  0
%define UNROLL (1 << UNROLL_SHIFT)

    mov     rcx, rsi
    mov     rdx, rsi
    mov     eax, 1

    shr     rdi, UNROLL_SHIFT

align 16
.top:
    ;unroll      UNROLL
    unrollA     UNROLL
    ;unrollB     UNROLL

;    add     rcx, UNROLL * STRIDE
    add     rdx, UNROLL * STRIDE
    ;mov     r8, [rdx + FIRSTO + SECONDO]
    PREFETCHT0 [rdx + 128]

    sub    rdi,1
    jne    .top

    ret

weirdo_read:
    mov     rdx, rsi
    mov     rax, 0

    shr     rdi, UNROLL_SHIFT

align 16
.top:
    ;unrollr      UNROLL
    unrollrA     UNROLL
    ;  unrollrB     UNROLL

    add     rdx, UNROLL * STRIDE

    sub    rdi,1
    jne    .top

    ret

