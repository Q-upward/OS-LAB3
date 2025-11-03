#include <assert.h>
#include <clock.h>
#include <console.h>
#include <defs.h>
#include <kdebug.h>
#include <memlayout.h>
#include <mmu.h>
#include <riscv.h>
#include <stdio.h>
#include <trap.h>
#include <sbi.h>
#include <clock.h>

// 【时钟计数常量定义】
#define TICK_NUM 100

// 【定时器中断打印功能】
static void print_ticks() {
    cprintf("%d ticks\n", TICK_NUM);
#ifdef DEBUG_GRADE
    cprintf("End of Test.\n");
    panic("EOT: kernel seems ok.");
#endif
}

// 【中断向量表初始化（IDT/stvec设置）】
/* idt_init - initialize IDT to each of the entry points in kern/trap/vectors.S
 */
void idt_init(void) {
    /* LAB3 2311383 : STEP 2 */
    /* (1) Where are the entry addrs of each Interrupt Service Routine (ISR)?
     * All ISR's entry addrs are stored in __vectors. where is uintptr_t
     * __vectors[] ?
     * __vectors[] is in kern/trap/vector.S which is produced by
     * tools/vector.c
     * (try "make" command in lab3, then you will find vector.S in kern/trap
     * DIR)
     * You can use  "extern uintptr_t __vectors[];" to define this extern
     * variable which will be used later.
     * (2) Now you should setup the entries of ISR in Interrupt Description
     * Table (IDT).
     * Can you see idt[256] in this file? Yes, it's IDT! you can use SETGATE
     * macro to setup each item of IDT
     * (3) After setup the contents of IDT, you will let CPU know where is the
     * IDT by using 'lidt' instruction.
     * You don't know the meaning of this instruction? just google it! and
     * check the libs/x86.h to know more.
     * Notice: the argument of lidt is idt_pd. try to find it!
     */

    extern void __alltraps(void);
    /* Set sup0 scratch register to 0, indicating to exception vector
       that we are presently executing in the kernel */
    write_csr(sscratch, 0);
    /* Set the exception vector address */
    write_csr(stvec, &__alltraps);
}

// 【判断陷阱是否发生在内核态】
/* trap_in_kernel - test if trap happened in kernel */
bool trap_in_kernel(struct trapframe *tf) {
    return (tf->status & SSTATUS_SPP) != 0;
}

// 【打印陷阱帧（Trapframe）信息】
void print_trapframe(struct trapframe *tf) {
    cprintf("trapframe at %p\n", tf);
    print_regs(&tf->gpr);
    cprintf("   status   0x%08x\n", tf->status);
    cprintf("   epc      0x%08x\n", tf->epc);
    cprintf("   badvaddr 0x%08x\n", tf->badvaddr);
    cprintf("   cause    0x%08x\n", tf->cause);
}

// 【打印通用寄存器（General Purpose Registers）信息】
void print_regs(struct pushregs *gpr) {
    cprintf("   zero     0x%08x\n", gpr->zero);
    cprintf("   ra       0x%08x\n", gpr->ra);
    cprintf("   sp       0x%08x\n", gpr->sp);
    cprintf("   gp       0x%08x\n", gpr->gp);
    cprintf("   tp       0x%08x\n", gpr->tp);
    cprintf("   t0       0x%08x\n", gpr->t0);
    cprintf("   t1       0x%08x\n", gpr->t1);
    cprintf("   t2       0x%08x\n", gpr->t2);
    cprintf("   s0       0x%08x\n", gpr->s0);
    cprintf("   s1       0x%08x\n", gpr->s1);
    cprintf("   a0       0x%08x\n", gpr->a0);
    cprintf("   a1       0x%08x\n", gpr->a1);
    cprintf("   a2       0x%08x\n", gpr->a2);
    cprintf("   a3       0x%08x\n", gpr->a3);
    cprintf("   a4       0x%08x\n", gpr->a4);
    cprintf("   a5       0x%08x\n", gpr->a5);
    cprintf("   a6       0x08x\n", gpr->a6);
    cprintf("   a7       0x%08x\n", gpr->a7);
    cprintf("   s2       0x%08x\n", gpr->s2);
    cprintf("   s3       0x%08x\n", gpr->s3);
    cprintf("   s4       0x%08x\n", gpr->s4);
    cprintf("   s5       0x%08x\n", gpr->s5);
    cprintf("   s6       0x%08x\n", gpr->s6);
    cprintf("   s7       0x%08x\n", gpr->s7);
    cprintf("   s8       0x%08x\n", gpr->s8);
    cprintf("   s9       0x%08x\n", gpr->s9);
    cprintf("   s10      0x%08x\n", gpr->s10);
    cprintf("   s11      0x%08x\n", gpr->s11);
    cprintf("   t3       0x%08x\n", gpr->t3);
    cprintf("   t4       0x%08x\n", gpr->t4);
    cprintf("   t5       0x%08x\n", gpr->t5);
    cprintf("   t6       0x%08x\n", gpr->t6);
}

