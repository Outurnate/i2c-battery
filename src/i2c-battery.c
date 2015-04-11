#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>

// http://lxr.free-electrons.com/source/drivers/hwmon/sht21.c
// http://cdn.sparkfun.com/datasheets/Prototyping/MAX17043-MAX17044.pdf
// http://www.linuxjournal.com/article/7252

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Joseph D <joseph@panicnot42.com>");
MODULE_DESCRIPTION("test");

static int i2c_battery_probe(struct i2c_client* client, const struct i2c_device_id* id)
{
}

// chip data
struct i2c_battery_data
{
  struct mutex update_lock;
  char valid; // non-zero if valid data
  unsigned long last_update;
  u16 battery_percent;
};

static struct i2c_device_id i2c_battery_id[] =
{
  { "i2c_battery", 0 },
  { }
};
MODULE_DEVICE_TABLE(i2c, i2c_battery_id); // no idea...

// our driver definition
static struct i2c_driver i2c_battery_driver =
{
  .driver.name = "i2c_battery", // name should match module
  .probe = i2c_battery_probe, // our probe
  .id_table = i2c_battery_id, // ties in above table
};

module_i2c_driver(i2c_battery_driver);
