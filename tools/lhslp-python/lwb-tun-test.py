#!/usr/bin/python

import serial
import lhslp
import sys
import time
import sys
import getopt
import os
import Queue
import threading
import struct
import subprocess
import fcntl
import select
import signal

b_exit_flag = False

#---------------------------------------------------------------------------------------------------

def signal_handler(signal, frame):
    global b_exit_flag
    global o_thread_shutdown_event

    print ""
    b_exit_flag = True
    o_thread_shutdown_event.set()


#---------------------------------------------------------------------------------------------------
class ThreadSafeFile(object):
    def __init__(self, f):
        self.f = f
        self.lock = threading.RLock()
        self.nesting = 0

    def _getlock(self):
        self.lock.acquire()
        self.nesting += 1

    def _droplock(self):
        nesting = self.nesting
        self.nesting = 0
        for i in range(nesting):
            self.lock.release()

    def __getattr__(self, name):
        if name == 'softspace':
            return tls.softspace
        else:
            raise AttributeError(name)

    def __setattr__(self, name, value):
        if name == 'softspace':
            tls.softspace = value
        else:
            return object.__setattr__(self, name, value)

    def write(self, data):
        self._getlock()
        self.f.write(data)
        if data == '\n':
            self._droplock()


#---------------------------------------------------------------------------------------------------

def hexdump(src, length=16):
    FILTER = ''.join([(len(repr(chr(x))) == 3) and chr(x) or '.' for x in range(256)])
    lines = []
    for c in xrange(0, len(src), length):
        chars = src[c:c+length]
        hex = ' '.join(["%02x" % x for x in chars])
        printable = ''.join(["%s" % ((x <= 127 and FILTER[x]) or '.') for x in chars])
        lines.append("%04x  %-*s  %s\n" % (c, length*3, hex, printable))
    return ''.join(lines)

#---------------------------------------------------------------------------------------------------
def show_help(path):
    print "%s   [options] device" % path
    print "Serial dump for LHSLP"
    print "-b|--baudrate        Baudrate of the serial port"
    print "-h|--help            Show this help"

#---------------------------------------------------------------------------------------------------

def lhslp_do_exchange(o_shutdown_event, o_serial, q_in_queue, q_out_queue):
    
    print "Starting serial read/write"
    
    o_lhslp = lhslp.LHSLP() 
    lst_characters = []
    
    while not o_shutdown_event.is_set():
        lst_characters.extend(o_serial.read(1))
        packet = o_lhslp.decode_packet(lst_characters)
        if packet is None:
            continue  
            
        if packet.main_type == lhslp.Packet.MAIN_TYPE_DBG_DATA and packet.sub_type == lhslp.Packet.SUB_TYPE_DBG_DATA:
            
            sys.stdout.write(''.join(chr(i) for i in packet.data))
            
        elif packet.main_type == lhslp.Packet.MAIN_TYPE_DATA_REQ and packet.sub_type == lhslp.Packet.SUB_TYPE_SCHED:
            
            lstDumSched = [0x01, 0x00, 0x0c, 0x00, 0x84, 0x01, 0x01, 0x00, 0x0B, 0x00, 0x01, 0x00, 0x0B, 0x00]
            o_out_packet = lhslp.Packet(lhslp.Packet.MAIN_TYPE_DATA, lhslp.Packet.SUB_TYPE_SCHED, lstDumSched)
            o_serial.write(''.join(chr(c) for c in o_lhslp.encode_packet(o_out_packet)))
            
        elif packet.main_type == lhslp.Packet.MAIN_TYPE_DATA_REQ and packet.sub_type == lhslp.Packet.SUB_TYPE_APP_DATA:
            if not q_in_queue.empty():
                lst_out_packet = q_in_queue.get()
                print "HOST > NODE len : %d" % len(lst_out_packet)
                
                lst_app_data = [0x00, 0x00, 0x00, 0x00, len(lst_out_packet) & 0xFF, 0x00]
                lst_app_data.extend(lst_out_packet)
                o_out_packet = lhslp.Packet(lhslp.Packet.MAIN_TYPE_DATA, lhslp.Packet.SUB_TYPE_APP_DATA, lst_app_data)
                o_serial.write(''.join(chr(c) for c in o_lhslp.encode_packet(o_out_packet)))
            
        elif packet.main_type == lhslp.Packet.MAIN_TYPE_DATA and packet.sub_type == lhslp.Packet.SUB_TYPE_APP_DATA:
            if len(packet.data) > 6:
                lst_data_header = packet.data[:6]
                lst_in_packet = packet.data[6:]
                print "data header %s " % ' '.join("%02X" % c for c in lst_data_header)
                print "NODE > HOST len : %d" % len(lst_in_packet)
                q_out_queue.put(lst_in_packet)
            else:
                print "Invalid data"
                print hexdump(packet.data)
            
                
