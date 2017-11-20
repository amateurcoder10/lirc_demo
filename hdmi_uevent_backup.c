#define pr_fmt(fmt)	KBUILD_MODNAME ":%s " fmt, __func__
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/sysfs.h>
#include <asm/uaccess.h>
#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/major.h>
#include <linux/list.h>
#include <linux/types.h>
#include <linux/kobject.h>
#include <linux/device.h>
#include <linux/timer.h>
#include <linux/gpio.h>
#include <asm/gpio.h>

#define GPIO_HDMI 46
static struct timer_list my_timer;
static int hdmi_pin;
static struct device *dev;
static char *envp[]={"SUBSYSTEM=HDMI","attr{gpu_support}=Broadcom:VideoCore_IV","attr{cpu_support}=Broadcom:BCM2837","MODALIAS=platform:lirccustom_alias_rpi",NULL};

static ssize_t read_cpu_support (struct device *dev, struct device_attribute *attr,char *buf){
	return sprintf(buf,"Broadcom:BCM2837\n");
}
static ssize_t read_gpu_support (struct device *dev, struct device_attribute *attr,char *buf){
	return sprintf(buf,"Broadcom:VideoCore_IV\n");
}
static ssize_t read_hdmi_status (struct device *dev, struct device_attribute *attr,char *buf){
	if(hdmi_pin==0)
		return sprintf(buf,"Connected\n");
	else if(hdmi_pin==1)
		return sprintf(buf,"Disconnected\n");
	else
		return sprintf(buf,"Error\n");
}

static DEVICE_ATTR(cpu_support, S_IRUSR|S_IRGRP, read_cpu_support, NULL);
static DEVICE_ATTR(gpu_support, S_IRUSR|S_IRGRP, read_gpu_support, NULL);
static DEVICE_ATTR(hdmi_status, S_IRUSR|S_IRGRP, read_hdmi_status, NULL);

static struct attribute *dev_attrs[] = {
	&dev_attr_hdmi_status.attr,
	&dev_attr_cpu_support.attr,
	&dev_attr_gpu_support.attr,
	NULL
};
static struct attribute_group dev_attr_group = {
	.attrs = dev_attrs,NULL
};
static const struct attribute_group *dev_attr_groups[] = {
	&dev_attr_group,
	NULL
};
static struct miscdevice hdmi_dev = {
	.name = "hdmi_device",
	.minor = MISC_DYNAMIC_MINOR,
};
static void gpio_init(void){
	if(!gpio_is_valid(GPIO_HDMI)){
		printk(KERN_INFO"Invalid GPIO %d\n",GPIO_HDMI);
	}
	else printk(KERN_INFO"Valid GPIO %d\n",GPIO_HDMI);
	gpio_request(GPIO_HDMI,"sysfs");
	gpio_direction_input(GPIO_HDMI);
	gpio_export(GPIO_HDMI,false);
}
static void my_timer_func(unsigned long dummy){
	int error,curr_val;
	curr_val = gpio_get_value(GPIO_HDMI);
	if(hdmi_pin!=curr_val){
		hdmi_pin = curr_val;
		if(hdmi_pin==0){
			error = kobject_uevent_env(&dev->kobj,KOBJ_ADD,envp);
			if (error){
				kobject_put(&dev->kobj);
				pr_err("No kobject_uevent %d\n", error);
			}	
		}
		else if(hdmi_pin==1){
			error = kobject_uevent_env(&dev->kobj,KOBJ_REMOVE,envp);
			if (error){
				kobject_put(&dev->kobj);
				pr_err("No kobject_uevent %d\n", error);
			}	
		} 
	}
	printk("Inside callback Function Timer = %lu %d\n",jiffies,gpio_get_value(GPIO_HDMI));
	error = mod_timer( &my_timer, jiffies + msecs_to_jiffies(3000) );
	if (error) printk("Error in mod_timer\n");
}
static int __init module_start(void)
{
	int error=0;

	pr_info("\n");

	error = misc_register(&hdmi_dev);
	if (error)
		pr_err("error %d\n", error);
	dev = hdmi_dev.this_device;
	error = device_create_file(dev,&dev_attr_hdmi_status);
	if (error){
		put_device(dev);
		pr_err("No Read device attribute Added%d\n", error);
	}
	error = device_create_file(dev,&dev_attr_cpu_support);
	if (error){
		put_device(dev);
		pr_err("No Change device attribute Added%d\n", error);
	}
	error = device_create_file(dev,&dev_attr_gpu_support);
	if (error){
		put_device(dev);
		pr_err("No Change device attribute Added%d\n", error);
	}
	gpio_init();
	setup_timer(&my_timer,my_timer_func,0);
	hdmi_pin = gpio_get_value(GPIO_HDMI);
	error = mod_timer( &my_timer, jiffies + msecs_to_jiffies(3000));
	if (error) printk("Error in mod_timer\n");
	return error;
}

static void __exit module_end(void)
{
	int error;
	misc_deregister(&hdmi_dev);
	gpio_unexport(GPIO_HDMI);
	gpio_free(GPIO_HDMI); 
	error = del_timer(&my_timer);
	if(error) printk("The timer is still in use...\n");
	pr_info("\n");
}

module_init(module_start);
module_exit(module_end);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("HDMI Device");
MODULE_ALIAS("platform:rpi2");
