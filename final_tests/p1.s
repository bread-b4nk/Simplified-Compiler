	.file	"p1.c"
	.text
	.globl	func
	.type	func, @function
func:
.LFB0:
	.cfi_startproc
	pushl	%ebp
	.cfi_def_cfa_offset 8
	.cfi_offset 5, -8
	movl	%esp, %ebp
	.cfi_def_cfa_register 5
	pushl	%ebx
	subl	$20, %esp
	.cfi_offset 3, -12
	call	__x86.get_pc_thunk.bx
	addl	$_GLOBAL_OFFSET_TABLE_, %ebx
	movl	$1, -24(%ebp)
	movl	$1, -20(%ebp)
	movl	$0, -16(%ebp)
	jmp	.L2
.L3:
	subl	$12, %esp
	pushl	-24(%ebp)
	call	print@PLT
	addl	$16, %esp
	addl	$1, -16(%ebp)
	movl	-20(%ebp), %eax
	movl	%eax, -12(%ebp)
	movl	-24(%ebp), %eax
	addl	%eax, -20(%ebp)
	movl	-12(%ebp), %eax
	movl	%eax, -24(%ebp)
.L2:
	movl	-16(%ebp), %eax
	cmpl	8(%ebp), %eax
	jl	.L3
	movl	$1, %eax
	movl	-4(%ebp), %ebx
	leave
	.cfi_restore 5
	.cfi_restore 3
	.cfi_def_cfa 4, 4
	ret
	.cfi_endproc
.LFE0:
	.size	func, .-func
	.section	.text.__x86.get_pc_thunk.bx,"axG",@progbits,__x86.get_pc_thunk.bx,comdat
	.globl	__x86.get_pc_thunk.bx
	.hidden	__x86.get_pc_thunk.bx
	.type	__x86.get_pc_thunk.bx, @function
__x86.get_pc_thunk.bx:
.LFB1:
	.cfi_startproc
	movl	(%esp), %ebx
	ret
	.cfi_endproc
.LFE1:
	.ident	"GCC: (Ubuntu 11.3.0-1ubuntu1~22.04) 11.3.0"
	.section	.note.GNU-stack,"",@progbits
