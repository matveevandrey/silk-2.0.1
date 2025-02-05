//#define silk_version {os.environ['PACKAGE_VERSION']}
#define silk_version "2.0.1"
#define silk_build "1002"

/* Files silk-guardian will remove upon detecting change in usb state. */
static char *remove_files[] = {
	//"/home/user/privatekey",
	//"/private/ssnumber.pdf",
	NULL,	/* Must be NULL terminated */
};


/* Userspace poweroff routine */
static  char *shutdown_argv[] = { "/sbin/poweroff", "-p", "-f", NULL, };

/* How many times to shred file. The more iterations the longer it takes. */
static char *shredIterations = "3";

/* List of all USB devices you want whitelisted (i.e. ignored) */
static const struct usb_device_id whitelist_table[] = {
//	{ USB_DEVICE(0x0000, 0x0000) },
    { USB_DEVICE(0x046b, 0xff10) },
    { USB_DEVICE(0x046b, 0xff31) },
    { USB_DEVICE(0x046b, 0xff40) },
    { USB_DEVICE(0x046b, 0xff20) },
    { USB_DEVICE(0x046b, 0xff01) },
};
