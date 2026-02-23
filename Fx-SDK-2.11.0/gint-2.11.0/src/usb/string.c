//---
// gint:usb:string - STRING descriptor management
//---

#include <gint/usb.h>

#include <stdlib.h>
#include <string.h>

/* String descriptor array */
static usb_dc_string_t **array = NULL;
static int array_size = 0;

uint16_t usb_dc_string(uint16_t const *literal, size_t len)
{
	if(array_size == 255) return 0;

	/* Determine the length of the string */
	if(len == 0)
	{
		while(literal[len]) len++;
	}
	if(2*len + 2 >= 256) return 0;

	/* Allocate a new descriptor */
	usb_dc_string_t *dc = malloc(sizeof *dc + 2*len);
	if(!dc) return 0;

	dc->bLength = 2*len + 2;
	dc->bDescriptorType = USB_DC_STRING;
	for(size_t i = 0; i < len; i++) dc->data[i] = htole16(literal[i]);

	/* Try to make room in the array */
	size_t new_size = (array_size + 1) * sizeof(*array);
	usb_dc_string_t **new_array = realloc(array, new_size);

	if(!new_array)
	{
		free(dc);
		return 0;
	}

	array = new_array;
	array[array_size++] = dc;
	/* IDs are numbered 1 to 255 */
	return (array_size - 1) + 1;
}

usb_dc_string_t *usb_dc_string_get(uint16_t id)
{
	if((int)id < 1 || id - 1 >= array_size) return NULL;
	return array[id - 1];
}
