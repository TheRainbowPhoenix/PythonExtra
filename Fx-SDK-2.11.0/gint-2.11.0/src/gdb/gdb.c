#include <gint/cpu.h>
#include <gint/exc.h>
#include <gint/gdb.h>
#include <gint/ubc.h>
#include <gint/usb-ff-bulk.h>
#include <gint/usb.h>
#include <gint/video.h>
#include <gint/display.h>
#include <gint/config.h>
#include <gint/hardware.h>
#include <gint/fs.h>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define GDB_VISUAL_FEEDBACK 1
#define GDB_BRIDGE_LOGS 0

/* Note about trap numbers:
   - trapa #16...#23 were historically used for Linux's syscall
     interface for syscalls with up to 0...7 arguments.
   - trapa #31 is unified syscall interface (Linux)
   - trapa #32 is GDB software breakpoint
   - trapa #33 shall thus be our stubcall. */
#define TRA_SWBREAK 32
#define TRA_STUBCALL 33

#if GDB_VISUAL_FEEDBACK

enum { ICON_WORKING, ICON_ERROR, ICON_COMM, ICON_IDLE };

static void gdb_show_stub_status(int icon)
{
// TODO[3]: Use normal way for both fx and cg (and remove display.h include)
#if GINT_RENDER_MONO
	extern bopti_image_t gint_gdb_icons_i1msb;
	dsubimage(120, 0, &gint_gdb_icons_i1msb, 8*icon, 0, 8, 5, DIMAGE_NONE);
	dupdate();
#else
	video_mode_t const *M = video_get_current_mode();
	if(!M)
		return;

	extern image_t gint_gdb_icons_rgb565;

	if(M->format == IMAGE_RGB565) {
		image_t sub;
		image_sub(&gint_gdb_icons_rgb565, 6*icon, 0, 7, 7, &sub);
		if(!video_update(M->width-7, 0, &sub, 0))
			abort();
	}
#endif
}

#else
# define gdb_show_stub_status(...) ((void)0)
#endif

static void gdb_hexlify(char* output_string, const uint8_t* input_buffer, size_t input_size)
{
	const char* hex = "0123456789ABCDEF";
	for (size_t i = 0; i < input_size; i++) {
		uint8_t byte = input_buffer[i];
		output_string[i*2 + 0] = hex[(byte & 0xF0) >> 4];
		output_string[i*2 + 1] = hex[byte & 0x0F];
	}
}

// TODO : bug in fxlibc ? strtoul doesn't support uppercase
static uint32_t gdb_unhexlify_sized(const char* input_string, size_t input_length)
{
	uint32_t ret = 0;
	for (size_t i = 0; i < input_length; i++) {
		uint8_t nibble_hex = tolower(input_string[i]);
		uint8_t nibble = nibble_hex >= 'a' && nibble_hex <= 'f' ? nibble_hex - 'a' + 10 :
				 nibble_hex >= '0' && nibble_hex <= '9' ? nibble_hex - '0' : 0;
		ret = (ret << 4) | nibble;
	}
	return ret;
}

static uint32_t gdb_unhexlify(const char* input_string)
{
	return gdb_unhexlify_sized(input_string, strlen(input_string));
}

static bool gdb_started = false;

static void gdb_send(const char *data, size_t size)
{
	usb_fxlink_header_t header;
	usb_fxlink_fill_header(&header, "gdb", "remote", size);

	int pipe = usb_ff_bulk_output();
	usb_write_sync(pipe, &header, sizeof(header), false);
	usb_write_sync(pipe, data, size, false);
	usb_commit_sync(pipe);
}

static void gdb_send_start(void)
{
	usb_fxlink_header_t header;
	usb_fxlink_fill_header(&header, "gdb", "start", 0);

	int pipe = usb_ff_bulk_output();
	usb_write_sync(pipe, &header, sizeof(header), false);
	usb_commit_sync(pipe);
}

