/******************************************************************************
 * Copyright (C) 2015, The Linux Foundation. All rights reserved.
 * Copyright 2019-2023 NXP
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 ******************************************************************************/
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/delay.h>

#include "common.h"

int nfc_parse_dt(struct device *dev, struct platform_configs *nfc_configs,
		 uint8_t interface)
{
	struct device_node *np = dev->of_node;
	struct platform_gpio *nfc_gpio = &nfc_configs->gpio;
	unsigned int i = 0;

	if (!np) {
		pr_err("%s: nfc of_node NULL\n", __func__);
		return -EINVAL;
	}

	/*Initialize all the gpios to -EINVAL*/
	for (i = 0; i < sizeof(struct platform_gpio) / sizeof(unsigned int);
	     i++) {
		*((unsigned int *)nfc_gpio + i) = -EINVAL;
	}
	/* irq required for i2c based chips only */
	if (interface == PLATFORM_IF_I2C || interface == PLATFORM_IF_SPI) {
		nfc_gpio->irq = of_get_named_gpio(np, DTS_IRQ_GPIO_STR, 0);
		if ((!gpio_is_valid(nfc_gpio->irq))) {
			pr_err("%s: irq gpio invalid %d\n", __func__,
			       nfc_gpio->irq);
			return -EINVAL;
		}
		pr_info("%s: irq %d\n", __func__, nfc_gpio->irq);
	}
	nfc_gpio->ven = of_get_named_gpio(np, DTS_VEN_GPIO_STR, 0);
	if ((!gpio_is_valid(nfc_gpio->ven))) {
		pr_err("%s: ven gpio invalid %d\n", __func__, nfc_gpio->ven);
		return -EINVAL;
	}

	nfc_gpio->i2c_sw = of_get_named_gpio(np, DTS_I2C_SW_STR, 0);
	if ((!gpio_is_valid(nfc_gpio->i2c_sw)))
		pr_warn("%s: i2c_sw gpio invalid %d\n", __func__,
			nfc_gpio->i2c_sw);

	nfc_gpio->mode_sw_nfcc = of_get_named_gpio(np, DTS_MODE_SW_STR, 0);
	if ((!gpio_is_valid(nfc_gpio->mode_sw_nfcc)))
		pr_warn("%s: mode_sw_nfcc gpio invalid %d\n", __func__,
			nfc_gpio->mode_sw_nfcc);

	nfc_gpio->mode_sw_smcu = of_get_named_gpio(np, DTS_MODE_SW_SP_STR, 0);
	if ((!gpio_is_valid(nfc_gpio->mode_sw_smcu)))
		pr_warn("%s: mode_sw_smcu gpio invalid %d\n", __func__,
			nfc_gpio->mode_sw_smcu);

	nfc_gpio->mode_sw_smcu_done =
	    of_get_named_gpio(np, DTS_MODE_SW_SP_DONE_STR, 0);
	if ((!gpio_is_valid(nfc_gpio->mode_sw_smcu_done)))
		pr_warn("%s: mode_sw_sp gpio invalid %d\n", __func__,
			nfc_gpio->mode_sw_smcu_done);

	nfc_gpio->led_red = of_get_named_gpio(np, DTS_RED_LED_STR, 0);
	if ((!gpio_is_valid(nfc_gpio->led_red)))
		pr_warn("%s: led_red gpio invalid %d\n", __func__,
			nfc_gpio->led_red);

	nfc_gpio->led_green = of_get_named_gpio(np, DTS_GREEN_LED_STR, 0);
	if ((!gpio_is_valid(nfc_gpio->led_green)))
		pr_warn("%s: led_green gpio invalid %d\n", __func__,
			nfc_gpio->led_green);
	pr_info("%s: %d, %d, %d, %d, %d, %d, %d, %d\n", __func__, nfc_gpio->irq,
		nfc_gpio->ven, nfc_gpio->i2c_sw, nfc_gpio->mode_sw_nfcc,
		nfc_gpio->mode_sw_smcu, nfc_gpio->mode_sw_smcu_done,
		nfc_gpio->led_red, nfc_gpio->led_green);

	return 0;
}

