; 64-bit NASM macro for stack tracing
;  
; Define a macro named "s", taking one argument (+ means "greedy", including commas)
; This macro takes one instruction, and prints the state of the stack after
; running that instruction.  
%macro s 1+
	%1 ; run user's code

; Fancy footwork to save the flags & all registers
	mov QWORD[rsp-1000],rax ; save user's rax
	lahf ; copy flags into rax (FIXME: this doesn't copy overflow flag...)
	sub rsp,1000 ; move far from user's area of the stack
	push rax ; save even the scratch registers (be nice to user code)
	push rcx
	push rdx
	push rdi
	push rsi
	push r8
	push r9
	push r10
	push r11
; Print state of stack
	; Print user's current instruction
	extern printf
	mov rax,0 ; number of "vector registers" used (none)
	mov rdi,formatInstr
	mov rsi,%%codeString ; string version of user's code
	call printf

	; Figure out where the user's stack is
	mov r8,rsp
	add r8,1000+8*9 ; rcx==user's original stack pointer
	cmp QWORD[firstRSP],0 ; figure out where to start printing
	jne %%notFirst
		mov QWORD[firstRSP],r8 ; copy rsp, the first time around
		add QWORD[firstRSP],8 ; print one extra value
	%%notFirst:
	; Loop over stack, and print each (long) value
	; for (long *cur=first_rsp;cur>=user_rsp; cur--)
	mov r9,QWORD[firstRSP] ; cur: start printing where stack began
	jmp %%loopTest
	%%keepPrinting:
		mov rax,0 ; <- again, must be zero for printf to work.
		mov rdi,formatStack
		mov rsi,QWORD[r9] ; read value to print from the stack
		push r9 ; save loop variables
		push r8
		call printf
		pop r8
		pop r9
		sub r9,8 ; cur--
		%%loopTest:
		cmp r9,r8
		jae %%keepPrinting
	
	; End with a newline
	mov rax,0
	mov rdi,formatNewline
	call printf

; Restore user state
	pop r11
	pop r10
	pop r9
	pop r8
	pop rsi
	pop rdi
	pop rdx
	pop rcx
	pop rax
	add rsp,1000
	sahf; restore flags
	mov rax,QWORD[rsp-1000]; restore user's rax
	jmp %%afterStrings
	
section .data ; Need strings and such here.
%%codeString:
%defstr codestr %1
	db codestr,0

%ifndef have_RSPstore
firstRSP: ; stores first copy of RSP
	dq 0

formatInstr: db "%-15s Stack: ",0
formatStack: db "%6lx ",0
formatNewline: db 0xA,0
%define have_RSPstore
%endif

section .text ; Back to user code.
	%%afterStrings:
%endmacro

