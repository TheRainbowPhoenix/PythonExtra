import gint

def main():
    print("Starting fxlink USB demo...")
    with gint.USB() as u:
        print("Connected!")

        # 1. Text Transfer
        print("Sending text packet...")
        text_payload = b"Hello from Python via fxlink!"
        # Application="gintctl" (or custom), Type="text", Size=len(payload)
        # Note: "text" type in fxlink expects just the string.
        u.fxlink_header("python", "text", len(text_payload))
        u.write(text_payload)
        u.flush()
        print("Text sent.")

        # 2. Image Transfer (VRAM)
        # Assuming we can get VRAM content. For now, let's send a dummy image or text as image.
        # Ideally, we'd grab VRAM using gint.
        # But `gint` module doesn't expose VRAM pointer easily to Python yet.
        # Let's send a generated pattern.
        width = 10
        height = 10
        # RGB565 is 2 bytes per pixel
        img_size = width * height * 2
        img_data = bytearray(img_size)
        for i in range(img_size):
            img_data[i] = i % 255

        print("Sending image packet...")
        # For fxlink "image" type, there's a subheader "usb_fxlink_image_t" needed *before* pixels?
        # The C code: usb_fxlink_fill_header(&header_image, "justui", "image", vram_size);
        # And then it sends vram directly?
        # Wait, the gint header says: "fxlink messages consist of a simple header followed by message contents (sometimes with a subheader)."
        # "Subheader for the fxlink built-in 'image' type: usb_fxlink_image_t"
        # So we need to construct that manually if `fxlink_header` only does the main header.

        # struct usb_fxlink_image { uint32_t width; uint32_t height; int pixel_format; };
        # pixel_format: 0=RGB565
        import struct
        sub_header = struct.pack("<IIi", width, height, 0) # Little endian

        total_size = len(sub_header) + len(img_data)

        u.fxlink_header("python", "image", total_size)
        u.write(sub_header)
        u.write(img_data)
        u.flush()
        print("Image sent.")

    print("Disconnected.")

if __name__ == "__main__":
    main()