static char *gdb_recv_buffer = NULL;
static const size_t gdb_recv_buffer_capacity = 256;
static size_t gdb_recv_buffer_size = 0;
static ssize_t gdb_recv(char *buffer, size_t buffer_size)
{
	if (gdb_recv_buffer_size >= buffer_size) {
		memcpy(buffer, gdb_recv_buffer, buffer_size);
		memmove(gdb_recv_buffer, &gdb_recv_buffer[buffer_size], gdb_recv_buffer_size - buffer_size);
		gdb_recv_buffer_size -= buffer_size;
		return buffer_size;
	}

	usb_fxlink_header_t header;
	while (!usb_fxlink_handle_messages(&header)) {
		sleep();
	}

	// TODO : should we abort or find a way to gracefully shutdown the debugger ?
	if (strncmp(header.application, "gdb", 16) == 0
	     && strncmp(header.type, "remote", 16) == 0) {
		if (header.size > gdb_recv_buffer_capacity - gdb_recv_buffer_size) {
			abort();
		}
		usb_read_sync(usb_ff_bulk_input(), &gdb_recv_buffer[gdb_recv_buffer_size], header.size, false);
		gdb_recv_buffer_size += header.size;
		return gdb_recv(buffer, buffer_size);
	} else {
		abort();
	}
}

static ssize_t gdb_recv_packet(char* buffer, size_t buffer_size)
{
	char read_char;

	// Waiting for packet start '$'
	do {
		if (gdb_recv(&read_char, 1) != 1) {
			return -1;
		}
	} while (read_char != '$');

	uint8_t checksum = 0;
	size_t packet_len = 0;
	while (true) {
		if (gdb_recv(&read_char, 1) != 1) {
			return -1;
		}

		if (read_char != '#') {
			// -1 to ensure space for a NULL terminator
			if (packet_len >= (buffer_size - 1)) {
				return -1;
			}
			buffer[packet_len++] = read_char;
			checksum += read_char;
		} else {
			break;
		}
	}
	buffer[packet_len] = '\0';

	char read_checksum_hex[3];
	if (gdb_recv(read_checksum_hex, 2) != 2) {
		return -1;
	}
	read_checksum_hex[2] = '\0';
	uint8_t read_checksum = gdb_unhexlify(read_checksum_hex);

	if (read_checksum != checksum) {
		read_char = '-';
		gdb_send(&read_char, 1);
		return -1;
	} else {
		read_char = '+';
		gdb_send(&read_char, 1);
		return packet_len;
	}
}

static ssize_t gdb_send_packet(const char* packet, size_t packet_length)
{
	if (packet == NULL || packet_length == 0) {
		// Empty packet
		gdb_send("$#00", 4);
		return 4;
	}

	size_t buffer_length = packet_length + 1 + 4;
	// TODO : find if it's more efficient to malloc+copy on each packet or send 3 small fxlink messages
	char* buffer = malloc(buffer_length);

	uint8_t checksum = 0;
	for (size_t i = 0; i < packet_length; i++) {
		checksum += packet[i];
	}

	buffer[0] = '$';
	memcpy(&buffer[1], packet, packet_length);
	snprintf(&buffer[buffer_length - 4], 4, "#%02X", checksum);

	// -1 to not send the NULL terminator of snprintf
	gdb_send(buffer, buffer_length - 1);
	free(buffer);
	return buffer_length;
}

#if GDB_BRIDGE_LOGS
static void gdb_send_bridge_log(const char* fmt, ...)
{
	char str[256];
	va_list args;
	va_start(args, fmt);
	vsnprintf(str, sizeof str, fmt, args);
	va_end(args);
	usb_fxlink_text(str, strlen(str));
}
#else
# define gdb_send_bridge_log(...)
#endif

static int gdb_signal_number = 0;
static int gdb_trap_number = 0;

static void gdb_send_stop_reply(void)
{
	char str[4] = "S00";
	uint8_t num = gdb_signal_number ? gdb_signal_number : 5 /* SIGTRAP */;
	gdb_hexlify(str+1, &num, 1);
	gdb_send_packet(str, 3);
}

static void gdb_handle_qXfer_packet(const char* packet, const char* data, size_t data_size)
{
	char offset_hex[16] = {0}, length_hex[16] = {0};
	for (size_t i = 0; i < sizeof(offset_hex); i++) {
		offset_hex[i] = *(packet++); // consume offset
		if (*packet == ',') break;
	}
	packet++; // consume ','
	for (size_t i = 0; i < sizeof(length_hex); i++) {
		length_hex[i] = *(packet++); // consume length
		if (*packet == '\0') break;
	}

	size_t offset = (size_t)gdb_unhexlify(offset_hex);
	size_t length = (size_t)gdb_unhexlify(length_hex);

	if (offset >= data_size) {
		gdb_send_packet("l", 1);
	} else if (offset + length >= data_size) {
		char *reply_buffer = malloc(data_size - offset + 1);
		reply_buffer[0] = 'l';
		memcpy(&reply_buffer[1], &data[offset], data_size - offset);
		gdb_send_packet(reply_buffer, data_size - offset + 1);
		free(reply_buffer);
	} else {
		char *reply_buffer = malloc(length + 1);
		reply_buffer[0] = 'm';
		memcpy(&reply_buffer[1], &data[offset], length);
		gdb_send_packet(reply_buffer, length + 1);
		free(reply_buffer);
	}
}

