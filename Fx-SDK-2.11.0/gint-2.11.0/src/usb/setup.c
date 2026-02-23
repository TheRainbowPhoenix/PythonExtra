#include <gint/usb.h>
#include <gint/mpu/usb.h>
#include <gint/config.h>

#include <stdarg.h>
#include <endian.h>

#include "usb_private.h"

#define USB SH7305_USB

#define dcp_write(data, size) usb_write_sync(0, data, size, false)

//---
// SETUP requests
//---

/* 0x6101: fx-9860G II, Protocol 7.00, etc.
   0x6102: fx-CP 400, fx-CG 50, Mass Storage, etc. */
#define ID_PRODUCT GINT_HW_SWITCH(0x6101, 0x6102, 0x6102)
#define DESC_PRODUCT GINT_HW_SWITCH( \
    u"CASIO fx-9860G family on gint", \
    u"CASIO fx-CG family on gint", \
    u"CASIO fx-CP family on gint")

static usb_dc_device_t dc_device = {
	.bLength              = sizeof(usb_dc_device_t),
	.bDescriptorType      = USB_DC_DEVICE,
	.bcdUSB               = htole16(0x0200), /* USB 2.00 */
	.bDeviceClass         = 0, /* Configuration-specific */
	.bDeviceSubClass      = 0,
	.bDeviceProtocol      = 0,
	.bMaxPacketSize0      = 64,
	.idVendor             = htole16(0x07cf), /* Casio Computer Co., Ltd. */
	.idProduct            = htole16(ID_PRODUCT),
	.bcdDevice            = htole16(0x0100),
	.iManufacturer        = 0,
	.iProduct             = 0,
	.iSerialNumber        = 0,
	.bNumConfigurations   = 1,
};
static usb_dc_configuration_t dc_configuration = {
	.bLength              = sizeof(usb_dc_configuration_t),
	.bDescriptorType      = USB_DC_CONFIGURATION,
	.wTotalLength         = 0,
	.bNumInterfaces       = 1,
	.bConfigurationValue  = 1,
	.iConfiguration       = 0,
	.bmAttributes         = 0xc0,
	.bMaxPower            = 50,
};
static usb_dc_string_t dc_string0 = {
	.bLength              = 4,
	.bDescriptorType      = USB_DC_STRING,
	.data                 = { htole16(0x0409) }, /* English (US) */
};

GCONSTRUCTOR static void set_strings(void)
{
	char const *serial_base =
		(void *)GINT_OS_SWITCH(0x8000ffd0, 0x8001ffd0, 0x8001ffd0);

	/* Convert the serial number to UTF-16 */
	uint16_t serial[8];
	for(int i = 0; i < 8; i++) serial[i] = serial_base[i];

	dc_device.iManufacturer = usb_dc_string(u"CASIO Computer Co., Ltd", 0);
	dc_device.iProduct      = usb_dc_string(DESC_PRODUCT, 0);
	dc_device.iSerialNumber = usb_dc_string(serial, 8);
}

//---
// Configuration descriptor generation
//---

static void write_configuration_descriptor(int wLength)
{
	usb_interface_t const * const *interfaces = usb_configure_interfaces();
	size_t total_length = sizeof(usb_dc_configuration_t);

	for(int i = 0; interfaces[i]; i++)
	for(int k = 0; interfaces[i]->dc[k]; k++)
	{
		uint8_t const *dc = interfaces[i]->dc[k];
		total_length += dc[0];
	}
	USB_LOG("Configuration descriptor size: %d\n", (int)total_length);

	/* Write the configuration descriptor */
	dc_configuration.wTotalLength = htole16(total_length);
	dcp_write(&dc_configuration, dc_configuration.bLength);
	/* For the first call, the host usually wants only this */
	if(wLength <= dc_configuration.bLength) return;

	/* Write all the other descriptors */
	for(int i = 0; interfaces[i]; i++)
	for(int k = 0; interfaces[i]->dc[k]; k++)
	{
		uint8_t const *dc = interfaces[i]->dc[k];

		/* Edit interface numbers on-the-fly */
		if(dc[1] == USB_DC_INTERFACE)
		{
			usb_dc_interface_t idc = *(usb_dc_interface_t *)dc;
			idc.bInterfaceNumber = i;
			dcp_write(&idc, idc.bLength);
		}
		/* Edit endpoint numbers on-the-fly */
		else if(dc[1] == USB_DC_ENDPOINT)
		{
			usb_dc_endpoint_t edc = *(usb_dc_endpoint_t *)dc;
			endpoint_t *e = usb_get_endpoint_by_local_address(interfaces[i],
				edc.bEndpointAddress);
			edc.bEndpointAddress = e->global_address;
			dcp_write(&edc, edc.bLength);
		}
		/* Forward other descriptors */
		else dcp_write(dc, dc[0]);
	}
}

static void req_get_descriptor(int wValue, int wLength)
{
	int type = (wValue >> 8) & 0xff;
	int num = (wValue & 0xff);

	GUNUSED static char const *strs[] = {
		"DEV","CONFIG","STR","INTF","ENDP","DEVQ","OSC","POWER" };
	USB_LOG("GET_DESCRIPTOR: %s #%d len:%d\n", strs[type-1], num, wLength);

	if(type == USB_DC_DEVICE && num == 0)
		dcp_write(&dc_device, dc_device.bLength);

	else if(type == USB_DC_CONFIGURATION && num == 0)
		write_configuration_descriptor(wLength);

	else if(type == USB_DC_STRING && num == 0)
		dcp_write(&dc_string0, dc_string0.bLength);

	else if(type == USB_DC_STRING)
	{
		usb_dc_string_t *dc = usb_dc_string_get(num);
		if(dc) dcp_write(dc, dc->bLength);
		else USB.DCPCTR.PID = 2;
	}
}

static void req_get_configuration(void)
{
	USB_LOG("GET_CONFIGURATION -> %d\n", 1);
	dcp_write("\x01", 1);
}

static void req_set_configuration(int wValue)
{
	USB_LOG("SET_CONFIGURATION: %d\n", wValue);
	/* Ok for (wValue == 1) only */
	USB.DCPCTR.PID = (wValue == 1) ? 1 : 2;
}

void usb_req_setup(void)
{
	/* Respond to setup requests */
	int bRequest = USB.USBREQ.BREQUEST;
	int bmRequestType = USB.USBREQ.BMREQUEST;
	int wValue = USB.USBVAL.word;
	GUNUSED int wIndex = USB.USBINDX.word;
	int wLength = USB.USBLENG.word;

	USB.INTSTS0.VALID = 0;
	usb_while(USB.INTSTS0.VALID);

	/* Standard requests */

	if(bmRequestType == 0x80 && bRequest == GET_DESCRIPTOR)
		req_get_descriptor(wValue, wLength);

	else if(bmRequestType == 0x80 && bRequest == GET_CONFIGURATION)
		req_get_configuration();

	else if(bmRequestType == 0x00 && bRequest == SET_CONFIGURATION)
		req_set_configuration(wValue);

	/* TODO: Other standard SETUP requests */
	else USB_LOG("SETUP: bRequest=%02x bmRequestType=%02x wValue=%04x\n"
		"  wIndex=%04x wLength=%d -> ???\n",
		bRequest, bmRequestType, wValue, wIndex, wLength);

	/* Push the buffer when responding to an IN request with a BUF */
	if((bmRequestType & 0x80) && USB.DCPCTR.PID == 1)
		usb_commit_sync(0);

	/* Finalize request */
	USB.DCPCTR.CCPL = 1;
}
