import socket
import time

LOCAL_IP = "192.168.1.1"
LOCAL_PORT = 5005

SERVER_IP = "192.168.1.2"
SERVER_PORT = 5005

TIMEOUT_S = 2.0

def main():
    message_count = 0

    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as sock:
        # Bind to your PC's static Ethernet IP
        sock.bind((LOCAL_IP, LOCAL_PORT))
        sock.settimeout(TIMEOUT_S)

        print(f"UDP echo client bound to {LOCAL_IP}:{LOCAL_PORT}")
        print(f"Sending to STM32 at {SERVER_IP}:{SERVER_PORT}")
        print("Press Ctrl+C to stop.\n")

        try:
            while True:
                message = f"hello {message_count}"
                tx = message.encode("utf-8")

                print(f"TX: {message}")
                sock.sendto(tx, (SERVER_IP, SERVER_PORT))

                try:
                    data, addr = sock.recvfrom(2048)
                    print(f"RX from {addr[0]}:{addr[1]}: {data.decode('utf-8', errors='replace')}\n")
                except socket.timeout:
                    print("RX timeout: no reply from board\n")

                message_count += 1
                time.sleep(1.0)

        except KeyboardInterrupt:
            print("\nStopped.")

if __name__ == "__main__":
    main()