void set_valid_gpio(int gpio, int value)
{
	if (gpio_is_valid(gpio)) {
		pr_debug("%s: gpio %d value %d\n", __func__, gpio, value);
		gpio_set_value(gpio, value % 2);
		/* hardware dependent delay */
		usleep_range(NFC_GPIO_SET_WAIT_TIME_US,
			     NFC_GPIO_SET_WAIT_TIME_US + 100);
	}
}

int get_valid_gpio(int gpio)
{
	int value = -EINVAL;

	if (gpio_is_valid(gpio)) {
		value = gpio_get_value(gpio);
		pr_debug("%s: gpio %d value %d\n", __func__, gpio, value);
	}
	return value;
}

void gpio_set_ven(struct nfc_dev *nfc_dev, int value)
{
	struct platform_gpio *nfc_gpio = &nfc_dev->configs.gpio;

	if (gpio_get_value(nfc_gpio->ven) != value) {
		pr_debug("%s: value %d\n", __func__, value);

		gpio_set_value(nfc_gpio->ven, value);
		/* hardware dependent delay */
		usleep_range(NFC_GPIO_SET_WAIT_TIME_US,
			     NFC_GPIO_SET_WAIT_TIME_US + 100);
	}
}

int configure_gpio(unsigned int gpio, int flag)
{
	int ret;

	pr_debug("%s: nfc gpio [%d] flag [%01x]\n", __func__, gpio, flag);
	if (gpio_is_valid(gpio)) {
		ret = gpio_request(gpio, "nfc_gpio");
		if (ret) {
			pr_err("%s: unable to request nfc gpio [%d]\n",
			       __func__, gpio);
			return ret;
		}
		/* set direction and value for output pin */
		if (flag & GPIO_OUTPUT) {
			ret = gpio_direction_output(gpio, (GPIO_HIGH & flag));
			pr_debug("%s: nfc o/p gpio %d level %d\n", __func__,
				 gpio, gpio_get_value(gpio));
		} else {
			ret = gpio_direction_input(gpio);
			pr_debug("%s: nfc i/p gpio %d\n", __func__, gpio);
		}

		if (ret) {
			pr_err
			    ("%s: unable to set direction for nfc gpio [%d]\n",
			     __func__, gpio);
			gpio_free(gpio);
			return ret;
		}
		/* Consider value as control for input IRQ pin */
		if (flag & GPIO_IRQ) {
			ret = gpio_to_irq(gpio);
			if (ret < 0) {
				pr_err("%s: unable to set irq [%d]\n", __func__,
				       gpio);
				gpio_free(gpio);
				return ret;
			}
			pr_debug("%s: gpio_to_irq successful [%d]\n", __func__,
				 gpio);
			return ret;
		}
	} else {
		pr_err("%s: invalid gpio\n", __func__);
		ret = -EINVAL;
	}
	return ret;
}

/* Configuring board leds*/
int configure_leds(struct platform_gpio *nfc_gpio)
{
	int ret = 0;

	ret = configure_gpio(nfc_gpio->led_green, GPIO_OUTPUT);
	if (ret) {
		pr_err
			("%s: unable to request mode switch sp interface gpio [%d]\n",
			 __func__, nfc_gpio->led_green);
	}
	ret = configure_gpio(nfc_gpio->led_red, GPIO_OUTPUT);
	if (ret) {
		pr_err
			("%s: unable to request mode switch sp interface gpio [%d]\n",
			 __func__, nfc_gpio->led_red);
	}
	return ret;
}

void gpio_free_all(struct nfc_dev *nfc_dev)
{
	struct platform_gpio *nfc_gpio = &nfc_dev->configs.gpio;
	unsigned int i = 0;

	for (i = 0; i < sizeof(struct platform_gpio) / sizeof(unsigned int);
	     i++) {
		if (gpio_is_valid(*((unsigned int *)nfc_gpio + i)))
			gpio_free(*((unsigned int *)nfc_gpio + i));
	}
}

