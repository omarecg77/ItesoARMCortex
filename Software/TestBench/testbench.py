'''
MIT No Attribution

Copyright (c) [2025] [Omar Castro]

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
'''
import serial
import time
import re


def send_bytes(ser, data):
    ser.write(data)

def receive_bytes(ser, num_bytes=1):
    data = ser.read(num_bytes)
    return data

def receive_string(ser):
    received_data = b""
    while True:
        byte = ser.read(1)
        if byte == b'\0': 
            break
        if byte: 
            received_data += byte
        else:
            break
    return received_data.decode('utf-8')



def send_size(ser, size):
    high_byte = (size >> 8) & 0xFF 
    low_byte = size & 0xFF 
    send_bytes(ser, b'a')
    send_bytes(ser, bytes([high_byte]))
    high_byte = receive_bytes(ser, 1)
    #print(high_byte)
    send_bytes(ser, bytes([low_byte]))
    low_byte = receive_bytes(ser, 1)
    #print(low_byte)

#def send_data(ser, data):
#    send_bytes(ser, b'b')
#    #received_string = receive_string(ser)
#    #print(f"Received string: {received_string}")
#    num_bits = data.bit_length()
#    num_bytes = (num_bits + 7) // 8
#    data_bytes = data.to_bytes(num_bytes, byteorder='big')
#    print(f"Sending {num_bytes} bytes: {data_bytes.hex()}")  # Debug print
#    ser.write(data_bytes)

def send_data(ser, data):
    send_bytes(ser, b'b')
    received_string = receive_string(ser)
    #print(f"Received string: {received_string}")
    num_bits = data.bit_length() 
    num_bytes = (num_bits + 7) // 8 
    data_bytes = data.to_bytes(num_bytes, byteorder='big')
    ser.write(data_bytes)
    
def get_data(ser):
    send_bytes(ser, b'c')
    for n in range (0,71):
        data = receive_bytes(ser, 1);
        print(data.hex())

def calculate_sha_software(ser):
    send_bytes(ser, b'd')

def calculate_sha_hardware(ser):
    send_bytes(ser, b'e')

def clean_buffers(ser):
    send_bytes(ser, b'h')

def get_output(ser):
    send_bytes(ser, b'f')
    received_string=''
    received_string = receive_string(ser)

    tokens = received_string.split()

    result = []
    for token in tokens:
        if len(token) == 2 and token.isalnum():
            result.append(token)
        elif len(token) == 1 and token.isalnum():
            result.append('0' + token)
        else:
            continue
    output_string = ''.join(result).lower()
    return output_string

def get_latency(ser):
    send_bytes(ser, b'g') 
    received_string = receive_string(ser)
    #print(f"Received string: {received_string}")
    tokens = received_string.split()
    result = []
    for token in tokens:
        result.append(token)
    return result


def main():
    ser = serial.Serial(
        port='COM4',
        baudrate=115200,
        timeout=1
    )
    file_path = 'SHA3_512ShortMsg.rsp'
    output_file_path = 'output.txt'
    time.sleep(2)
    with open(output_file_path, 'w') as output_file:
        if ser.is_open:
            print("Serial port is open.")
            output_file.write("Serial port is open.\n")
            received_string = receive_string(ser)
            print(f"Received string: {received_string}")
            output_file.write(f"Received string: {received_string}\n")
            with open(file_path, 'r') as file:
                lines = file.readlines()
            length = None
            message = None
            md = None
            len_pattern = re.compile(r'Len\s*=\s*(\d+)')
            msg_pattern = re.compile(r'Msg\s*=\s*([0-9a-fA-F]+)')
            md_pattern = re.compile(r'MD\s*=\s*([0-9a-fA-F]+)')

            for line in lines:
                len_match = len_pattern.search(line)
                msg_match = msg_pattern.search(line)
                md_match = md_pattern.search(line)

                if len_match:
                    length = int(len_match.group(1))
                if msg_match:
                    message = int(msg_match.group(1), 16)
                if md_match:
                    md = int(md_match.group(1), 16)
                if length is not None and message is not None and md is not None:
                    print("#####################################################################################################################################################")
                    print("##                                                               HARDWARE                                                                          ##")
                    print("#####################################################################################################################################################")
                    clean_buffers(ser)
                    output_file.write(f"Hardware\n")
                    print(f"send_size: {length} bits")
                    output_file.write(f"send_size: {length} bits\n")
                    send_size(ser, length)
                    print(f"Input:{hex(message)}")
                    output_file.write(f"Input:{hex(message)}\n")
                    send_data(ser, message)
                    #print("get_data")
                    #get_data(ser)
                    calculate_sha_hardware(ser)
                    data = get_output(ser)
                    print(f"Output from FPGA 0x{data}")
                    output_file.write(f"Output from FPGA 0x{data}\n")
                    print(f"Output from file {hex(md)}")
                    output_file.write(f"Output from file {hex(md)}\n")
                    print("Hardware latency")
                    output_file.write("Hardware latency\n")
                    data = get_latency(ser)
                    print(f"Start Time {data[0]}, End Time {data[1]}")
                    output_file.write(f"Start Time {data[0]}, End Time {data[1]}\n")
                    print(f"Latency in Cycles {data[2]}")
                    output_file.write(f"Latency in Cycles {data[2]}\n")
                    print("#####################################################################################################################################################")
                    print("##                                                               SOFTWARE                                                                          ##")
                    print("#####################################################################################################################################################")
                    clean_buffers(ser)
                    output_file.write(f"Software\n")
                    print(f"send_size: {length} bits")
                    output_file.write(f"send_size: {length} bits\n")
                    send_size(ser, length)
                    print(f"Input:{hex(message)}")
                    output_file.write(f"Input:{hex(message)}\n")
                    send_data(ser, message)
                    #print("get_data")
                    #get_data(ser)
                    calculate_sha_software(ser)
                    data = get_output(ser)
                    print(f"Output from FPGA 0x{data}")
                    output_file.write(f"Output from FPGA 0x{data}\n")
                    print(f"Output from file {hex(md)}")
                    output_file.write(f"Output from file {hex(md)}\n")
                    data = get_latency(ser)
                    print(f"Start Time {data[0]}, End Time {data[1]}")
                    output_file.write(f"Start Time {data[0]}, End Time {data[1]}\n")
                    print(f"Latency in Cycles {data[2]}")
                    output_file.write(f"Latency in Cycles {data[2]}\n")
                    length = None
                    message = None
                    md = None
            ser.close()

        else:
            print("Failed to open serial port.")
            output_file.write("Failed to open serial port.\n")

if __name__ == "__main__":
    main()