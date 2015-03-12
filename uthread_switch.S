	.text

/* Switch from current_thread to next_thread. Make next_thread
 * the current_thread, and set next_thread to 0.
 * Use eax as a temporary register, which should be caller saved.
 */
	.globl thread_switch
thread_switch:
	/* YOUR CODE HERE */
/*	push %ebp */
/*	movl %esp, %ebp */

	pushal

/* Save the state of current thread */
	movl current_thread, %eax
	movl %esp, (%eax)

/* Move %esp to next thread */
	movl next_thread, %eax
	movl (%eax), %esp	

/* Make current_thread point to next_thread */
	movl %eax, current_thread

/* Set next_thread to 0 */
	movl $0x0, next_thread

	popal

/*	pop %ebp */

	ret				/* pop return address from stack */
