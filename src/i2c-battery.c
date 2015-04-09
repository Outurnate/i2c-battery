#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Joseph D <joseph@panicnot42.com>");
MODULE_DESCRIPTION("test");

static int __init i2cbattery_init(void)
{
  printk(KERN_INFO "i2c-battery has loadedn");
  return 0;
}

static void __exit i2cbattery_exit(void)
{
  printk(KERN_INFO "i2c-battery has gone awayn");
}

module_init(i2cbattery_init);
module_exit(i2cbattery_exit);
