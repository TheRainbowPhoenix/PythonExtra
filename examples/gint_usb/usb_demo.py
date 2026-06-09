import gint

def main():
    print("Connecting to USB...")
    with gint.USB() as u:
        print("Connected!")
        # Example: write a simple message
        u.write(b"Hello from calculator!")
        u.flush() # Explicit flush is required to commit the USB transfer!

        # Example: read response
        print("Waiting for response...")
        try:
            data = u.read(64)
            print("Received:", data)
        except OSError as e:
            print("Error reading:", e)

    print("Disconnected.")

main()