void nfc_misc_unregister(struct nfc_dev *nfc_dev, int count)
{
	pr_debug("%s: entry\n", __func__);
	device_destroy(nfc_dev->nfc_class, nfc_dev->devno);
	cdev_del(&nfc_dev->c_dev);
	class_destroy(nfc_dev->nfc_class);
	unregister_chrdev_region(nfc_dev->devno, count);
}

int nfc_misc_register(struct nfc_dev *nfc_dev,
		      const struct file_operations *nfc_fops, int count,
		      char *devname, char *classname)
{
	int ret = 0;

	ret = alloc_chrdev_region(&nfc_dev->devno, 0, count, devname);
	if (ret < 0) {
		pr_err("%s: failed to alloc chrdev region ret %d\n", __func__,
		       ret);
		return ret;
	}
	nfc_dev->nfc_class = class_create(THIS_MODULE, classname);
	if (IS_ERR(nfc_dev->nfc_class)) {
		ret = PTR_ERR(nfc_dev->nfc_class);
		pr_err("%s: failed to register device class ret %d\n", __func__,
		       ret);
		unregister_chrdev_region(nfc_dev->devno, count);
		return ret;
	}
	cdev_init(&nfc_dev->c_dev, nfc_fops);
	ret = cdev_add(&nfc_dev->c_dev, nfc_dev->devno, count);
	if (ret < 0) {
		pr_err("%s: failed to add cdev ret %d\n", __func__, ret);
		class_destroy(nfc_dev->nfc_class);
		unregister_chrdev_region(nfc_dev->devno, count);
		return ret;
	}
	nfc_dev->nfc_device = device_create(nfc_dev->nfc_class, NULL,
					    nfc_dev->devno, nfc_dev, devname);
	if (IS_ERR(nfc_dev->nfc_device)) {
		ret = PTR_ERR(nfc_dev->nfc_device);
		pr_err("%s: failed to create the device ret %d\n", __func__,
		       ret);
		cdev_del(&nfc_dev->c_dev);
		class_destroy(nfc_dev->nfc_class);
		unregister_chrdev_region(nfc_dev->devno, count);
		return ret;
	}
	return 0;
}

static int led_switch_control_ioctl(struct nfc_dev *nfc_dev, unsigned long arg)
{
	int ret = 0;
	struct platform_gpio *nfc_gpio = &nfc_dev->configs.gpio;

	if (arg == RED_LED_ON || arg == RED_LED_OFF) {
		set_valid_gpio(nfc_gpio->led_red, arg);
	} else if (arg == GREEN_LED_ON || arg == GREEN_LED_OFF) {
		set_valid_gpio(nfc_gpio->led_green, arg);
	} else {
		pr_err("%s: bad arg %lu\n", __func__, arg);
		ret = -ENOIOCTLCMD;
	}
	return ret;

}

static int mode_switch_smcu_ioctl(struct nfc_dev *nfc_dev, unsigned long arg)
{
	int ret = 0;
	struct platform_gpio *nfc_gpio = &nfc_dev->configs.gpio;

	if (arg == NCI_MODE) {
		set_valid_gpio(nfc_gpio->mode_sw_smcu, 0);
	} else if (arg == EMVCO_MODE) {
		set_valid_gpio(nfc_gpio->mode_sw_smcu, 1);
		smcu_done_enable_irq(nfc_dev);
	} else {
		pr_err("%s: bad arg %lu\n", __func__, arg);
		ret = -ENOIOCTLCMD;
	}
	return ret;
}

static int mode_switch_nfcc_ioctl(struct nfc_dev *nfc_dev, unsigned long arg)
{
	int ret = 0;
	struct platform_gpio *nfc_gpio = &nfc_dev->configs.gpio;

	if (arg == NCI_MODE) {
		set_valid_gpio(nfc_gpio->mode_sw_nfcc, 0);
	} else if (arg == EMVCO_MODE) {
		set_valid_gpio(nfc_gpio->mode_sw_nfcc, 1);
	} else {
		pr_err("%s: bad arg %lu\n", __func__, arg);
		ret = -ENOIOCTLCMD;
	}
	return ret;
}

