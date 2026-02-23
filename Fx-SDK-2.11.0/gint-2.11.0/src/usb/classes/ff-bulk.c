#include <gint/usb.h>
#include <gint/usb-ff-bulk.h>
#include <gint/display.h>
#include <gint/hardware.h>
#include <gint/cpu.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

static void notify_read(int endpoint);

static usb_dc_interface_t dc_interface = {
	.bLength              = sizeof(usb_dc_interface_t),
	.bDescriptorType      = USB_DC_INTERFACE,
	.bInterfaceNumber     = -1, /* Set by driver */
	.bAlternateSetting    = 0,
	.bNumEndpoints        = 2,
	.bInterfaceClass      = 0xff, /* Vendor-Specific */
	.bInterfaceSubClass   = 0x77, /* (not recognized by Casio tools?) */
	.bInterfaceProtocol   = 0x00,
	.iInterface           = 0,
};

/* Endpoint for calculator -> PC communication */
static usb_dc_endpoint_t dc_endpoint1i = {
	.bLength             = sizeof(usb_dc_endpoint_t),
	.bDescriptorType     = USB_DC_ENDPOINT,
	.bEndpointAddress    = 0x81, /* 1 IN */
	.bmAttributes        = 0x02, /* Bulk transfer */
	.wMaxPacketSize      = htole16(512),
	.bInterval           = 1,
};
/* Endpoint for PC -> calculator communication */
static usb_dc_endpoint_t dc_endpoint1o = {
	.bLength             = sizeof(usb_dc_endpoint_t),
	.bDescriptorType     = USB_DC_ENDPOINT,
	.bEndpointAddress    = 0x02, /* 2 OUT */
	.bmAttributes        = 0x02, /* Bulk transfer */
	.wMaxPacketSize      = htole16(512),
	.bInterval           = 1,
};

usb_interface_t const usb_ff_bulk = {
	/* List of descriptors */
	.dc = (void const *[]){
		&dc_interface,
		&dc_endpoint1i,
		&dc_endpoint1o,
		NULL,
	},
	/* Parameters for each endpoint */
	.params = (usb_interface_endpoint_t []){
		{ .endpoint     = 0x81, /* 1 IN */
		  .buffer_size  = 2048, },
		{ .endpoint     = 0x02, /* 2 OUT */
		  .buffer_size  = 2048, },
		{ 0 },
	},
	.notify_read = notify_read,
};

GCONSTRUCTOR static void set_strings(void)
{
	dc_interface.iInterface = usb_dc_string(u"Bulk Input", 0);
}

//---
// Direct bulk access
//---

int usb_ff_bulk_output(void)
{
	return usb_interface_pipe(&usb_ff_bulk, 0x81);
}

int usb_ff_bulk_input(void)
{
	return usb_interface_pipe(&usb_ff_bulk, 0x02);
}

//---
// fxlink protocol
//---

bool usb_fxlink_fill_header(usb_fxlink_header_t *header,
	char const *application, char const *type, uint32_t data_size)
{
	if(strlen(application) > 16 || strlen(type) > 16) return false;

	memset(header, 0, sizeof *header);
	header->version = htole32(0x00000100);
	header->size = htole32(data_size);
	/* TODO: usb_fxlink_fill_header: avoid harcoded transfer size */
	header->transfer_size = htole32(2048);

	strncpy(header->application, application, 16);
	strncpy(header->type, type, 16);

	return true;
}

static void capture_vram(GUNUSED bool onscreen, char const *type)
{
	void *source = gint_vram;
	int size, format;

	#if GINT_RENDER_MONO
	size = 1024;
	format = USB_FXLINK_IMAGE_MONO;
	#endif

	#if GINT_RENDER_RGB
	if(onscreen) {
		uint16_t *main, *secondary;
		dgetvram(&main, &secondary);
		source = (gint_vram == main) ? secondary : main;
	}
	size = DWIDTH * DHEIGHT * 2;
	format = USB_FXLINK_IMAGE_RGB565;
	#endif

	usb_fxlink_header_t header;
	usb_fxlink_image_t subheader;

	usb_fxlink_fill_header(&header, "fxlink", type,
		size + sizeof subheader);

	subheader.width = htole32(DWIDTH);
	subheader.height = htole32(DHEIGHT);
	subheader.pixel_format = htole32(format);

	int pipe = usb_ff_bulk_output();
	usb_write_sync(pipe, &header, sizeof header, false);
	usb_write_sync(pipe, &subheader, sizeof subheader, false);
	usb_write_sync(pipe, source, size, false);
	usb_commit_sync(pipe);
}

void usb_fxlink_screenshot(bool onscreen)
{
	capture_vram(onscreen, "image");
}

void usb_fxlink_text(char const *text, int size)
{
	if(size == 0) size = strlen(text);

	usb_fxlink_header_t header;
	usb_fxlink_fill_header(&header, "fxlink", "text", size);

	int pipe = usb_ff_bulk_output();
	usb_write_sync(pipe, &header, sizeof header, false);
	usb_write_sync(pipe, text, size, false);
	usb_commit_sync(pipe);
}

void usb_fxlink_videocapture(bool onscreen)
{
	capture_vram(onscreen, "video");
}

//---
// Built-in command execution
//---