// 【中断处理函数】
void interrupt_handler(struct trapframe *tf) {
    intptr_t cause = (tf->cause << 1) >> 1;
    switch (cause) {
        // 这些值都定义在riscv.c文件下
        case IRQ_U_SOFT:
            cprintf("User software interrupt\n");
            break;
        case IRQ_S_SOFT:
            cprintf("Supervisor software interrupt\n");
            break;
        case IRQ_H_SOFT:
            cprintf("Hypervisor software interrupt\n");
            break;
        case IRQ_M_SOFT:
            cprintf("Machine software interrupt\n");
            break;
        case IRQ_U_TIMER:
            cprintf("User Timer interrupt\n");
            break;
        case IRQ_S_TIMER:
            // "All bits besides SSIP and USIP in the sip register are
            // read-only." -- privileged spec1.9.1, 4.1.4, p59
            // In fact, Call sbi_set_timer will clear STIP, or you can clear it
            // directly.
            // cprintf("Supervisor timer interrupt\n");
             /* LAB3 EXERCISE1   2311383 :  */
             /*(1)设置下次时钟中断- clock_set_next_event()
              *(2)计数器（ticks）加一
              *(3)当计数器加到100的时候，我们会输出一个`100ticks`表示我们触发了100次时钟中断，同时打印次数（num）加一
              * (4)判断打印次数，当打印次数为10时，调用<sbi.h>中的关机函数关机
              */
             /* LAB3 EXERCISE1 implementation */
             /* Set next timer event */
             clock_set_next_event();

             /* ticks is declared volatile in clock.c */
             extern volatile size_t ticks;

             static int num_prints = 0;

             /* increase tick counter */
             ticks++;

             /* every TICK_NUM ticks, print and count prints */
             if (ticks % TICK_NUM == 0) {
                 print_ticks();
                 num_prints++;
                 if (num_prints >= 10) {
                     /* after printing 10 times, shut down via SBI */
                     sbi_shutdown();
                 }
             }

             break;
        case IRQ_H_TIMER:
            cprintf("Hypervisor software interrupt\n");
            break;
        case IRQ_M_TIMER:
            cprintf("Machine software interrupt\n");
            break;
        case IRQ_U_EXT:
            cprintf("User software interrupt\n");
            break;
        case IRQ_S_EXT:
            cprintf("Supervisor external interrupt\n");
            break;
        case IRQ_H_EXT:
            cprintf("Hypervisor software interrupt\n");
            break;
        case IRQ_M_EXT:
            cprintf("Machine software interrupt\n");
            break;
        default:
            print_trapframe(tf);
            break;
    }
}

// 【异常处理函数】
void exception_handler(struct trapframe *tf) {
    switch (tf->cause) {
        case CAUSE_MISALIGNED_FETCH:
            break;
        case CAUSE_FAULT_FETCH:
            break;
        case CAUSE_ILLEGAL_INSTRUCTION:
             // 非法指令异常处理
             /* LAB3 CHALLENGE3   YOUR CODE :2310421  */
             /*(1)输出指令异常类型（ Illegal instruction）
              *(2)输出异常指令地址
              *(3)更新 tf->epc寄存器
              */
            cprintf("Exception type: Illegal instruction\n");
            cprintf("Illegal instruction caught at 0x%lx\n", tf->epc);
            tf->epc += 4;
            break;

        case CAUSE_BREAKPOINT:
             //断点异常处理
             /* LAB3 CHALLLENGE3   YOUR CODE : 2312103 */
             /*(1)输出指令异常类型（ breakpoint）
              *(2)输出异常指令地址
              *(3)更新 tf->epc寄存器
              */
            cprintf("Exception type: breakpoint\n");
            cprintf("ebreak caught at 0x%lx\n", tf->epc);
            tf->epc += 4;
            break;
        case CAUSE_MISALIGNED_LOAD:
            break;
        case CAUSE_FAULT_LOAD:
            break;
        case CAUSE_MISALIGNED_STORE:
            break;
        case CAUSE_FAULT_STORE:
            break;
        case CAUSE_USER_ECALL:
            break;
        case CAUSE_SUPERVISOR_ECALL:
            break;
        case CAUSE_HYPERVISOR_ECALL:
            break;
        case CAUSE_MACHINE_ECALL:
            break;
        default:
            print_trapframe(tf);
            break;
    }
}

// 【陷阱分发函数】
static inline void trap_dispatch(struct trapframe *tf) {
    if ((intptr_t)tf->cause < 0) {
        // interrupts
        interrupt_handler(tf);
    } else {
        // exceptions
        exception_handler(tf);
    }
}

// 【SBI 调用通用接口】
uint64_t sbi_call(uint64_t sbi_type, uint64_t arg0, uint64_t arg1, uint64_t arg2) {
    uint64_t ret_val;
    __asm__ volatile (
        "mv x17, %[sbi_type]\n"  // 1. 将服务ID (a7) 设置为 SBI_SET_TIMER (0)
        "mv x10, %[arg0]\n"      // 2. 将 stime_value (arg0) 放入 a0 (x10) 寄存器
        "mv x11, %[arg1]\n"
        "mv x12, %[arg2]\n"
        "ecall\n"                // 3. 执行环境调用，切换到 M 模式
        "mv %[ret_val], x10"
        : [ret_val] "=r" (ret_val)
        : [sbi_type] "r" (sbi_type), [arg0] "r" (arg0), [arg1] "r" (arg1), [arg2] "r" (arg2)
        : "memory"
    );
    return ret_val;
}

/* *
 * trap - handles or dispatches an exception/interrupt. if and when trap()
 * returns,
 * the code in kern/trap/trapentry.S restores the old CPU state saved in the
 * trapframe and then uses the iret instruction to return from the exception.
 * */
void trap(struct trapframe *tf) {
    // dispatch based on what type of trap occurred
    trap_dispatch(tf);
}

