.CODE

; º¯ÊýÔ­ÐÍ
; void * __asm_memcpy(void *dst, const void *src, size_t nbytes);
__asm_memcpy proc
	; save reg
	push rsi
	push rdi
	push rbx
	push rdx

	mov rdi, rcx
	mov rsi, rdx
	mov rbx, r8

	; edx takes number of bytes%256
	mov	rdx, rbx
	and rdx, 255

	; ebx takes number of bytes/256
	shr	rbx, 8
	jz byteCopy
	
	; flush 32k into temporary buffer
loop32k:
	push rsi
	mov rcx, rbx

	; copy per block of 256 bytes. Must not override 256*128 = 32768 bytes(32k).
	cmp rcx, 128
	jle	skipMiniMize
	mov	rcx, 128

skipMiniMize:
	; eax takes the number of 256 bytes packet for this block.
	mov rax, rcx

loopMemToL1:
	; Prefetch next loop, non-temporal
	prefetchnta [rsi+256]

	; Read in source data
	movdqu xmm0, [rsi]
	movdqu xmm1, [rsi+16]
	movdqu xmm2, [rsi+32]
	movdqu xmm3, [rsi+48]
	movdqu xmm4, [rsi+64]
	movdqu xmm5, [rsi+80]
	movdqu xmm6, [rsi+96]
	movdqu xmm7, [rsi+112]
	movdqu xmm8, [rsi+128]
	movdqu xmm9, [rsi+144]
	movdqu xmm10, [rsi+160]
	movdqu xmm11, [rsi+176]
	movdqu xmm12, [rsi+192]
	movdqu xmm13, [rsi+208]
	movdqu xmm14, [rsi+224]
	movdqu xmm15, [rsi+240]

	add rsi, 256
	dec rcx
	jnz loopMemToL1

	; Now copy from L1 to system memory
	pop rsi
	mov rcx, rax

loopL1ToMem:
	; Read in source data from L1
	movdqu xmm0, [rsi]
	movdqu xmm1, [rsi+16]
	movdqu xmm2, [rsi+32]
	movdqu xmm3, [rsi+48]
	movdqu xmm4, [rsi+64]
	movdqu xmm5, [rsi+80]
	movdqu xmm6, [rsi+96]
	movdqu xmm7, [rsi+112]
	movdqu xmm8, [rsi+128]
	movdqu xmm9, [rsi+144]
	movdqu xmm10, [rsi+160]
	movdqu xmm11, [rsi+176]
	movdqu xmm12, [rsi+192]
	movdqu xmm13, [rsi+208]
	movdqu xmm14, [rsi+224]
	movdqu xmm15, [rsi+240]

	; Non-temporal stores 
	movntdq [rdi], xmm0
	movntdq [rdi+16], xmm1
	movntdq [rdi+32], xmm2
	movntdq [rdi+48], xmm3
	movntdq [rdi+64], xmm4
	movntdq [rdi+80], xmm5
	movntdq [rdi+96], xmm6
	movntdq [rdi+112], xmm7
	movntdq [rdi+128], xmm8
	movntdq [rdi+144], xmm9
	movntdq [rdi+160], xmm10
	movntdq [rdi+176], xmm11
	movntdq [rdi+192], xmm12
	movntdq [rdi+208], xmm13
	movntdq [rdi+224], xmm14
	movntdq [rdi+240], xmm15

	add rsi, 256
	add rdi, 256
	dec rcx
	jnz loopL1ToMem

	; Do next 32k block
	sub rbx, rax
	jnz loop32k
byteCopy:
	; Do last bytes with std cpy
	mov	rcx, rdx
	rep movsb

	sub rdi, r8
	mov rax, rdi

	; recovery
	pop rdx
	pop rbx
	pop rdi
	pop rsi
	ret
__asm_memcpy endp

END