static char const * const str_MPU[] = {
	[HWMPU_UNKNOWN] = "Unknown MPU",
	[HWMPU_SH7337]  = "SH7737",
	[HWMPU_SH7305]  = "SH7305",
	[HWMPU_SH7355]  = "SH7355",
	[HWMPU_SH7724]  = "SH7724",
};
static char const * const str_CALC[] = {
	[HWCALC_FX9860G_SH3]   = "SH3-based fx-9860G-like",
	[HWCALC_FX9860G_SH4]   = "SH4-based fx-9860G-like",
	[HWCALC_G35PE2]        = "fx-9860G III/Graph 35+E II",
	[HWCALC_PRIZM]         = "Prizm fx-CG 10/20",
	[HWCALC_FXCG50]        = "fx-CG 50/Graph 90+E",
	[HWCALC_FXCG_MANAGER]  = "fx-CG Manager",
	[HWCALC_FX9860G_SLIM]  = "fx-9860G Slim",
	[HWCALC_FXCP400]       = "fx-CP 400",
};

static void execute_command(char const *cmd)
{
	if(!strncmp(cmd, "echo", 4)) {
		char const *text = cmd+4 + strspn(cmd+4, " \t\n");
		usb_fxlink_text(text, 0);
	}
	if(!strncmp(cmd, "identify", 8)) {
#if GINT_OS_FX
		char const *serial_number = (void *)0x8000ffd0;
		char const *OS_version = (void *)0x80010020;
		char const *BC_version = (void *)0x8000ffb0;
#elif GINT_OS_CG || GINT_OS_CP
		char const *serial_number = (void *)0x8001ffd0;
		char const *OS_version = (void *)0x80020020;
		char const *BC_version = (void *)0x8001ffb0;
#endif

		char str[256];
		sprintf(str, "gint %.8s %s (0x%07x) on %s (%s) %.10s %.14s\n",
			serial_number,
			GINT_VERSION, GINT_HASH,
			str_MPU[gint[HWMPU]], str_CALC[gint[HWCALC]],
			OS_version, BC_version);
		usb_fxlink_text(str, 0);
	}
}

//---
// Data reception
//---

/* User notification function */
static void (*recv_handler)(void) = NULL;

bool usb_fxlink_handle_messages(usb_fxlink_header_t *header_ptr)
{
	if(!usb_is_open())
		return false;

	/* Read a message header first */
	usb_fxlink_header_t header;
	timeout_t tm = timeout_make_ms(1000);

	int rc = usb_read_async(usb_ff_bulk_input(), &header, sizeof header,
		USB_READ_IGNORE_ZEROS | USB_READ_AUTOCLOSE | USB_READ_WAIT, NULL,
		&tm, GINT_CALL_NULL);
	if(rc == USB_READ_IDLE)
		return false;

	if(rc < 0) {
		USB_LOG("[ff-bulk] Header read failed: %d\n", rc);
		return false;
	}
	if(rc < (int)sizeof header) {
		USB_LOG("[ff-bulk] Incomplete header (%d/%d bytes)\n", rc,
			sizeof header);
		return false;
	}

	header.version = le32toh(header.version);
	header.size = le32toh(header.size);
	header.transfer_size = le32toh(header.transfer_size);

	int major = (header.version >> 8) & 0xff;
	int minor = (header.version & 0xff);
	(void)minor;
	(void)major;

	if(major != 1 || minor != 0) {
		USB_LOG("[ff-bulk] Invalid message header!\n");
		return usb_fxlink_handle_messages(header_ptr);
	}

	USB_LOG("[ff-bulk %d.%d] New %.16s.%.16s (%d bytes)\n", major, minor,
		header.application, header.type, header.size);

	/* Handle built-in fxlink messages */
	if(!strncmp(header.application, "fxlink", 16)
			&& !strncmp(header.type, "command", 16)) {
		USB_LOG("[ff-bulk] Receiving command (%d bytes)\n", header.size);
		char *cmd = malloc(header.size + 1);
		if(!cmd)
			return false;
		usb_read_sync(usb_ff_bulk_input(), cmd, header.size, false);
		cmd[header.size] = 0;
		USB_LOG("[ff-bulk] Command is: '%s'\n", cmd);
		execute_command(cmd);
		free(cmd);
		return usb_fxlink_handle_messages(header_ptr);
	}

	*header_ptr = header;
	return true;
}

void usb_fxlink_set_notifier(void (*notifier_function)(void))
{
	recv_handler = notifier_function;
}

void usb_fxlink_drop_transaction(void)
{
	int block = USB_READ_BLOCK;
	char buf[512];

	while(1) {
		timeout_t tm = timeout_make_ms(1000);
		int rc = usb_read_async(usb_ff_bulk_input(), buf, 512,
			USB_READ_WAIT | block, NULL, &tm, GINT_CALL_NULL);

		/* Break on error or short read (end of transaction) */
		if(rc != 512)
			break;
		block = 0;
	}
}

static void notify_read(int endpoint)
{
	/* We only have one endpoint for reading, the bulk OUT */
	(void)endpoint;

//	USB_LOG("[ff-bulk] Data available on %02x\n", endpoint);

	if(recv_handler)
		recv_handler();
}
