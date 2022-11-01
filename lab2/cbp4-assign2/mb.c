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

<<<<<<< HEAD
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
=======
#define CHOSEN_TESTCASE (2)

/*
Lab 2 Microbenchmark Suite
*/

/*
Testcase 0: Base case

When there is no content in the main function, we still see 1813 mispredictions.
This is due to the fixed overhead added by C.

We measure this as a basis to calibrate the other, more valuable, testcases.
*/

#if CHOSEN_TESTCASE == 0
    int main()
    {
        unsigned int i = 0;
        unsigned int b = 0;
        unsigned int j = 0;

        // This outer while branches (taken) to continue.
        // It "not takens" only at the end.
        // Therefore: it should contribute only one mispredict.

        while (i < ITERS) {
            i += 1;
            while (b < 8 * i) {
                b += 1;
                j ^= 1;
            }
        }
        
        return i + b + j;
    }
#endif



/**
 * Testcase 1: Successful Memorization
 * 
 * Our 2-level predictor uses a history of 6 bits.
 * That means it is theoretically capable of perfectly memorizing a branch with a period of <= 7.
 * 
 * The code below tests that, and based on the analysis, we expect somewhere on the order of 10 mispredicts.
*/

#if CHOSEN_TESTCASE == 1
int main()
{
    /* Overall pattern.
        X: N                       N                       N    ..............T
        Y:   N               N   T   N               N   T   N
        Z:     T T T T T T N   N       T T T T T T N   N
           |______________________|____|   |   - history = 01111: cycle is memorized!
                period                 |   - history = 11011
                                       - history = 111100
    */

    /*
    ================================================================
    Raw Assembly: This is the assembly exactly as it's generated. See below for something more readable.
    ================================================================
    main:
    .LFB0:
        .cfi_startproc
        movl	$7, %ecx
        movl	$0, %eax
        movl	$0, %edx
        jmp	.L4
    .L2:
        addl	$7, %ecx
        cmpl	$70007, %ecx
        je	.L7
    .L4:
        cmpl	%eax, %ecx
        jbe	.L2
    .L3:
        addl	$1, %eax
        xorl	$1, %edx
        cmpl	%eax, %ecx
        jne	.L3
        jmp	.L2
    .L7:
        leal	10000(%rax,%rdx), %eax
        ret
        .cfi_endproc

    */

   /*
   ================================================================
    Aliased Assembly: This maintains the same structure as the assembly, but made human readable.
    ================================================================
    main:
        C = 7
        A = 0
        D = 0
        goto .L4

    .L2:
        C += 7

        // This is not-taken every time but the final one.
        // Since we default to not-taken, it's right every other time.
        // Pattern: NT NT NT NT NT T NT...
        // Contributes 1 mispredict

        if C == 70007: 
            goto DONE
    .L4:
        // This is not-taken the first time, when passed from .L3.
        // Execution then flows down (NT) through .L3, and immediately back up to .L2.
        // This time, we take it (T).
        // Then, it flows down and is once again taken again (NT).
        // Pattern: NT, T, NT, NT, T, NT T ..
        // Contributes 3 mispredicts.
        
        if A > C:
            goto .L2
    .L3:
        A += 1
        D += 1

        // Each outer-loop, this is taken 6 times in a row,
        // and not taken the 7th time.
        // We learn this pattern after 1 time through (WN -> WT),
        // so this contributes 6 mispredicts.
        // Pattern: T T T T T T NT
        // Contributes 6 mispredicts

        if C != A:
            goto .L3
        goto .L4
    */

    unsigned int i = 0;
    unsigned int j = 0;
    unsigned int b = 0;

    while (i < ITERS) {
        i += 1;
        while (b < 7 * i) {
            // These operations are chosen strategically to avoid optimization.
            b += 1;
            j ^= b;
        }   
    }

    return i + j + b;
}
#endif

/**
 * Testcase 2: Unsuccessful Memorization
 * 
 * This code is very similar to testcase 2, so a full analysis will not be given.
 * The important thing to note here is that the inner loop uses a period of 8: which is too long for memorization.
 * So, we expect (ignoring interference) 1 extra mis-predict every cycle.
 * So, in total, close to 10000 mispredicts (above overhead) are expected.
*/

#if CHOSEN_TESTCASE == 2
int main()
{
    unsigned int i = 0;
    unsigned int j = 0;
    unsigned int b = 0;

    while (i < ITERS)
    {
        i += 1;
        while (b < 8 * i) {
            b += 1;
            j ^= b;
        }   
    }

    return i + j + b;
>>>>>>> ee8bb59f325e689cd0cb09b9ceb6c46a0a73b8fd
}
#endif