#include <linux/module.h>
#include <linux/fs.h>
#include <asm/io.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>
#include <linux/uaccess.h>

#define NOD_NAME "nobrand"
#define NOD_MAJOR 100

MODULE_LICENSE("GPL");

static volatile uint32_t *BASE;
static volatile uint32_t *GPFSEL1,*GPFSEL2;
static volatile uint32_t *GPIO_PUP_PDN_CNTRL_REG1;
static volatile uint32_t *GPLEV0;
static volatile uint32_t *GPSET0;
static volatile uint32_t *GPCLR0;

static int irq_num1;
static int irq_num2;
static int app_pid;

static struct kernel_siginfo sig_info1;
static struct kernel_siginfo sig_info2;
static struct task_struct *task1;
static struct task_struct *task2;

static irqreturn_t btn1_gogo(int irq, void *data){
	printk(KERN_ALERT "It's Button1\n");
	if(app_pid>0){
		task1 = pid_task(find_vpid(app_pid), PIDTYPE_PID);
		send_sig_info(SIGIO, &sig_info1, task1);
	}
	return IRQ_HANDLED;
} //btn1 interrupt function

static irqreturn_t btn2_gogo(int irq, void *data){
	printk(KERN_ALERT "It's Button2\n");

	if(app_pid > 0){
		task2 = pid_task(find_vpid(app_pid), PIDTYPE_PID);
		send_sig_info(SIGUSR1, &sig_info2, task2);
	}

	return IRQ_HANDLED;
} //btn2 interrupt function

static int nobrand_open(struct inode *inode, struct file *filp) {
    printk( KERN_INFO "OPEN!!!\n");
    return 0;
} //device driver open

static int nobrand_release(struct inode *inode, struct file *filp) {
    printk( KERN_INFO "CLOSE!!!\n");
    return 0;
} //device dirver close
struct Node{
	int a,b,c,d;
}; 

static long nobrand_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int lev1,lev2;
    switch (cmd)
    {
		case _IO(0,3) :
			lev1 = (*GPLEV0 >> 21) & 0x1; //button1 connect ioctl
			lev2 = (*GPLEV0 >> 20) & 0x1; //button2 connect ioctl
			printk(KERN_INFO "GPIO21 LEV = %d\n", lev1);
			printk(KERN_INFO "GPIO20 LEV = %d\n", lev2);
			break;
		case _IO(0,4) :
			app_pid = (int)arg;
			break;
		case _IO(0,5) :
			*GPSET0 |= (0x1 << 18); //led1 on ioctl
			*GPSET0 |= (0x1 << 15); //led2 on ioctl
			break;
		case _IO(0, 6) :
			*GPCLR0 |= (0x1 << 18); //led2 off ioctl
			*GPCLR0 |= (0x1 << 15); //led2 off ioctl
			break;
		case _IO(0, 7) :
			*GPCLR0 |= (0x1 << 15); //led2 off ioctl
			*GPSET0 |= (0x1 << 18); //led1 on ioctl
			break;
		case _IO(0, 8) :
			*GPCLR0 |= (0x1 << 18); //led1 off ioctl
			*GPSET0 |= (0x1 << 15); //led2 on ioctl
			break;
	} 

    printk( KERN_ALERT "%d\n", cmd); //command
    return 0;
}

static struct file_operations fops = { //file operations
    .open = nobrand_open,
    .release = nobrand_release,
    .unlocked_ioctl = nobrand_ioctl
};

static void myinit(void){

	BASE = (uint32_t*)ioremap(0xFE200000, 256);
	GPFSEL1 = BASE + (0x04 / 4); //bit connect by using datasheet
	GPFSEL2 = BASE + (0x08 / 4);
	
	*GPFSEL1 &= ~(0x7 << 24); //GPIO 18
    *GPFSEL1 |= (1 << 24); 
	*GPFSEL1 &= ~(0x7 << 15); //GPIO 15
	*GPFSEL1 |= (1 << 15);	

	*GPFSEL2 &= ~(0x07 << 3);
	*GPFSEL2 &= ~(0x07);	

	GPSET0 = BASE + (0x1C / 4);
    GPCLR0 = BASE + (0x28 / 4);
	
	GPIO_PUP_PDN_CNTRL_REG1 = BASE +(0xe8/4);
	*GPIO_PUP_PDN_CNTRL_REG1 &= ~(0x03<<10);
	*GPIO_PUP_PDN_CNTRL_REG1 |= (0x01<<10);
	*GPIO_PUP_PDN_CNTRL_REG1 &= ~(0x03 << 8);
	*GPIO_PUP_PDN_CNTRL_REG1 |= (0x01 << 8);
	
	GPLEV0 = BASE + (0x34/4);
}

static int nobrand_init(void) {
	int ret1,ret2;	
	myinit();
	
	gpio_direction_input(21); //gpio 21
	gpio_direction_input(20); //gpio 20

	irq_num1 = gpio_to_irq(21); //interrupt
	irq_num2 = gpio_to_irq(20); 
	ret1 = request_irq(irq_num1, btn1_gogo, IRQF_TRIGGER_RISING, "k~f~c", NULL);
	ret2 = request_irq(irq_num2, btn2_gogo, IRQF_TRIGGER_RISING, "k~f~c", NULL);
	
    printk( KERN_INFO "OK HELLO NOBRAND!!\n");

    if (register_chrdev(NOD_MAJOR, NOD_NAME, &fops) < 0) { //error handling
        printk( KERN_INFO "ERROR!!! register error\n");
    }

    return 0;
}

static void nobrand_exit(void) { //exit
	
	free_irq(irq_num1, NULL);
	*GPCLR0 = (1 << 18);
	*GPCLR0 = (1 << 15);
	iounmap(BASE);
    unregister_chrdev(NOD_MAJOR, NOD_NAME);
    printk( KERN_INFO "BYE BYE\n\n");
}

module_init(nobrand_init);
module_exit(nobrand_exit);
