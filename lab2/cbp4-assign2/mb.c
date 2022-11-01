// easy micro benchmark
#define ITERS 10000
// generated assembly code
/*
	.file	"mb.c"
	.text
	.section	.text.startup,"ax",@progbits
	.p2align 4
	.globl	main
	.type	main, @function
main:
.LFB0:
	.cfi_startproc
	xorl	%edx, %edx
	xorl	%ecx, %ecx
	xorl	%eax, %eax
	movl	$2, %r8d
	movl	$1, %edi
	.p2align 4,,10
	.p2align 3
.L3:
	addl	$1, %eax
	imull	$-1227133513, %eax, %esi
	cmpl	$613566756, %esi
	cmovbe	%r8d, %edx
	cmovbe	%edi, %ecx
	cmpl	$10000, %eax
	jne	.L3
	leal	10000(%rcx,%rdx), %eax
	ret
	.cfi_endproc
.LFE0:
	.size	main, .-main
	.ident	"GCC: (Debian 10.2.1-6) 10.2.1 20210110"
	.section	.note.GNU-stack,"",@progbits

// Testcase 1:
// When there is no content in the loop, 1943 mispredicts.
*/
int main()
{
	unsigned int i = 0;
	unsigned int b = 0;

	//  it should mispredict 6 times for the while loop below as initially all conditions are set to be weakly not taken
	while (i < ITERS)
	{
		i += 1;
		// furthur we expect the bits to change one more time for a taken condition

		if (i % 7 == 0)
		{
			b = 1;
		}
	}

	return 0;
}
