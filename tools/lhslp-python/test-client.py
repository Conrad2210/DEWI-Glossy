#!/usr/bin/env python

import socket
import time

TCP_IP = 'fec0::0b'
TCP_PORT = 20222
BUFFER_SIZE = 1024
MESSAGE = "Hello, World!"

s = socket.socket(socket.AF_INET6, socket.SOCK_STREAM)
s.connect((TCP_IP, TCP_PORT))

i_counter = 0;
while True:
    lst_msg = [0x01, (i_counter & 0xFF), (i_counter >> 8) & 0xFF]
    s.send(''.join(chr(c) for c in lst_msg))
    data = s.recv(BUFFER_SIZE)
    print list(data)
    i_counter = i_counter + 1
    #time.sleep(0.5)
s.close()
