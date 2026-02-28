import gint
import machine

# VRAM address
VRAM_ADDR = 0x8c052800

def main():
    print("Starting USB & Machine demo...")
    print("Run this script, then on PC run: python fxlink_py.py /echo Hello")
    print("Or send commands like '/vram'")

    try:
        with gint.USB() as u:
            print("USB Connected! Waiting for data (sync read)...")

            while True:
                try:
                    # Read up to 64 bytes
                    data = u.read(64)
                    if not data:
                        # Empty read (timeout or no data)
                        continue

                    print("Received:", data)

                    # Very simple parsing
                    if b"echo" in data:
                        # Echo back
                        reply = b"Echoing: " + data
                        u.fxlink_header("python", "text", len(reply))
                        u.write(reply)
                        u.flush()
                        print("Echoed back!")

                    elif b"vram" in data:
                        # Read first few bytes of VRAM using machine.mem32
                        vram_val1 = machine.mem32[VRAM_ADDR]
                        vram_val2 = machine.mem32[VRAM_ADDR + 4]

                        reply = f"VRAM starts with: {hex(vram_val1)}, {hex(vram_val2)}".encode('utf-8')
                        u.fxlink_header("python", "text", len(reply))
                        u.write(reply)
                        u.flush()
                        print("Sent VRAM info.")

                    elif b"quit" in data:
                        print("Quit command received. Exiting.")
                        break

                except OSError as e:
                    print("Read error:", e)
                    break

    except Exception as e:
        print("Error:", e)

    print("Demo finished.")

if __name__ == "__main__":
    main()
