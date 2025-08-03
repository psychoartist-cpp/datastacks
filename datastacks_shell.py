import json
import time
import socket

s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
host = input("DataStacks host [default: localhost]: ")
password = input("DataStacks password [default 123]: ")

if not host: host = "localhost"


if not password: password = "123"

port = 3008
while port_str := input("DataStacks port [default: 3008]: "):
    if port == "":
        port = 3008
        break
    try:
        port = int(port_str)
    except:
        print("Invalid port!")
        continue

s.connect((host, port))

to_send = json.dumps({"password": password})
s.sendall(to_send.encode('utf-8'))

data4= s.recv(1024)
if data4.decode('utf-8') == 'OK':
    print("Authorized!")


while (uinput := input("[datastacks] > ")) != "exit":
    start = time.perf_counter()
    s.sendall(uinput.encode())
    data = s.recv(1024)
    end = time.perf_counter()
    print(f'{data.decode('utf-8')}')
    print(f"Elapsed: {end - start}")

