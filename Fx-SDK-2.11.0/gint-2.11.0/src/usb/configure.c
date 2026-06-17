#include <gint/usb.h>
#include "usb_private.h"
#include <stdlib.h>
#include <string.h>

//---
// Endpoint assignment
//---

/* Current configuration: list of interfaces and endpoint assignments */
#define MAX_INTERFACES 16 /* actually 15 */
static usb_interface_t const *conf_if[MAX_INTERFACES];
static endpoint_t *conf_ep;

GCONSTRUCTOR static void usb_alloc(void)
{
	 conf_ep = malloc(32 * sizeof *conf_ep);
}

endpoint_t *usb_get_endpoint_by_global_address(int global_address)
{
	global_address = (uint8_t)global_address;

	/* Accept only valid non-DCP endpoints */
	if((global_address & 0x70) || !(global_address & 0x0f))
		return NULL;

	int dir = (global_address & 0x80) ? 0x10 : 0;
	return &conf_ep[dir + (global_address & 0x0f)];
}

endpoint_t *usb_get_endpoint_by_local_address(usb_interface_t const *intf,
   int local_address)
{
	for(int i = 0; i < 32; i++)
	{
		endpoint_t *ep = &conf_ep[i];
		if(ep->intf == intf && ep->dc->bEndpointAddress == local_address)
			return ep;
	}
	return NULL;
}

endpoint_t *usb_get_endpoint_by_pipe(int pipe)
{
	for(int i = 0; i < 32; i++) {
		if(conf_ep[i].pipe == pipe)
			return &conf_ep[i];
	}
	return NULL;
}

//---
// Resource allocation
//---

/* is_pipe_used(): Determine whether a pipe is used by an endpoint */
static bool is_pipe_used(int pipe)
{
	for(int i = 0; i < 32; i++)
	{
		if(conf_ep[i].pipe == pipe) return true;
	}
	return false;
}

/* find_pipe(): Find an unused pipe for the specified type of transfer */
static int find_pipe(int type)
{
	int min=0, max=-1;

	/* Isochronous transfers: use pipes 1,2 */
	if(type == 1) min=1, max=2;
	/* Bulk transfers: try 3..5 first, leaving 1,2 for isochronous */
	if(type == 2) min=1, max=5;
	/* Interrupt transfers: use pipes 6..9 */
	if(type == 3) min=6, max=9;

	/* Start from the end to avoid using pipes 1,2 on bulk transfers */
	for(int pipe = max; pipe >= min; pipe--)
	{
		if(!is_pipe_used(pipe)) return pipe;
	}
	return -1;
}

int usb_configure_solve(usb_interface_t const **interfaces)
{
	/* Reset the previous configuration */
	memset(conf_if, 0, MAX_INTERFACES * sizeof *conf_if);
	memset(conf_ep, 0, 32 * sizeof *conf_ep);

	/* Next interface number to assign */
	int next_interface = 0;
	/* Next endpoint to assign */
	int next_endpoint = 1;
	/* Next buffer position to assign for pipes 1..5 */
	int next_bufnmb = 8;

	for(int i = 0; interfaces[i]; i++)
	{
		if(i == MAX_INTERFACES - 1)
			return USB_OPEN_TOO_MANY_INTERFACES;
		usb_interface_t const *intf = interfaces[i];

		conf_if[next_interface++] = intf;

		for(int k = 0; intf->dc[k]; k++)
		{
			uint8_t const *dc = intf->dc[k];
			if(dc[1] != USB_DC_ENDPOINT) continue;

			/* If the endpoint number has already been assigned in the other
			   direction, keep the same global endpoint number. */
			endpoint_t *dual_ep = usb_get_endpoint_by_local_address(intf,
				dc[2] ^ 0x80);

			int address;
			if(dual_ep)
				address = dual_ep->global_address ^ 0x80;
			else if(next_endpoint >= 16)
				return USB_OPEN_TOO_MANY_ENDPOINTS;
			else
				address = (next_endpoint++) | (dc[2] & 0x80);

			int pipe = find_pipe(dc[3] & 0x03);
			if(pipe < 0) return USB_OPEN_TOO_MANY_ENDPOINTS;

			endpoint_t *ep = usb_get_endpoint_by_global_address(address);
			ep->intf = intf;
			ep->dc = (void *)dc;
			ep->global_address = address;
			ep->pipe = pipe;
			ep->bufnmb = 0;
			ep->bufsize = 0;

			/* Fixed areas */
			if(pipe >= 6)
			{
				ep->bufnmb = (pipe - 2);
				ep->bufsize = 1;
			}
		}

		for(int k = 0; intf->params[k].endpoint; k++)
		{
			usb_interface_endpoint_t *params = &intf->params[k];

			endpoint_t *ep = usb_get_endpoint_by_local_address(intf,
				params->endpoint);
			if(!ep) return USB_OPEN_INVALID_PARAMS;

			uint bufsize = params->buffer_size >> 6;
			if(params->buffer_size & 63
			   || bufsize <= 0
			   || bufsize > 0x20
			   || (ep->pipe >= 6 && bufsize != 1))
				return USB_OPEN_INVALID_PARAMS;
			if(ep->pipe >= 6) continue;

			if(next_bufnmb + bufsize > 0x100)
				return USB_OPEN_NOT_ENOUGH_MEMORY;

			ep->bufnmb = next_bufnmb;
			ep->bufsize = bufsize;
			next_bufnmb += bufsize;
		}
	}

	/* Check that all endpoints have memory assigned */
	for(int i = 0; i < 32; i++)
	{
		if(!conf_ep[i].intf) continue;
		if(conf_ep[i].bufsize == 0)
			return USB_OPEN_MISSING_DATA;
	}

	return 0;
}

void usb_configure_log(void)
{
#ifdef GINT_USB_DEBUG
	/* Log the final configuration */
	for(int i = 0; conf_if[i]; i++)
		USB_LOG("Interface #%d: %p\n", i, conf_if[i]);

	for(int i = 0; i < 32; i++)
	{
		if(!conf_ep[i].intf) continue;

		endpoint_t *ep = &conf_ep[i];
		USB_LOG("Endpoint %02x\n",
			(i & 15) + (i >= 16 ? 0x80 : 0));
		USB_LOG("  Interface %p address %02x\n",
			ep->intf, ep->dc->bEndpointAddress);
		USB_LOG("  Pipe %d (FIFO: %02x..%02x)\n",
			ep->pipe, ep->bufnmb, ep->bufnmb + ep->bufsize);
	}
#endif
}

void usb_configure(void)
{
	for(int i = 0; i < 32; i++)
	{
		if(!conf_ep[i].intf) continue;
		int address = (i & 0xf) + (i >= 16 ? 0x80 : 0);
		usb_pipe_configure(address, &conf_ep[i]);
	}
}

usb_interface_t const * const *usb_configure_interfaces(void)
{
	return conf_if;
}

//---
// API for interfaces
//---

int usb_interface_pipe(usb_interface_t const *intf, int address)
{
	endpoint_t const *ep = usb_get_endpoint_by_local_address(intf, address);
	return ep ? ep->pipe : -1;
}
