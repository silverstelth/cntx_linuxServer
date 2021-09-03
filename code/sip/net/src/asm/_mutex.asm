.CODE

; º¯ÊýÔ­ÐÍ
; bool __asm_atomic_swap(uint32_t *lockPtr);
__asm_atomic_swap proc
	mov		rax, [rcx]
	mov		dword ptr [rcx], 1
	ret
__asm_atomic_swap endp

END
