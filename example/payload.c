// payload.c

void _start()
{
    __asm__ volatile (
        "jmp 2f\n"
        "1:\n"
        "pop %rsi\n"        // buffer
        "mov $1, %rax\n"    // write
        "mov $1, %rdi\n"    // stdout
        "mov $6, %rdx\n"    // len
        "syscall\n"
        
        "mov $60, %rax\n"   // exit
        "xor %rdi, %rdi\n"  // status 0
        "syscall\n"
        
        "2:\n"
        "call 1b\n"
        ".ascii \"PWNED\\n\"\n"
    );
}

