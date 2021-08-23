/* 
 * Copyright (C) 2021 Nicholas CHin
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#include <libusb.h>
#include <stdio.h>

int main(int argc, char **argv)
{
	// Initialization	
	libusb_context *ctx = NULL;
	if (libusb_init(&ctx) != 0)
		printf("Error: Init\n");
	
	libusb_device_handle *dev_h;
	struct libusb_config_descriptor *cfg_desc;
	
	// Read vid and pid from command line argument
	unsigned short vid, pid;
	sscanf(argv[1], "%hx:%hx", &vid, &pid);
	printf("VID: %hx, PID: %hx\n", vid, pid);
	
	// Open specified usb device
	dev_h = libusb_open_device_with_vid_pid(ctx, vid, pid);
	if (dev_h == NULL)
		printf("Error: Open\n");

	if (libusb_get_active_config_descriptor(libusb_get_device(dev_h), &cfg_desc) != 0) {
		printf("Error: Read config descriptor\n");
	}

	// Detach kernel driver for each interface	
	for (int i = 0; i < cfg_desc->bNumInterfaces; i++) {
		printf("Kernel detach: %s\n", libusb_strerror(libusb_detach_kernel_driver(dev_h, i)));
		if (libusb_claim_interface(dev_h, i) != 0) {
			printf("Error: Claim interface\n");
		}
	}

	/* Determine the correct endpoint to send the initialization packet and receive the response.
	 * So far it seems like these endpoints are on the only interface with 2 endpoints, one out
	 * and one in. All other interfaces (the ones that the tablet sends reports over in their
	 * uninitialized state) seem to only have a single in endpoint.
	 */
	unsigned char out_addr, in_addr;
	for (int i = 0; i < cfg_desc->bNumInterfaces; i++) {
		if (cfg_desc->interface[i].altsetting[0].bNumEndpoints == 2) {
			unsigned char addr = cfg_desc->interface[i].altsetting[0].endpoint[0].bEndpointAddress;
			// bit 7 indicates direction. 1 is in, 0 is out
			if (addr & 0x80) {
				in_addr = addr;
				out_addr = cfg_desc->interface[i].altsetting[0].endpoint[1].bEndpointAddress;
			} else {
				out_addr = addr;
				in_addr = cfg_desc->interface[i].altsetting[0].endpoint[1].bEndpointAddress;
			}
		}
	}
	
	unsigned char init_packet[] = {0x02, 0xb0, 0, 0, 0, 0, 0, 0};
	unsigned char data_buff[10] = {0};
	for (int code = 0; code <= 6; code++) { 
		int len = 0;
		init_packet[2] = code;
		printf("\nTrying ");
		for (int i = 0; i<8; i++){
			printf("%02X ", init_packet[i]);
		}
		printf("\nSending init packet: %s\n", libusb_strerror(libusb_interrupt_transfer(dev_h, out_addr, init_packet, 8, &len, 0)));
		//printf("%d\n", len);
		printf("Receive packet: %s\n", libusb_strerror(libusb_interrupt_transfer(dev_h, in_addr, data_buff, 10, &len, 3000)));
		//printf("%d\n", len);
		printf("Received data: ");
		for (int i = 0; i<8; i++){
			printf("0x%02X ", data_buff[i]);
		}
		printf("\n");
	}
	// Reattach kernel driver for each interface	
	for (int i = 0; i < cfg_desc->bNumInterfaces; i++) {
		libusb_release_interface(dev_h, 2);
		libusb_attach_kernel_driver(dev_h, 2);
	}
	libusb_free_config_descriptor(cfg_desc);
	libusb_close(dev_h);
	libusb_exit(ctx);
	return 0;
}
