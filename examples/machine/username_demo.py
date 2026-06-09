import machine

def read_username():
    USERNAME_ADDR = 0x8C1BE984
    username_bytes = bytearray()

    # Read until null byte or max 88 chars
    for i in range(88):
        char_val = machine.mem8[USERNAME_ADDR + i]
        if char_val == 0:
            break
        username_bytes.append(char_val)

    try:
        return username_bytes.decode('utf-8')
    except:
        # Fallback if invalid utf-8
        return repr(username_bytes)

def main():
    print("ClassPad 400 Username Reader Demo")
    print("--------------------------------------")

    username = read_username()
    print(f"Calculator Username: '{username}'")
    print("")

main()
