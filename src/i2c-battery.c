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
// https://www.kernel.org/doc/Documentation/power/power_supply_class.txt

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Joseph D <joseph@panicnot42.com>");
MODULE_DESCRIPTION("test");

#define I2C_SLAVE_ADDRESS 0x36

#define REG_VCELL_MSB     0x02 // R
#define REG_VCELL_LSB     0x03 // R
#define REG_SOC_MSB       0x04 // R
#define REG_SOC_LSB       0x05 // R
#define REG_MODE_MSB      0x06 //  W
#define REG_MODE_LSB      0x07 //  W
#define REG_VERSION_MSB   0x08 // R
#define REG_VERSION_LSB   0x09 // R
#define REG_CONFIG_MSB    0x0C // RW 97
#define REG_CONFIG_LSB    0x0D // RW 1C
#define REG_COMMAND_MSB   0xFE //  W
#define REG_COMMAND_LSB   0xFF //  W

// chip data
struct i2c_battery_data
{
  struct i2c_client* client;
  struct mutex update_lock;
  char valid; // non-zero if valid data (unused, but provided)
  unsigned long last_update; // jiffies of last update
  u16 voltage; // VCELL
  u16 soc;     // SOC (state of charge)
  u16 mode;    // MODE (w only)
  u16 version; // VERSION
  u16 config;  // CONFIG
  u16 command; // COMMAND (w only)
};

// reads all new data into drvdata
static int i2c_battery_update_measurements(struct device *dev)
{
  int res = 0;
  struct i2c_battery_data* data = dev_get_drvdata(dev);
  struct i2c_client* client = data->client;
  // these are the commands that will be sent to the chip
  u8 cmds[] = // big-endian
  {
    REG_VCELL_MSB,
    REG_VCELL_LSB,
    REG_SOC_MSB,
    REG_SOC_LSB,
    REG_CONFIG_MSB,
    REG_CONFIG_LSB,
    REG_VERSION_MSB,
    REG_VERSION_LSB
  };
  // values from chip will be read into here
  // each line corresponds to a command result
  u8 values[] = // big endian
  {
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0
  };
  // interleave the data from the two, and configure direction/length
  // we use len 1 to avoid endian mess
  // chip talks big-endian
  // we will convert to cpu endian
  struct i2c_msg msgs[] =
  {
    {
      .addr = client->addr,
      .flags = 0,
      .len = 1,
      .buf = &cmds[0]
    },
    {
      .addr = client->addr,
      .flags = 0,
      .len = 1,
      .buf = &cmds[1]
    },
    {
      .addr = client->addr,
      .flags = I2C_M_RD,
      .len = 1,
      .buf = &values[0]
    },
    {
      .addr = client->addr,
      .flags = I2C_M_RD,
      .len = 1,
      .buf = &values[1]
    },
    {
      .addr = client->addr,
      .flags = 0,
      .len = 1,
      .buf = &cmds[2]
    },
    {
      .addr = client->addr,
      .flags = 0,
      .len = 1,
      .buf = &cmds[3]
    },
    {
      .addr = client->addr,
      .flags = I2C_M_RD,
      .len = 1,
      .buf = &values[2]
    },
    {
      .addr = client->addr,
      .flags = I2C_M_RD,
      .len = 1,
      .buf = &values[3]
    },
    {
      .addr = client->addr,
      .flags = 0,
      .len = 1,
      .buf = &cmds[4]
    },
    {
      .addr = client->addr,
      .flags = 0,
      .len = 1,
      .buf = &cmds[5]
    },
    {
      .addr = client->addr,
      .flags = I2C_M_RD,
      .len = 1,
      .buf = &values[4]
    },
    {
      .addr = client->addr,
      .flags = I2C_M_RD,
      .len = 1,
      .buf = &values[5]
    },
    {
      .addr = client->addr,
      .flags = 0,
      .len = 1,
      .buf = &cmds[6]
    },
    {
      .addr = client->addr,
      .flags = 0,
      .len = 1,
      .buf = &cmds[7]
    },
    {
      .addr = client->addr,
      .flags = I2C_M_RD,
      .len = 1,
      .buf = &values[6]
    },
    {
      .addr = client->addr,
      .flags = I2C_M_RD,
      .len = 1,
      .buf = &values[7]
    }
  };

  mutex_lock(&data->update_lock);

  // chip is big-endian
  res = i2c_transfer(client->adapter, msgs, 12); // yeah, this will be a pain to debug
  if (res != 12)
    goto bail;
  data->voltage = be16_to_cpu((values[0] << 8) | (values[1] & 0xff)); // build a big-endian u16, then make it cpu-endian
  data->soc     = be16_to_cpu((values[2] << 8) | (values[3] & 0xff));
  data->config  = be16_to_cpu((values[4] << 8) | (values[5] & 0xff));
  data->version = be16_to_cpu((values[6] << 8) | (values[7] & 0xff));
  data->last_update = jiffies;
  data->valid = 1;
  
bail:
  mutex_unlock(&data->update_lock);

  return res >= 0 ? 0 : res;
}