/**
 * nfc_ioctl_power_states() - power control
 * @nfc_dev:    nfc device data structure
 * @arg:    mode that we want to move to
 *
 * Device power control. Depending on the arg value, device moves to
 * different states, refer common.h for args
 *
 * Return: -ENOIOCTLCMD if arg is not supported, 0 in any other case
 */
static int nfc_ioctl_power_states(struct nfc_dev *nfc_dev, unsigned long arg)
{
	int ret = 0;
	// struct platform_gpio *nfc_gpio = &nfc_dev->configs.gpio;

	if (arg == NFC_POWER_OFF) {
		/*
		 * We are attempting a hardware reset so let us disable
		 * interrupts to avoid spurious notifications to upper
		 * layers.
		 */
		nfc_dev->nfc_disable_intr(nfc_dev);
		gpio_set_ven(nfc_dev, 0);
		nfc_dev->nfc_ven_enabled = false;
	} else if (arg == NFC_POWER_ON) {
		nfc_dev->nfc_enable_intr(nfc_dev);
		gpio_set_ven(nfc_dev, 1);
		nfc_dev->nfc_ven_enabled = true;
	}  else if (arg == NFC_FW_DWL_VEN_TOGGLE) {
		/*
		 * We are switching to download Mode, toggle the enable pin
		 * in order to set the NFCC in the new mode
		 */
		nfc_dev->nfc_disable_intr(nfc_dev);
		nfc_dev->nfc_state = NFC_STATE_FW_DWL;
		gpio_set_ven(nfc_dev, 0);
		gpio_set_ven(nfc_dev, 1);
		nfc_dev->nfc_enable_intr(nfc_dev);
	} else if (arg == NFC_FW_DWL_HIGH) {
		/*
		 * Setting firmware download gpio to HIGH
		 * before FW download start
		 */
		nfc_dev->nfc_state = NFC_STATE_FW_DWL;

	} else if (arg == NFC_FW_DWL_LOW) {
		/*
		 * Setting firmware download gpio to LOW
		 * FW download finished
		 */
		nfc_dev->nfc_state = NFC_STATE_NCI;
	} else if (arg == NFC_VEN_FORCED_HARD_RESET) {
		nfc_dev->nfc_disable_intr(nfc_dev);
		gpio_set_ven(nfc_dev, 0);
		gpio_set_ven(nfc_dev, 1);
		nfc_dev->nfc_enable_intr(nfc_dev);
	} else {
		pr_err("%s: bad arg %lu\n", __func__, arg);
		ret = -ENOIOCTLCMD;
	}
	return ret;
}

static int smcu_fw_dnld_triggered_ioctl(struct nfc_dev *nfc_dev, unsigned long arg)
{
	int ret = 0;
    int timeout = (uint32_t)arg;

	pr_info("%s: secure mcu firmware download started",__func__);
	ret = wait_event_interruptible_timeout(nfc_dev->smcu_fw_dnld_wq,
				nfc_dev->smcu_fw_dnld_triggered, msecs_to_jiffies(timeout));
	if (ret <= 0) {
		pr_err("%s smcu firmware download timeout elapsed", __func__);
		set_valid_gpio(nfc_dev->configs.gpio.mode_sw_nfcc,0);
		gpio_set_ven(nfc_dev, 0);
		gpio_set_ven(nfc_dev, 1);
		set_valid_gpio(nfc_dev->configs.gpio.mode_sw_smcu,0);
		smcu_done_disable_irq(nfc_dev);
		return -ETIMEDOUT;
	} else {
			pr_info("%s: secure mcu firmware download completed successfully",__func__);
			nfc_dev->smcu_fw_dnld_triggered = false;
		}
	return ret;
}
/**
 * nfc_dev_ioctl - used to set or get data from upper layer.
 * @pfile   file node for opened device.
 * @cmd     ioctl type from upper layer.
 * @arg     ioctl arg from upper layer.
 *
 * NFC and ESE Device power control, based on the argument value
 *
 * Return: -ENOIOCTLCMD if arg is not supported, 0 or other in any other case
 */
