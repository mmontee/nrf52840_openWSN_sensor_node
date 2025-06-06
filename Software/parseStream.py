import time
import serial

def main():
    # Get the COM port name from the user
    com_port = input("Enter the COM port name (e.g., COM3 or /dev/ttyUSB0) [default: /dev/ttyUSB0]: ") or "/dev/ttyUSB0"
    user_keyword = input("Enter the keyword to stop reading [default: uinject]: ").encode('utf-8') or b'uinject'  # Convert keyword to bytes
    print(f"{time.strftime('%Y-%m-%d %H:%M:%S')} - Listening on {com_port} for user keyword '{user_keyword.decode('utf-8')}'...")

    # Define internal keywords and the end character
    internal_keywords = [b'D']  # Add your internal keywords here
    end_character = b'~'  # Define the end character for internal messages

    try:
        # Create a serial connection
        with serial.Serial(com_port, baudrate=115200, timeout=1) as ser:
            print(f"{time.strftime('%Y-%m-%d %H:%M:%S')} - Connected to {com_port}. Reading data...")

            buffer = b""  # Use a bytes buffer
            id_dict = {}  # Dictionary to store ID: (data_value, clock_value)
            while True:
                # Read one byte at a time
                char = ser.read()
                if char:
                    buffer += char

                    # Check for internal keywords
                    for internal_keyword in internal_keywords:
                        if internal_keyword in buffer:
                            start_index = buffer.find(internal_keyword)
                            end_index = buffer.find(end_character, start_index)
                            if end_index != -1:
                                message = buffer[start_index:end_index + 1]

                                # Check for user keyword
                                if user_keyword in buffer:
                                    keyword_index = buffer.find(user_keyword)
                                    after_keyword = buffer[keyword_index + len(user_keyword): keyword_index + len(user_keyword) + 9]
                                    if len(after_keyword) == 9:
                                        data_bytes = after_keyword[0:2]
                                        id_bytes = after_keyword[2:4]
                                        clock_bytes = after_keyword[4:9]

                                        data_value = int.from_bytes(data_bytes, byteorder='little')
                                        id_hex = id_bytes.hex()
                                        clock_value = int.from_bytes(clock_bytes, byteorder='little')

                                        # Add timestamp and message length
                                        msg_timestamp = time.strftime('%Y-%m-%d %H:%M:%S')
                                        msg_length = len(message)

                                        if id_hex not in id_dict:
                                            print(f"New ID detected: {id_hex}")
                                        id_dict[id_hex] = {
                                            'data_bytes': data_bytes.hex(),
                                            'data_value': data_value,
                                            'clock_bytes': clock_bytes.hex(),
                                            'clock_value': clock_value,
                                            'timestamp': msg_timestamp,
                                            'msg_length': msg_length
                                        }

                                        # Print the current list of IDs and their values
                                        print("\nCurrent ID List:")
                                        for k, v in id_dict.items():
                                            print(f"ID: {k} | Data: {v['data_bytes']} ({v['data_value']}) | Clock: {v['clock_bytes']} ({v['clock_value']}) | Time: {v['timestamp']} | MsgLen: {v['msg_length']}")
                                        print("-" * 40)

                                # Remove the processed message from the buffer
                                buffer = buffer[end_index + 1:]
                            break  # Exit the loop after processing one internal message

    except serial.SerialException as e:
        print(f"{time.strftime('%Y-%m-%d %H:%M:%S')} - Error: {e}")
    except KeyboardInterrupt:
        print(f"{time.strftime('%Y-%m-%d %H:%M:%S')} - Stopped by user.")

if __name__ == "__main__":
    main()















