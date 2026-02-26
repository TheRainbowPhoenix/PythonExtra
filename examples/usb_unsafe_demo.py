import gint

def main():
    print("Starting unsafe USB demo...")
    u = gint.USB()

    print("Opening USB connection...")
    try:
        u.open()
        print("Connected!")

        msg = b"Hello from unsafe demo!"
        print("Writing message:", msg)
        u.write(msg)
        u.flush() # Explicit flush needed now
        print("Write flushed.")

        print("Closing connection...")
        u.close()
        print("Disconnected.")

    except OSError as e:
        print("USB Error:", e)
        # Try to close if open failed mid-way?
        try:
            u.close()
        except:
            pass

if __name__ == "__main__":
    main()
