#!/usr/bin/env python3
#
# Copyright (C) 2021 Nicholas Chin
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 2 of the License, or
# (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

import sys
from os import getuid
import usb.core
import usb.util

key_packets = ['02B002', '02B004']

if getuid() != 0:
    print("This program must be run with root privileges")
    sys.exit()

# Check number of arguments
if len(sys.argv) < 2:
    print("Missing argument.")
    print("Usage: ./uclogic_new_init.py VID:PID")
    sys.exit()

# Help message
if (sys.argv[1] == "-h" or sys.argv[1] == "--help"):
    print("This program requires root privileges.")
    print("Usage: ./uclogic_new_init.py VID:PID")
    print("Please hold the pen on the tablet surface while running")
    sys.exit()

# Parse arguments
try:
    vid, pid = [int(i, 16) for i in sys.argv[1].split(":")]
except ValueError:
    print("Bad argument")
    print("Usage: ./uclogic_new_init.py VID:PID")
    sys.exit()

dev = usb.core.find(idVendor=vid, idProduct=pid)
if dev is None:
    print("Error! Device not found!")
    sys.exit()

cfg = dev.get_active_configuration()
in_endpoints = list()

# Search for OUT endpoint to send the init key to
for i, iface in enumerate(cfg):
    if dev.is_kernel_driver_active(i):
        dev.detach_kernel_driver(i)
    for e in iface.endpoints():
        if e.bEndpointAddress & 0x80 == 0:
            ei = iface[0]
            eo = iface[1]
            iface_num = i
            in_packet_size = ei.wMaxPacketSize
            out_packet_size = eo.wMaxPacketSize
        else:
            in_endpoints.append(e)

# Test all known key packets
for p in key_packets:
    packet = p + '00'*(out_packet_size-len(p)//2)
    print(f"Trying init packet 0x{packet}:")
    eo.write(bytes.fromhex(packet))

    # Read response packet
    data = ei.read(in_packet_size)
    print("Received " + ''.join(format(x, '02X') for x in data))

    # Flush interfaces
    for e in in_endpoints:
        try:
            e.read(in_packet_size, 1000)
        except usb.core.USBTimeoutError:
            continue

    is_initialized = False
    old_deactivated = True

    # Invalid init packets will enable the correct interface, but
    # do not disable the old ones. Check for reports on the old
    # interfaces to determine key validity
    for e in in_endpoints:
        try:
            data = e.read(in_packet_size, 1000)
            if data is not None:
                if e.bEndpointAddress == ei.bEndpointAddress:
                    is_initialized = True
                else:
                    print("Invalid key!\n")
                    old_deactivated = False
                    break
        except usb.core.USBTimeoutError:
            # Skip interfaces which did not send an event
            continue

    if is_initialized and old_deactivated:
        endpoint = "0x{0:02X}".format(eo.bEndpointAddress)
        print(f"Key is 0x{packet}, interface {iface_num}, endpoint {endpoint}")
        break