long nfc_dev_ioctl(struct file *pfile, unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	struct nfc_dev *nfc_dev = pfile->private_data;

	if (!nfc_dev)
		return -ENODEV;

	pr_debug("%s: cmd = %x arg = %zx\n", __func__, cmd, arg);
	if (cmd == NFC_SET_PWR) {
		ret = nfc_ioctl_power_states(nfc_dev, arg);
	} else if (cmd == NFCC_PROFILE_SWITCH) {
		ret = mode_switch_nfcc_ioctl(nfc_dev, arg);
	} else if (cmd == SMCU_PROFILE_SWITCH) {
		ret = mode_switch_smcu_ioctl(nfc_dev, arg);
	} else if (cmd == LEDS_CONTROL) {
		ret = led_switch_control_ioctl(nfc_dev, arg);
	} else if (cmd == SMCU_FW_DNLD_TRIGGERED) {
		ret = smcu_fw_dnld_triggered_ioctl(nfc_dev, arg);
	} else {
		pr_err("%s: bad cmd %lu\n", __func__, arg);
		ret = -ENOIOCTLCMD;
	}
	return ret;
}

int nfc_dev_open(struct inode *inode, struct file *filp)
{
	struct nfc_dev *nfc_dev =
	    container_of(inode->i_cdev, struct nfc_dev, c_dev);

	pr_debug("%s: %d, %d\n", __func__, imajor(inode), iminor(inode));

	mutex_lock(&nfc_dev->dev_ref_mutex);

	filp->private_data = nfc_dev;

	if (nfc_dev->dev_ref_count == 0)
		nfc_dev->nfc_enable_intr(nfc_dev);
	nfc_dev->dev_ref_count = nfc_dev->dev_ref_count + 1;
	mutex_unlock(&nfc_dev->dev_ref_mutex);
	return 0;
}

int nfc_dev_close(struct inode *inode, struct file *filp)
{
	struct nfc_dev *nfc_dev =
	    container_of(inode->i_cdev, struct nfc_dev, c_dev);

	pr_debug("%s: %d, %d\n", __func__, imajor(inode), iminor(inode));
	mutex_lock(&nfc_dev->dev_ref_mutex);
	if (nfc_dev->dev_ref_count == 1)
		nfc_dev->nfc_disable_intr(nfc_dev);
	if (nfc_dev->dev_ref_count > 0)
		nfc_dev->dev_ref_count = nfc_dev->dev_ref_count - 1;
	filp->private_data = NULL;

	mutex_unlock(&nfc_dev->dev_ref_mutex);
	return 0;
}

int nfc_dev_flush(struct file *pfile, fl_owner_t id)
{
	struct nfc_dev *nfc_dev = pfile->private_data;

	if (!nfc_dev)
		return -ENODEV;
	/*
	 * release blocked user thread waiting for pending read during close
	 */
	if (!mutex_trylock(&nfc_dev->read_mutex)) {
		nfc_dev->release_read = true;
		nfc_dev->nfc_disable_intr(nfc_dev);
		wake_up(&nfc_dev->read_wq);
		pr_debug("%s: waiting for release of blocked read\n", __func__);
		mutex_lock(&nfc_dev->read_mutex);
		nfc_dev->release_read = false;
	} else {
		pr_debug("%s: read thread already released\n", __func__);
	}
	mutex_unlock(&nfc_dev->read_mutex);
	return 0;
}

int validate_nfc_state_nci(struct nfc_dev *nfc_dev)
{
	struct platform_gpio *nfc_gpio = &nfc_dev->configs.gpio;

	if (!gpio_get_value(nfc_gpio->ven)) {
		pr_err("%s: ven low - nfcc powered off\n", __func__);
		return -ENODEV;
	}
	if (nfc_dev->nfc_state != NFC_STATE_NCI) {
		pr_err("%s: fw download state\n", __func__);
		return -EBUSY;
	}
	return 0;
}