#         else:
#             print "Main Type : %d, Sub Type : %d" % (packet.main_type, packet.sub_type)
#             print hexdump(packet.data)
    print "Stopping serial read/write"


#---------------------------------------------------------------------------------------------------

def tun_do_read(o_shutdown_event, o_tun, q_out_queue):
    print "Starting reading tun"
    while not o_shutdown_event.is_set():
        lst_readables, lst_writables, lst_exceptionals = select.select([o_tun], [], [], 0.1)
        for o_readable in lst_readables:
            if o_readable == o_tun:
                # We have something to read
                recv_bytes = os.read(o_tun.fileno(), 2048)
                if len(recv_bytes) > 0:
                    q_out_queue.put(map(ord, list(recv_bytes)))
    print "Stopping reading tun"
    
#---------------------------------------------------------------------------------------------------

def tun_do_write(o_shutdown_event, o_tun, q_in_queue):
    print "Starting writing to tun"
    while not o_shutdown_event.is_set():            
        try:
            lst_out_packet = q_in_queue.get(timeout = 0.01)
    #             print "Outgoing IP packet len %d" % len(lst_out_packet)
            os.write(o_tun.fileno(), ''.join(chr(c) for c in lst_out_packet))
        except Queue.Empty:
            pass
    print "Stopping writing to tun"
#---------------------------------------------------------------------------------------------------

def create_tun(s_tun_name, s_tun_address):
    TUNSETIFF   = 0x400454ca
    TUNSETOWNER = TUNSETIFF + 2
    IFF_TUN     = 0x0001
    IFF_TAP     = 0x0002
    IFF_NO_PI   = 0x1000
    
    # Open TUN device file.
    o_tun = open('/dev/net/tun', 'r+b')
    # Tall it we want a TUN device named tun0.
    s_ifr = struct.pack('16sH', s_tun_name, IFF_TUN | IFF_NO_PI)
    fcntl.ioctl(o_tun, TUNSETIFF, s_ifr)
    # Optionally, we want it be accessed by the normal user.
    fcntl.ioctl(o_tun, TUNSETOWNER, 1000)

    # Bring it up and assign addresses.
    subprocess.check_call('ifconfig ' + s_tun_name + ' add ' + s_tun_address + ' up', shell=True)
    
    return o_tun
    
#---------------------------------------------------------------------------------------------------

i_baud_rate = 115200
s_device = "/dev/ttyUSB0"

s_tun_name = "tun0"
s_tun_address = "fec0::f0/64"

sys.stdout = ThreadSafeFile(sys.stdout)

try:
    opts, args = getopt.getopt(sys.argv[1:], "b:h", ["baudrate=", "help"])

except getopt.GetoptError, err:
    print str(err)
    sys.exit(2)
  
for opt, arg in opts:
    if opt in ('-b', '--baudrate'):
        i_baud_rate = int(arg)
    elif opt in ('-h', '--help'):
        show_help(sys.argv[0])
        sys.exit(0)
    else:
        print 'Unknown option'
        sys.exit(1)

if len(args) > 0:
    s_device = str(args[0])
else:
    print "ERROR: No device specified"
    show_help(sys.argv[0])
    sys.exit(1)


q_tun_in_queue = Queue.Queue()
q_tun_out_queue = Queue.Queue()
o_thread_shutdown_event = threading.Event()

o_tun = create_tun(s_tun_name, s_tun_address)

if (o_tun is None):
    print "ERROR: Error in creating tun device"
    sys.exit(1)
    
o_serial = serial.Serial(port=s_device, baudrate=i_baud_rate, timeout = 0.5)

if (o_serial is None):
    print "ERROR: Error in creating opening serial device"
    sys.exit(1)

o_tun_read_thread = threading.Thread(target=tun_do_read, 
                                    args=(o_thread_shutdown_event, 
                                    o_tun, 
                                    q_tun_out_queue))

o_tun_wite_thread = threading.Thread(target=tun_do_write, 
                                    args=(o_thread_shutdown_event, 
                                    o_tun, 
                                    q_tun_in_queue))

o_lhslp_thread = threading.Thread(target=lhslp_do_exchange, 
                                  args=(o_thread_shutdown_event, 
                                        o_serial, 
                                        q_tun_out_queue, 
                                        q_tun_in_queue))


signal.signal(signal.SIGINT, signal_handler)

lst_threads = []
lst_threads.append(o_tun_read_thread)
lst_threads.append(o_tun_wite_thread)
lst_threads.append(o_lhslp_thread)

for o_thread in lst_threads:
    o_thread.start()


while not b_exit_flag:
    time.sleep(0.1)

for o_thread in lst_threads:
    o_thread.join()
    
o_tun.close()
o_serial.close()