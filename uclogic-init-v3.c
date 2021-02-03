#include <libusb.h>
#include <stdio.h>

int main(int argc, char **argv)
{
	libusb_context *ctx = NULL;
	libusb_device_handle *dev_h;
	if (libusb_init(&ctx) != 0)
		printf("Error: Init\n");
	dev_h = libusb_open_device_with_vid_pid(ctx, 0x28bd, 0x0078);
	if (dev_h == NULL)
		printf("Error: Open\n");
	printf("Kernel detach: %s\n", libusb_strerror(libusb_detach_kernel_driver(dev_h, 2)));
	if (libusb_claim_interface(dev_h, 2) != 0)
		printf("Error: Claim interface\n");

	unsigned char init_packet[] = {0x02, 0xb0, 0x02};
	unsigned char data_buff[8] = {0};
	int len = 0;
	printf("Init packet: %s\n", libusb_strerror(libusb_interrupt_transfer(dev_h, 0x3, init_packet, 3, &len, 0)));
	printf("%d\n", len);
	printf("Receive packet: %s\n", libusb_strerror(libusb_interrupt_transfer(dev_h, 0x83, data_buff, 8, &len, 3000)));
	printf("%d\n", len);
	for (int i = 0; i<8; i++){
		printf("0x%02X ", data_buff[i]);
	}
	printf("\n");
	libusb_release_interface(dev_h, 2);
	libusb_attach_kernel_driver(dev_h, 2);
	libusb_close(dev_h);
	libusb_exit(ctx);
	return 0;
}
