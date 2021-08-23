Uclogic-new-init
===============

This program is meant to act as a compliment to uclogic-probe from the DIGImend project, and is intended to be used after following the instructions to collect tablet diagnostics as described [here](https://digimend.github.io/support/howto/trbl/diagnostics/)

Newer uclogic based drawing tablets seem to require a "key packet" to be sent to the OUT endpoint of a specific interface in order to be initialized properly. When initialized, each frame button is mapped to a single bit in the HID reports, which is usually what is expected of tablets (or digitizers, as they are referred to in HID documentation). Otherwise, the tablet reports generic keyboard events for frame buttons. This generally seems to be the case with newer XP-Pen tablets, but there are a few from other vendors which show characteristics of devices needing this procedure.

This utility aims to discover the key packet which needs to be sent to the device, as well as the interface it needs to be sent on. As of now, there is no known pattern to the packets, so it simply tries a list of known packets (2 as of now).

Currently only the python version works properly.

Who should use this
-------------------
You should use this program if your tablet shows characteristics of needing a "key packet". One key indicator is the output of `usbhid-dump` while collecting events for the tablet frame buttons. Tablet buttons should report as a bitmap, so you should see values like 01, 02, 04, 08, 10, 20, 40 or 80 while pressing the buttons. If pressing the buttons causes values like 05, 0C or 01 16 to show up, it is likely that these are keyboard events and thus the tablet may require the key packet.

Usage
-----

This program requires pyusb to be installed, and requires root permissions to be run.

IMPORTANT: In order to detect the correct interface, the pen must be held over the tablet surface (as if you were writing something) for the duration of the program execution.

The program can be run using

`sudo ./uclogic_new_init.py VID:PID`

where VID:PID is the USB ID of your tablet as output in the command `lsusb` (See the "Identify original model" section on the tablet diagnostics page linked above for more information about finding the IDs)

Afterwards, diagnostics for DIGImend can be collected as normal using `usbhid-dump` as described in the section "Collect raw input samples" from the tablet diagnostics instruction page, linked above.