/* We implement the memory-map qXfer extension to mark add-in memory as read-only
 * and enforce hardware breakpoints.
 * See : https://sourceware.org/gdb/onlinedocs/gdb/Memory-Map-Format.html
 *       https://sourceware.org/gdb/onlinedocs/gdb/Set-Breaks.html
 */
// TODO : Should we mark other regions as ROM ?
static const char gdb_memory_map_xml[] = "<?xml version=\"1.0\"?>"
"<!DOCTYPE memory-map PUBLIC \"+//IDN gnu.org//DTD GDB Memory Map V1.0//EN\" \"http://sourceware.org/gdb/gdb-memory-map.dtd\">"
"<memory-map>"
	// P0 mapping of add-in file
	"<memory type=\"rom\" start=\"0x00300000\" length=\"0x00200000\"/>"
	// P0 mapping of user RAM area
	"<memory type=\"ram\" start=\"0x08100000\" length=\"0x00080000\"/>"
	// Physical mapping of RAM chip (fx-CG 50)
	"<memory type=\"ram\" start=\"0x8c000000\" length=\"0x01000000\"/>"
	// Physical mapping of RAM chip (fx-CG 10/20 + emulator)
	"<memory type=\"ram\" start=\"0x88000000\" length=\"0x01000000\"/>"
"</memory-map>";

static void gdb_handle_query_packet(const char* packet)
{
	if (strncmp("qSupported", packet, 10) == 0) {
		const char* qsupported_ans = "PacketSize=255;qXfer:memory-map:read+";
		gdb_send_packet(qsupported_ans, strlen(qsupported_ans));
	} else if (strncmp("qXfer:memory-map:read::", packet, 23) == 0) {
		// -1 to not send the NULL terminator
		gdb_handle_qXfer_packet(&packet[23], gdb_memory_map_xml, sizeof(gdb_memory_map_xml) - 1);
	} else {
		gdb_send_packet(NULL, 0);
	}
}

static void gdb_handle_read_general_registers(gdb_cpu_state_t* cpu_state)
{
	char reply_buffer[23*8];
	if (!cpu_state) {
		memset(reply_buffer, 'x', sizeof(reply_buffer));
		memcpy(&reply_buffer[offsetof(gdb_cpu_state_t, reg.pc)*2],
		       "A0000000", 8); // pc needs to be set to make GDB happy
	} else {
		gdb_hexlify(reply_buffer, (uint8_t*)cpu_state->regs,
			    sizeof(cpu_state->regs));
	}
	gdb_send_packet(reply_buffer, sizeof(reply_buffer));
}

static void gdb_handle_read_register(gdb_cpu_state_t* cpu_state, const char* packet)
{
	uint8_t register_id = gdb_unhexlify(&packet[1]);
	char reply_buffer[8];
	if (!cpu_state || register_id >= sizeof(cpu_state->regs)/sizeof(uint32_t)) {
		memset(reply_buffer, 'x', sizeof(reply_buffer));
	} else {
		gdb_hexlify(reply_buffer, (uint8_t*)&cpu_state->regs[register_id],
			    sizeof(cpu_state->regs[register_id]));
	}
	gdb_send_packet(reply_buffer, sizeof(reply_buffer));
}

static void gdb_handle_write_general_registers(gdb_cpu_state_t* cpu_state, const char* packet)
{
	if (!cpu_state) {
		gdb_send_packet(NULL, 0);
		return;
	}

	packet++; // consume 'G'

	// Let's not handle incomplete 'G' packets as they're rarely used anyway
	if (strlen(packet) != sizeof(cpu_state->regs)*2) {
		gdb_send_packet(NULL, 0);
		return;
	}

	for (size_t i = 0; i < sizeof(cpu_state->regs)/sizeof(uint32_t); i++) {
		cpu_state->regs[i] = gdb_unhexlify_sized(&packet[i * sizeof(uint32_t) * 2],
							 sizeof(uint32_t) * 2);
	}
	gdb_send_packet("OK", 2);
}

