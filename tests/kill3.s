	.file	"kill.c"
	.version	"01.01"
gcc2_compiled.:
.text
	.align 16
.globl main
	.type	 main,@function
main:
	xorl %eax,%eax
	movb $37,%al
	xorl %ebx,%ebx
	xorl %ecx,%ecx
	movb $19,%ecx
	int $0x80
	ret

	pushl %ebp
	movl %esp,%ebp
	pushl $19
	pushl $0
	call kill
	addl $8,%esp
.L4:
	movl %ebp,%esp
	popl %ebp
	ret
.Lfe1:
	.size	 main,.Lfe1-main
	.ident	"GCC: (GNU) 2.7.2.2"
