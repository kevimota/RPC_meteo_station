import serial
import argparse

parser = argparse.ArgumentParser()
parser.add_argument("-p", "--port", help="Serial Port", type=str, required=True)
args = parser.parse_args()

meteo = serial.Serial(args.port, 115200)

from multiprocessing import Process

def read_serial():
    while True:
        line = meteo.readline()
        print(line.decode().strip('\r\n'))   

if __name__ == '__main__':
    read_process = Process(target=read_serial)
    read_process.start()

    while True:
        command = input()
        if command == "exit":
            break
        command = command + "\n"
        meteo.write(bytes(command.encode()))

    read_process.terminate()

    