static void gdb_handle_write_register(gdb_cpu_state_t* cpu_state, const char* packet)
{
	if (!cpu_state) {
		gdb_send_packet(NULL, 0);
		return;
	}

	char register_id_hex[16] = {0}, value_hex[16] = {0};

	packet++; // consume 'P'
	for (size_t i = 0; i < sizeof(register_id_hex); i++) {
		register_id_hex[i] = *(packet++); // consume register id
		if (*packet == '=') break;
	}
	packet++; // consume '='
	for (size_t i = 0; i < sizeof(value_hex); i++) {
		value_hex[i] = *(packet++); // consume register value
		if (*packet == '\0') break;
	}

	uint32_t register_id = gdb_unhexlify(register_id_hex);
	uint32_t value = gdb_unhexlify(value_hex);

	if (register_id >= sizeof(cpu_state->regs)/sizeof(uint32_t)) {
		gdb_send_packet(NULL, 0);
	} else {
		cpu_state->regs[register_id] = value;
		gdb_send_packet("OK", 2);
	}
}

static volatile bool gdb_tlbh_enable = false;
static volatile bool gdb_tlbh_caught = false;

static void gdb_handle_read_memory(const char* packet)
{
	char address_hex[16] = {0}, size_hex[16] = {0};
	uint8_t* read_address;
	size_t read_size;

	packet++; // consume 'm'
	for (size_t i = 0; i < sizeof(address_hex); i++) {
		address_hex[i] = *(packet++); // consume address
		if (*packet == ',') break;
	}
	packet++; // consume ','
	for (size_t i = 0; i < sizeof(size_hex); i++) {
		size_hex[i] = *(packet++); // consume size
		if (*packet == '\0') break;
	}

	read_address = (uint8_t*) gdb_unhexlify(address_hex);
	read_size = (size_t) gdb_unhexlify(size_hex);

	char *reply_buffer = malloc(read_size * 2);

	gdb_tlbh_enable = true;
	gdb_tlbh_caught = false;
	for (size_t i = 0; i < read_size && !gdb_tlbh_caught; i++) {
		gdb_hexlify(&reply_buffer[i * 2], &read_address[i], 1);
	}
	gdb_tlbh_enable = false;

	if (gdb_tlbh_caught) {
		gdb_send_packet("E22", 3); // EINVAL
		gdb_tlbh_caught = false;
	} else {
		gdb_send_packet(reply_buffer, read_size * 2);
	}
	free(reply_buffer);
}

static void cache_ocbwb(void *start, void *end)
{
	/* Cache lines are 32-aligned */
	void *p = (void *)((uintptr_t)start & -32);

	while(p < end) {
		__asm__("ocbwb @%0":: "r"(p));
		p += 32;
	}
}

static void cache_icbi(void *start, void *end)
{
	/* Cache lines are 32-aligned */
	void *p = (void *)((uintptr_t)start & -32);

	while(p < end) {
		__asm__("icbi @%0":: "r"(p));
		p += 32;
	}
}

static void gdb_handle_write_memory(const char* packet)
{
	char address_hex[16] = {0}, size_hex[16] = {0};
	uint8_t* read_address;
	size_t read_size;

	packet++; // consume 'M'
	for (size_t i = 0; i < sizeof(address_hex); i++) {
		address_hex[i] = *(packet++); // consume address
		if (*packet == ',') break;
	}
	packet++; // consume ','
	for (size_t i = 0; i < sizeof(size_hex); i++) {
		size_hex[i] = *(packet++); // consume size
		if (*packet == ':') break;
	}
	packet++; // consume ':'

	read_address = (uint8_t*) gdb_unhexlify(address_hex);
	read_size = (size_t) gdb_unhexlify(size_hex);

	gdb_tlbh_enable = true;
	gdb_tlbh_caught = false;
	for (size_t i = 0; i < read_size && !gdb_tlbh_caught; i++) {
		read_address[i] = (uint8_t)gdb_unhexlify_sized(&packet[i * 2], 2);
	}
	gdb_tlbh_enable = false;

	cache_ocbwb(read_address, read_address + read_size);
	cache_icbi(read_address, read_address + read_size);

	if (gdb_tlbh_caught) {
		gdb_send_packet("E22", 3); // EINVAL
		gdb_tlbh_caught = false;
	} else {
		gdb_send_packet("OK", 2);
	}
}

