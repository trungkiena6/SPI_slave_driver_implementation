#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/of_device.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/pm_domain.h>

#include "spi-slave-core.h"
#define DRIVER_NAME "spislavecore"

static int spislave_drv_probe(struct device *dev)
{
	int				ret = 0;
	const struct spislave_driver	*sdrv;
	struct spi_slave		*sdev;

	sdrv = to_spislave_drv(dev->driver);
	sdev = to_spislave_dev(dev);

	ret = dev_pm_domain_attach(dev, true);
	if (ret != -EPROBE_DEFER) {
		ret = sdrv->probe(sdev);
		if (ret)
			dev_pm_domain_detach(dev, true);
	}

	return ret;
}

static int spislave_drv_remove(struct device *dev)
{
	int			ret = 0;
	const struct spislave_driver	*sdrv;
	struct spi_slave		*sdev;

	sdrv = to_spislave_drv(dev->driver);
	sdev = to_spislave_dev(dev);

	ret = sdrv->remove(sdev);
	dev_pm_domain_detach(dev, true);

	return ret;
}

int spislave_register_driver(struct spislave_driver *sdrv)
{
	sdrv->driver.owner = THIS_MODULE;
	sdrv->driver.bus = &spislave_bus_type;

	sdrv->driver.probe = spislave_drv_probe;
	sdrv->driver.remove = spislave_drv_remove;

	return driver_register(&sdrv->driver);
}
EXPORT_SYMBOL_GPL(spislave_register_driver);

void spislave_unregister_driver(struct spislave_driver *sdrv)
{
	driver_unregister(&sdrv->driver);
}
EXPORT_SYMBOL_GPL(spislave_unregister_driver);

void spislave_unregister_slave(struct spi_slave *slave)
{
	device_unregister(&slave->dev);
}

static void devm_spislave_unregister(struct device *dev, void *res)
{
	spislave_unregister_slave(*(struct spi_slave **)res);
}

int spislave_register_slave(struct spi_slave *slave)
{
	int ret = 0;

	slave->dev.bus = &spislave_bus_type;
	dev_set_name(&slave->dev, "%s", slave->name);
	ret = device_register(&slave->dev);

	return ret;
}

int devm_spislave_register_device(struct device *dev, const char *name,
				  struct spi_slave *slave)
{
	int			ret = 0;
	struct spi_slave	**ptr;

	ptr = devres_alloc(devm_spislave_unregister, sizeof(*ptr), GFP_KERNEL);
	if (!ptr) {
		pr_err("%s: devers alloc error\n", DRIVER_NAME);
		return -ENOMEM;
	}

	pr_info("%s: devres alloc ok\n", DRIVER_NAME);

	ret = spislave_register_slave(slave);
	if (!ret) {
		*ptr = slave;
		devres_add(dev, ptr);
	} else {
		devres_free(ptr);
	}

	return ret;
}
EXPORT_SYMBOL_GPL(devm_spislave_register_device);


void spislave_unregister_device(struct spi_slave *slave)
{
	device_unregister(&slave->dev);
}
EXPORT_SYMBOL_GPL(spislave_unregister_device);

static int spislave_device_match(struct device *dev,
				 struct device_driver *drv)
{
	struct device_node			*nc;

	pr_info("%s: device match", DRIVER_NAME);

	for_each_available_child_of_node(dev->of_node, nc) {
		if (of_node_test_and_set_flag(nc, OF_POPULATED))
			continue;

		pr_info("%s: chid node is found: %s\n", DRIVER_NAME,
			nc->full_name);
	}

	return 1;
}

struct bus_type spislave_bus_type = {
	.name = "spislave",
	.match = spislave_device_match,
};

static int __init spislave_init(void)
{
	int			ret = 0;

	pr_info("%s: init\n", DRIVER_NAME);

	ret = bus_register(&spislave_bus_type);

	return ret;
}

static void __exit spislave_exit(void)
{
	bus_unregister(&spislave_bus_type);
	pr_info("%s: exit\n", DRIVER_NAME);
}

module_init(spislave_init);
module_exit(spislave_exit);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Patryk Mezydlo, <mezydlo.p@gmail.com>");
MODULE_DESCRIPTION("User mode SPI slave device interface");
MODULE_VERSION("1.0");