static int i2c_battery_write_settings(struct device *dev)
{
  return 0;
}

#define show(value)                                                                                     \
  static ssize_t i2c_battery_show_##value(struct device* dev, struct device_attribute *attr, char* buf) \
  {                                                                                                     \
    struct i2c_battery_data* data = dev_get_drvdata(dev);                                               \
    int ret;                                                                                            \
    ret = i2c_battery_update_measurements(dev);                                                         \
    if (ret < 0)                                                                                        \
      return ret;                                                                                       \
    return sprintf(buf, "%d\n", data->value);                                                           \
  }
// 5 down, endian?
#define store(value)                                                                                    \
  static ssize_t i2c_battery_store_##value(struct device* dev, struct device_attribute *attr, const char* buf, size_t count) \
  {                                                                                                     \
    struct i2c_battery_data* data = dev_get_drvdata(dev);                                               \
    int ret = kstrtou16(buf, 10, &data->value);                                                         \
    if (ret < 0)                                                                                        \
      goto bail;                                                                                        \
    ret = i2c_battery_write_settings(dev);                                                              \
  bail:                                                                                                 \
    return ret;                                                                                         \
  }

show(voltage);
show(soc);
show(version);
show(config);
store(config);
store(mode);
store(command);


static SENSOR_DEVICE_ATTR(in0_input, S_IRUGO,           i2c_battery_show_voltage, NULL, 0); // defines in0_input for below
static        DEVICE_ATTR(soc,       S_IRUGO,           i2c_battery_show_soc,     NULL); // define dev_attr_soc
static        DEVICE_ATTR(version,   S_IRUGO,           i2c_battery_show_version, NULL); // define dev_attr_version
static        DEVICE_ATTR(config,    S_IRUGO | S_IWUSR, i2c_battery_show_config,  i2c_battery_store_config); // define dev_attr_config
static        DEVICE_ATTR(mode,                S_IWUSR, NULL,                     i2c_battery_store_mode); // define dev_attr_mode
static        DEVICE_ATTR(command,             S_IWUSR, NULL,                     i2c_battery_store_command); // define dev_attr_command

static struct attribute* i2c_battery_attrs[] =
{
  &sensor_dev_attr_in0_input.dev_attr.attr, // only register in0_input with hwmon
  NULL
};

ATTRIBUTE_GROUPS(i2c_battery); // define i2c_battery_groups

static int i2c_battery_probe(struct i2c_client* client, const struct i2c_device_id* id)
{
  struct device* dev = &client->dev;
  struct device* hwmon_dev;
  struct i2c_battery_data* data;

  data = devm_kzalloc(dev, sizeof(*data), GFP_KERNEL);
  if (!data)
    return -ENOMEM;

  data->client = client;
  data->valid = 0;
  mutex_init(&data->update_lock);
  
  hwmon_dev = devm_hwmon_device_register_with_groups(dev, client->name, data, i2c_battery_groups); // register hwmon group
  device_create_file(&data->client->dev, &dev_attr_soc);     // register device soc
  device_create_file(&data->client->dev, &dev_attr_version); // register device version
  device_create_file(&data->client->dev, &dev_attr_config);  // register device config
  device_create_file(&data->client->dev, &dev_attr_mode);    // register device mode
  device_create_file(&data->client->dev, &dev_attr_command); // register device command

  return PTR_ERR_OR_ZERO(hwmon_dev);
}

// specify the address for the i2c device
static struct i2c_board_info i2c_battery_info[] =
{
  {  
    I2C_BOARD_INFO("i2c_battery", I2C_SLAVE_ADDRESS)
  }
};

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