static bool gdb_parse_hardware_breakpoint_packet(const char* packet, void** read_address)
{
	packet++; // consume 'z' or 'Z'
	if (*packet != '1') { // hardware breakpoint
		return false;
	}
	packet++; // consume '1'
	packet++; // consume ','

	char address_hex[16] = {0}, kind_hex[16] = {0};
	for (size_t i = 0; i < sizeof(address_hex); i++) {
		address_hex[i] = *(packet++); // consume address
		if (*packet == ',') break;
	}
	packet++; // consume ','
	for (size_t i = 0; i < sizeof(kind_hex); i++) {
		kind_hex[i] = *(packet++); // consume kind
		if (*packet == '\0' || *packet == ';') break;
	}

	*read_address = (void*) gdb_unhexlify(address_hex);
	uint32_t read_kind = gdb_unhexlify(kind_hex);

	if (read_kind != 2) { // SuperH instructions are 2 bytes long
		return false;
	}

	return true;
}

static void gdb_handle_insert_hardware_breakpoint(const char* packet)
{
	void* read_address;
	if (!gdb_parse_hardware_breakpoint_packet(packet, &read_address)) {
		gdb_send_bridge_log("bad Z packet\n");
		gdb_send_packet(NULL, 0);
		return;
	}

	void *channel0_addr, *channel1_addr;
	bool channel0_used = ubc_get_break_address(0, &channel0_addr);
	bool channel1_used = ubc_get_break_address(1, &channel1_addr);

	/* As stated by GDB doc : "the operations should be implemented in an idempotent way."
	 * Thus we first check if the breakpoint is already placed in one of the UBC channel.
	 */
	if ((channel0_used && channel0_addr == read_address) ||
	    (channel1_used && channel1_addr == read_address)) {
		gdb_send_bridge_log("hb %p: already exists\n", read_address);
		gdb_send_packet("OK", 2);
	} else if (!channel0_used) {
		ubc_set_breakpoint(0, read_address, UBC_BREAK_BEFORE);
		gdb_send_bridge_log("hb %p: using channel 0\n", read_address);
		gdb_send_packet("OK", 2);
	} else if (!channel1_used) {
		ubc_set_breakpoint(1, read_address, UBC_BREAK_BEFORE);
		gdb_send_bridge_log("hb %p: using channel 1\n", read_address);
		gdb_send_packet("OK", 2);
	} else {
		/* TODO : We should find a proper way to inform GDB that we are
		 *        limited by the number of UBC channels.
		 */
		gdb_send_bridge_log("hb %p: channels used (%p, %p)\n", read_address,
			channel0_addr, channel1_addr);
		gdb_send_packet(NULL, 0);
	}
}

static void gdb_handle_remove_hardware_breakpoint(const char* packet)
{
	void* read_address;
	if (!gdb_parse_hardware_breakpoint_packet(packet, &read_address)) {
		gdb_send_packet(NULL, 0);
		return;
	}

	void *channel0_addr, *channel1_addr;
	bool channel0_used = ubc_get_break_address(0, &channel0_addr);
	bool channel1_used = ubc_get_break_address(1, &channel1_addr);

	if (channel0_used && channel0_addr == read_address) {
		ubc_disable_channel(0);
	}
	if (channel1_used && channel1_addr == read_address) {
		ubc_disable_channel(1);
	}
	gdb_send_packet("OK", 2);
}

static void gdb_handle_continue_with_signal(gdb_cpu_state_t* cpu_state,
	const char* packet)
{
	packet++; // consume 'C'
	int signal = gdb_unhexlify_sized(packet, 2);
	char exit[4] = { 'X', packet[0], packet[1], 0 };
	packet += 2;

	if(*packet == ';')
		cpu_state->reg.pc = gdb_unhexlify(packet + 1);

	// TODO: This is a heuristic replacing the normal signal system
	uint32_t kills =
		  (1 << 4) /* SIGILL */
		+ (1 << 6) /* SIGABRT */
		+ (1 << 7) /* SIGEMT */
		+ (1 << 8) /* SIGFPE */
		+ (1 << 9) /* SIGKILL */
		+ (1 << 11) /* SIGSEGV */
		+ (1 << 15); /* SIGTERM */
	// Abort if the signal is kill by default
	if((uint)signal < 32 && (kills >> signal) & 1) {
		gdb_send_packet(exit, 3);
		abort();
	}
}

