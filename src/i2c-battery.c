#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>

#include <linux/i2c.h>
#include <linux/interrupt.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Joseph D <joseph@panicnot42.com>");
MODULE_DESCRIPTION("test");

static struct i2c_device_id i2cbattery_id[] =
{
  { "i2cbattery", 0 },
  { }
};

MODULE_DEVICE_TABLE(i2c, i2cbattery_id);

static int i2cbattery_probe(struct i2c_client *client, const struct i2c_device_id *idp)
{
  // do magic
  return 0;
}

static int i2cbattery_remove(struct i2c_client *client)
{
  // more magic
  return 0;
}

static struct i2c_driver i2cbattery_driver =
{
   .driver =
   {
      .name = "i2cbattery",
   },
   .probe = i2cbattery_probe,
   .remove = i2cbattery_remove,
   .id_table = i2cbattery_id,
};

static int __init i2cbattery_init(void)
{
  printk(KERN_INFO "i2c-battery loading");
  return i2c_add_driver(&i2cbattery_driver);
}

static void __exit i2cbattery_exit(void)
{
  i2c_del_driver(&i2cbattery_driver);
  printk(KERN_INFO "i2c-battery has gone away");
}

module_init(i2cbattery_init);
module_exit(i2cbattery_exit);
