#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/err.h>
#include <linux/mutex.h>
#include <linux/sysfs.h>
#include <linux/device.h>
#include <linux/jiffies.h>

// http://lxr.free-electrons.com/source/drivers/hwmon/sht21.c
// http://cdn.sparkfun.com/datasheets/Prototyping/MAX17043-MAX17044.pdf
// http://www.linuxjournal.com/article/7252

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Joseph D <joseph@panicnot42.com>");
MODULE_DESCRIPTION("test");

#define I2C_SLAVE_ADDRESS 0x36

#define REG_VCELL_MSB   0x02 // R
#define REG_VCELL_LSB   0x03 // R
#define REG_SOC_MSB     0x04 // R
#define REG_SOC_LSB     0x05 // R
#define REG_MODE_MSB    0x06 //  W
#define REG_MODE_LSB    0x07 //  W
#define REG_VERSION_MSB 0x08 // R
#define REG_VERSION_LSB 0x09 // R
#define REG_CONFIG_MSB  0x0C // RW 97
#define REG_CONFIG_LSB  0x0D // RW 1C
#define REG_COMMAND_MSB 0xFE //  W
#define REG_COMMAND_LSB 0xFF //  W

// chip data
struct i2c_battery_data
{
  struct i2c_client* client;
  struct mutex update_lock;
  char valid; // non-zero if valid data
  unsigned long last_update;
  u16 battery_percent;
  u16 voltage;
};

static int i2c_battery_update_measurements(struct device *dev)
{
  return 0; // great success
}

#define show(value)							\
  static ssize_t i2c_battery_show_##value(struct device* dev, char* buf)\
  {\
    struct i2c_battery_data* data = dev_get_drvdata(dev);\
    int ret;\
    ret = i2c_battery_update_measurements(dev);\
    if (ret < 0)\
      return ret;\
    return sprintf(buf, "%d\n", data->value);\
  }

show(voltage);

static SENSOR_DEVICE_ATTR(in0_input, S_IWUSR | S_IRUGO, i2c_battery_show_voltage, NULL, 0);

static int i2c_battery_probe(struct i2c_client* client, const struct i2c_device_id* id)
{
  struct device* dev = &client->dev;
  struct device* hwmon_dev;
  struct i2c_battery_data* data;

  data = devm_kzalloc(dev, sizeof(*data), GFP_KERNEL);
  if (!data)
    return -ENOMEM;

  data->client = client;
  mutex_init(&data->update_lock);
  
  //hwmon_dev = devm_hwmon_device_register_with_groups(dev, client->name, data, i2c_battery_groups);

  return PTR_ERR_OR_ZERO(hwmon_dev);
}

static struct i2c_board_info i2c_battery_info[] =
{
  {  
    I2C_BOARD_INFO("i2c_battery", I2C_SLAVE_ADDRESS),
    //.irq = IRQ_GPIO(mfp_to_gpio(MFP_PIN_GPIO0)), // TODO wat
  },  
}; // can be done from device tree; see here: https://www.kernel.org/doc/Documentation/i2c/instantiating-devices

static struct i2c_device_id i2c_battery_id[] =
{
  { "i2c_battery", 0 },
  { }
};
MODULE_DEVICE_TABLE(i2c, i2c_battery_id); // registers table above for hotplugging

// our driver definition
static struct i2c_driver i2c_battery_driver =
{
  .driver.name = "i2c_battery", // name should match module
  .probe = i2c_battery_probe, // our probe
  //.remove = i2c_battery_remove, //TODO
  .id_table = i2c_battery_id, // ties in above table
};

module_i2c_driver(i2c_battery_driver);