static struct {
	bool single_stepped;
	bool channel0_used;
	bool channel1_used;
	void* channel0_addr;
	void* channel1_addr;
} gdb_single_step_backup = { false };
static void gdb_handle_single_step(uint32_t pc, ubc_break_mode_t break_mode)
{
	gdb_single_step_backup.channel0_used = ubc_get_break_address(0, &gdb_single_step_backup.channel0_addr);
	gdb_single_step_backup.channel1_used = ubc_get_break_address(1, &gdb_single_step_backup.channel1_addr);

	ubc_disable_channel(0);
	ubc_set_breakpoint(1, (void*)pc, break_mode);

	gdb_single_step_backup.single_stepped = true;
}

static bool gdb_handle_stubcall(gdb_cpu_state_t* cpu_state)
{
	char str[30];
	int sc_num = cpu_state->reg.r3;

	if(sc_num == 64) { /* write */
		int len = snprintf(str, sizeof str, "Fwrite,%x,%08x,%x",
			cpu_state->reg.r4,
			cpu_state->reg.r5,
			cpu_state->reg.r6);
		gdb_send_packet(str, len);
		return true;
	}

	return false;
}

void gdb_main(gdb_cpu_state_t* cpu_state)
{
	if (!gdb_started && gdb_start()) {
		gdb_show_stub_status(ICON_ERROR);
		return;
	}

	gdb_show_stub_status(ICON_IDLE);

	if (gdb_single_step_backup.single_stepped) {
		if (gdb_single_step_backup.channel0_used) {
			ubc_set_breakpoint(0, gdb_single_step_backup.channel0_addr, UBC_BREAK_BEFORE);
		} else {
			ubc_disable_channel(0);
		}
		if (gdb_single_step_backup.channel1_used) {
			ubc_set_breakpoint(1, gdb_single_step_backup.channel1_addr, UBC_BREAK_BEFORE);
		} else {
			ubc_disable_channel(1);
		}

		gdb_single_step_backup.single_stepped = false;
	}

	if (cpu_state != NULL) {
		/* Ajust PC after a software breakpoint */
		if (gdb_trap_number == TRA_SWBREAK)
			cpu_state->reg.pc -= 2;

		/* Handle stubcall but fallback to normal stop if it fails */
		if (gdb_trap_number != TRA_STUBCALL || !gdb_handle_stubcall(cpu_state))
			gdb_send_stop_reply();
	}

	while (1) {
		gdb_show_stub_status(ICON_COMM);

		char packet_buffer[256];
		ssize_t packet_size = gdb_recv_packet(packet_buffer, sizeof(packet_buffer));
		if (packet_size <= 0) {
			// TODO : Should we break or log on recv error ?
			continue;
		}

		gdb_show_stub_status(ICON_WORKING);

		switch (packet_buffer[0]) {
			case '?': // Halt reason
				gdb_send_stop_reply();
				break;
			case 'q':
				gdb_handle_query_packet(packet_buffer);
				break;

			case 'g':
				gdb_handle_read_general_registers(cpu_state);
				break;
			case 'p':
				gdb_handle_read_register(cpu_state, packet_buffer);
				break;
			case 'm':
				gdb_handle_read_memory(packet_buffer);
				break;
			case 'G':
				gdb_handle_write_general_registers(cpu_state, packet_buffer);
				break;
			case 'P':
				gdb_handle_write_register(cpu_state, packet_buffer);
				break;
			case 'M':
				gdb_handle_write_memory(packet_buffer);
				break;

			case 'k': // Kill request
				abort();

			case 'Z':
				gdb_handle_insert_hardware_breakpoint(packet_buffer);
				break;
			case 'z':
				gdb_handle_remove_hardware_breakpoint(packet_buffer);
				break;

			case 's':
				gdb_handle_single_step(cpu_state->reg.pc, UBC_BREAK_AFTER);
				goto ret;
			case 'c': // Continue
				goto ret;
			case 'C': // Continue with signal
				gdb_handle_continue_with_signal(cpu_state, packet_buffer);
				// We'll often abort() at the signal rather than continuing
				goto ret;
			case 'F': // Continue after File I/O call response
				// TODO: parse 'F' response packets.
				goto ret;

			default: // Unsupported packet
				gdb_send_packet(NULL, 0);
				break;
		}

		gdb_show_stub_status(ICON_IDLE);
	}

ret:
	// We're started after the first round of exchanges
	gdb_started = true;

	gdb_signal_number = 0;
	gdb_trap_number = 0;
}

