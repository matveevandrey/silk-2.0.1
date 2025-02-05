#define pr_fmt(fmt)	KBUILD_MODNAME ": " fmt


#include <linux/time.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/usb.h>
#include <linux/reboot.h>
#include <linux/version.h>
#include "config.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Greg Kroah-Hartman and Nate Brune");
MODULE_DESCRIPTION("A module that protects you from having a terrible horrible no good very bad day.");


//as in /kernel/sys.c:do_sysinfo
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 13, 0))
static __kernel_old_time_t get_uptime(void)
{
    struct timespec64 uptime;
    ktime_get_boottime_ts64(&uptime);
    return uptime.tv_sec + (uptime.tv_nsec ? 1 : 0);
}
#else

	#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 20, 0))
	static __kernel_time_t get_uptime(void)
	{
	    struct timespec64 uptime;
	    ktime_get_boottime_ts64(&uptime);
	    return uptime.tv_sec + (uptime.tv_nsec ? 1 : 0);
	}
	#else
	static __kernel_time_t get_uptime(void)
	{
	    struct timespec uptime;
	    get_monotonic_boottime(&uptime);
	    return uptime.tv_sec + (uptime.tv_nsec ? 1 : 0);
	}

	#endif
#endif



static void panic_time(struct usb_device *usb)
{
	int i;
	struct device *dev;

	pr_info("shredding...\n");
	for (i = 0; remove_files[i] != NULL; ++i) {
		char *shred_argv[] = {
			"/usr/bin/shred",
			"-f", "-u", "-n",
			shredIterations,
			remove_files[i],
			NULL,
		};
		call_usermodehelper(shred_argv[0], shred_argv,
				    NULL, UMH_WAIT_EXEC);
	}
	pr_info("...done.\n");

	#if (LINUX_VERSION_CODE > KERNEL_VERSION(4, 14, 0))

	for (dev = &usb->dev; dev; dev = dev->parent) {
		mutex_unlock(&dev->mutex);
	}

	#endif

	pr_info("Syncing & powering off.\n");

	call_usermodehelper(shutdown_argv[0], shutdown_argv, NULL, UMH_NO_WAIT);

}

/*
 * returns 0 if no match, 1 if match
 *
 * Taken from drivers/usb/core/driver.c, as it's not exported for our use :(
 */
static int usb_match_device(struct usb_device *dev,
			    const struct usb_device_id *id)
{
	if ((id->match_flags & USB_DEVICE_ID_MATCH_VENDOR) &&
	    id->idVendor != le16_to_cpu(dev->descriptor.idVendor))
		return 0;

	if ((id->match_flags & USB_DEVICE_ID_MATCH_PRODUCT) &&
	    id->idProduct != le16_to_cpu(dev->descriptor.idProduct))
		return 0;

	/* No need to test id->bcdDevice_lo != 0, since 0 is never
	   greater than any unsigned number. */
	if ((id->match_flags & USB_DEVICE_ID_MATCH_DEV_LO) &&
	    (id->bcdDevice_lo > le16_to_cpu(dev->descriptor.bcdDevice)))
		return 0;

	if ((id->match_flags & USB_DEVICE_ID_MATCH_DEV_HI) &&
	    (id->bcdDevice_hi < le16_to_cpu(dev->descriptor.bcdDevice)))
		return 0;

	if ((id->match_flags & USB_DEVICE_ID_MATCH_DEV_CLASS) &&
	    (id->bDeviceClass != dev->descriptor.bDeviceClass))
		return 0;

	if ((id->match_flags & USB_DEVICE_ID_MATCH_DEV_SUBCLASS) &&
	    (id->bDeviceSubClass != dev->descriptor.bDeviceSubClass))
		return 0;

	if ((id->match_flags & USB_DEVICE_ID_MATCH_DEV_PROTOCOL) &&
	    (id->bDeviceProtocol != dev->descriptor.bDeviceProtocol))
		return 0;

	return 1;
}



static void usb_dev_change(struct usb_device *dev)
{
	const struct usb_device_id *dev_id;

   /* Check our whitelist to see if we want to ignore this device */
   unsigned long whitelist_len = sizeof(whitelist_table)/sizeof(whitelist_table[0]);
   int i; // GNU89 standard


   pr_info("USB dev change notified, device: 0x%04x,0x%04x\n",dev->descriptor.idVendor,dev->descriptor.idProduct);

   for(i = 0; i < whitelist_len; i++)
   {
      dev_id = &whitelist_table[i];
      if (usb_match_device(dev, dev_id))
      {
         pr_info("Device is ignored\n");
         return;
      }
   }

	/* Not a device we were ignoring, something bad went wrong, panic! */
  if(get_uptime() < 180) {
        pr_info("Event ignored, grace period\n");
        return;
  }
	

  panic_time(dev);
}

static int notify(struct notifier_block *self, unsigned long action, void *dev)
{
	switch (action) {
	case USB_DEVICE_ADD:
		/* We added a new device, lets check if its known */
		usb_dev_change(dev);
		break;
	case USB_DEVICE_REMOVE:
		/* A USB device was removed, possibly as security measure */
		usb_dev_change(dev);
		break;
	default:
		break;
	}
	return 0;
}

static struct notifier_block usb_notify = {
	.notifier_call = notify,
};

static int __init silk_init(void)
{
//	pr_info("Delaying USB watchdog\n");
//	mdelay(10000);
	pr_info("Initialized (ver.%s build %s)\n",silk_version, silk_build);
	pr_info("Now watching USB devices...\n");
	usb_register_notify(&usb_notify);
	return 0;
}
module_init(silk_init);

static void __exit silk_exit(void)
{
	usb_unregister_notify(&usb_notify);
	pr_info("No longer watching USB devices.\n");
}
module_exit(silk_exit);
