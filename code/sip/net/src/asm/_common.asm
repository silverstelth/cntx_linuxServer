.CODE

; º¯ÊýÔ­ÐÍ
; inline uint64 __asm_rdtsc();
__asm_rdtsc proc
	push	rdx
	rdtsc
	shl		rdx, 32
	add		rax, rdx
	pop		rdx
	ret
__asm_rdtsc endp

END