static void gdb_notifier_function(void)
{
	// We ignore fxlink notifications when we're already inside GDB code.
	if (ubc_dbh_lock || !gdb_started)
		return;

	// We make sure we are called during a USB interrupt.
	if (usb_interrupt_context == NULL)
		return;

	// And we make sure an other step break is not already set up.
	if (gdb_single_step_backup.single_stepped)
		return;

	gdb_handle_single_step(usb_interrupt_context->spc, UBC_BREAK_AFTER);
}

static int gdb_panic_handler(uint32_t code)
{
	// Catch memory access errors from GDB trying to read/print stuff
	if (gdb_tlbh_enable) {
		// We only handle TLB miss reads (0x040) and writes (0x060)
		if (code != 0x040 && code != 0x060)
			return 1;

		gdb_tlbh_caught = true;

		// We skip the offending instruction and continue
		gint_exc_skip(1);
		return 0;
	}
	// If we are in user code, let's break
	else if (!ubc_dbh_lock) {
		// We make sure an other step break is not already set up
		if (gdb_single_step_backup.single_stepped)
			return 1;

		// TODO: This only works for re-execution type exceptions
		uint32_t spc;
		__asm__("stc spc, %0" : "=r"(spc));
		gdb_handle_single_step(spc, UBC_BREAK_BEFORE);

		// Break reason
		if(code == 0x040 || code == 0x060 || code == 0x0e0 || code == 0x100)
			gdb_signal_number = 11; /* SIGSEGV */
		if(code == 0x160)
			gdb_signal_number = 5; /* SIGTRAP */
		if(code == 0x180 || code == 0x1a0)
			gdb_signal_number = 4; /* SIGILL */
		if(code >= 0x1000 && code != 0x10a0)
			gdb_signal_number = 5; /* SIGTRAP */
		if(code == 0x10a0)
			gdb_signal_number = 7; /* SIGEMT (used here for bad UBC breaks) */

		// Specific stop reasons
		if(code == 0x160) {
			uint32_t TRA = isSH3() ? 0xffffffd0 : 0xff000020;
			gdb_trap_number = *(uint32_t volatile *)TRA >> 2;
		}

		return 0;
	}
	return 1;
}

static bool gdb_redirect_stdout = false;
static bool gdb_redirect_stderr = false;

static fs_descriptor_type_t const redirect_type = {
    .read = NULL,
    .write = (void *)gdb_stubcall_write,
    .lseek = NULL,
    .close = NULL,
};

int gdb_start(void)
{
	if (gdb_started)
		return 0;

	gdb_show_stub_status(ICON_WORKING);

	if(usb_is_open() && !usb_is_open_interface(&usb_ff_bulk))
		usb_close();

	if(!usb_is_open()) {
		usb_interface_t const *interfaces[] = { &usb_ff_bulk, NULL };
		if(usb_open(interfaces, GINT_CALL_NULL) < 0)
			return -1;
		usb_open_wait();
	}

	usb_fxlink_set_notifier(gdb_notifier_function);
	gdb_send_start();

	if (!gdb_recv_buffer) {
		gdb_recv_buffer = malloc(gdb_recv_buffer_capacity);
	}

	// Redirect standard streams
	if(gdb_redirect_stdout) {
		close(STDOUT_FILENO);
		open_generic(&redirect_type, (void *)STDOUT_FILENO, STDOUT_FILENO);
	}
	if(gdb_redirect_stderr) {
		close(STDERR_FILENO);
		open_generic(&redirect_type, (void *)STDERR_FILENO, STDERR_FILENO);
	}

	// TODO : Should we detect if other panic or debug handlers are setup ?
	gint_exc_catch(gdb_panic_handler);
	ubc_set_debug_handler(gdb_main);

	return 0;
}

void gdb_start_on_exception(void)
{
	gint_exc_catch(gdb_panic_handler);
	ubc_set_debug_handler(gdb_main);
}

void gdb_redirect_streams(bool stdout, bool stderr)
{
	gdb_redirect_stdout = stdout;
	gdb_redirect_stderr = stderr;
}
