#!/usr/bin/python

import serial
import lhslp
import sys
import time
import sys
import getopt
import os

def hexdump(src, length=16):
    FILTER = ''.join([(len(repr(chr(x))) == 3) and chr(x) or '.' for x in range(256)])
    lines = []
    for c in xrange(0, len(src), length):
        chars = src[c:c+length]
        hex = ' '.join(["%02x" % x for x in chars])
        printable = ''.join(["%s" % ((x <= 127 and FILTER[x]) or '.') for x in chars])
        lines.append("%04x  %-*s  %s\n" % (c, length*3, hex, printable))
    return ''.join(lines)

#--------------------------------------------------------------------------------------------------
def show_help(path):
    print "%s   [options] device" % path
    print "Serial dump for LHSLP"
    print "-b|--baudrate        Baudrate of the serial port"
    print "-h|--help            Show this help"

#--------------------------------------------------------------------------------------------------
i_baud_rate = 115200
s_device = "/dev/ttyUSB0"

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


o_serial = serial.Serial(port=s_device, baudrate=i_baud_rate)

o_lhslp = lhslp.LHSLP() 
lst_characters = []

while 1:
    lst_characters.extend(o_serial.read(1))
    packet = o_lhslp.decode_packet(lst_characters)
    if packet is None:
        continue

    if packet.main_type == lhslp.Packet.MAIN_TYPE_DBG_DATA:
        sys.stdout.write(''.join(chr(i) for i in packet.data))
        #print hexdump(packet.data)
#     elif packet.main_type == lhslp.Packet.MAIN_TYPE_DATA_REQ:
#         if packet.sub_type == lhslp.Packet.SUB_TYPE_SCHED:
#             lstDumSched = [0x01, 0x00, 0x0c, 0x00, 0x84, 0x01, 0x09, 0x00, 0x0B, 0x00, 0x09, 0x00, 0x0B, 0x00]
#             o_packet = lhslp.Packet(lhslp.Packet.MAIN_TYPE_DATA, lhslp.Packet.SUB_TYPE_SCHED, lstDumSched)
#             o_serial.write(''.join(chr(c) for c in o_lhslp.encode_packet(o_packet)))
#     else:
#         print "Main Type : %d, Sub Type : %d" % (packet.main_type, packet.sub_type)
#         print hexdump(packet.data)
