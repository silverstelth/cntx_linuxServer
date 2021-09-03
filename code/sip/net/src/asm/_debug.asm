.CODE

; º¯ÊıÔ­ĞÍ
; inline void __asm_breakpoint();
__asm_breakpoint proc
	int 3
	ret
__asm_breakpoint endp

END