/* Name: set-led.c
 * Project: custom-class, a basic USB example
 * Author: Christian Starkjohann
 * Creation Date: 2008-04-10
 * Tabsize: 4
 * Copyright: (c) 2008 by OBJECTIVE DEVELOPMENT Software GmbH
 * License: GNU GPL v2 (see License.txt), GNU GPL v3 or proprietary (CommercialLicense.txt)
 */

/*
General Description:
This is the host-side driver for the custom-class example device. It searches
the USB for the LEDControl device and sends the requests understood by this
device.
This program must be linked with libusb on Unix and libusb-win32 on Windows.
See http://libusb.sourceforge.net/ or http://libusb-win32.sourceforge.net/
respectively.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <usb.h>        /* this is libusb */
#include "opendevice.h" /* common code moved to separate module */

#include "../firmware/requests.h"   /* custom request numbers */
#include "../firmware/usbconfig.h"  /* device's VID/PID and names */

void usage( const char* name )
{
    fprintf( stderr, "usage:\n" );
    fprintf( stderr, "  %s setled val ....... set LED\n", name );
    fprintf( stderr, "  %s readstat ... ask current status\n", name );
    fprintf( stderr, "  %s writestat 4-chars ... write the first 4 letters of the status string\n", name );
    fprintf( stderr, "  %s in string ... send a text string\n", name );
    fprintf( stderr, "  %s out ... receive data\n", name );
#if ENABLE_TEST
    fprintf( stderr, "  %s test ..... run driver reliability test\n", name );
#endif /* ENABLE_TEST */
}

char buffer[1024];

int main( int argc, char** argv )
{
    const unsigned char rawVid[2] = { USB_CFG_VENDOR_ID };
    const unsigned char rawPid[2] = { USB_CFG_DEVICE_ID };
    // compute VID/PID from usbconfig.h so that there is a central source of information
    const int vid = rawVid[1] * 256 + rawVid[0];
    const int pid = rawPid[1] * 256 + rawPid[0];
    const int timeout = 5000;
    char vendor[]  = { USB_CFG_VENDOR_NAME, 0 };
    char product[] = { USB_CFG_DEVICE_NAME, 0 };
    usb_dev_handle* handle = NULL;
    int cnt;

    if (argc < 2) {/* we need at least one argument */
        usage( argv[0] );
        return 1;
    }
    usb_init();
    /* The following function is in opendevice.c: */
    if (usbOpenDevice( &handle, vid, vendor, pid, product, NULL, NULL, NULL ) != 0) {
        fprintf( stderr, "Could not find USB device \"%s\" with vid=0x%x pid=0x%x\n", product, vid, pid );
        return 1;
    }
    /* Since we use only control endpoint 0, we don't need to choose a
     * configuration and interface. Reading device descriptor and setting a
     * configuration and interface is done through endpoint 0 after all.
     * However, newer versions of Linux require that we claim an interface
     * even for endpoint 0. Enable the following code if your operating system
     * needs it: */
#if 0
    int retries = 1, usbConfiguration = 1, usbInterface = 0;
    if(usb_set_configuration(handle, usbConfiguration) && showWarnings){
        fprintf(stderr, "Warning: could not set configuration: %s\n", usb_strerror());
    }
    /* now try to claim the interface and detach the kernel HID driver on
     * Linux and other operating systems which support the call. */
    while((len = usb_claim_interface(handle, usbInterface)) != 0 && retries-- > 0){
#ifdef LIBUSB_HAS_DETACH_KERNEL_DRIVER_NP
        if(usb_detach_kernel_driver_np(handle, 0) < 0 && showWarnings){
            fprintf(stderr, "Warning: could not detach kernel driver: %s\n", usb_strerror());
        }
#endif
    }
#endif

    if (0) {
    } else if (strcasecmp( argv[1], "setled" ) == 0) {
        int val = 0;
        if (argc >= 3) {
            val = atoi( argv[2] );
        } else {
            fprintf( stderr, "too few arguments\n" );
        }
        cnt = usb_control_msg( handle, USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_OUT, USB_SET_LED, val, 0, buffer, 0, timeout );
        if (cnt < 0) {
            fprintf( stderr, "USB error: %s\n", usb_strerror() );
        }
    } else if (strcasecmp( argv[1], "readstat" ) == 0) {
        cnt = usb_control_msg( handle, USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_IN, USB_STATUS_READ, 0, 0, buffer, sizeof( buffer ), timeout );
        if (cnt < 1) {
            if (cnt < 0) {
                fprintf( stderr, "USB error: %s\n", usb_strerror() );
            } else {
                fprintf( stderr, "only %d bytes received.\n", cnt );
            }
        } else {
            buffer[cnt] = '\0';
            printf( "cnt = %d\n", cnt );
            printf( "data = %s\n", buffer );
        }
    } else if (strcasecmp( argv[1], "writestat" ) == 0) {
        int value = 0;
        int index = 0;
        const char* ptr = argv[2];
        if (argc >= 3) {
            value = ptr[0] + (ptr[1] << 8);
            index = ptr[2] + (ptr[3] << 8);
        } else {
            fprintf( stderr, "too few arguments\n" );
        }
        cnt = usb_control_msg( handle, USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_OUT, USB_STATUS_WRITE, value, index, buffer, 0, timeout );
        if (cnt < 0) {
            fprintf( stderr, "USB error: %s\n", usb_strerror() );
        }
    } else if (strcasecmp( argv[1], "in" ) == 0) {
        if (argc >= 3) {
            cnt = usb_control_msg( handle, USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_OUT, USB_DATA_WRITE, 0, 0, argv[2], strlen( argv[2] ), timeout );
            if (cnt < 0) {
                fprintf( stderr, "USB error: %s\n", usb_strerror() );
            }
        } else {
            fprintf( stderr, "too few arguments\n" );
        }
    } else if (strcasecmp( argv[1], "out" ) == 0) {
        int n = 0;
        cnt = usb_control_msg( handle, USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_IN, USB_DATA_READ, 0, 0, buffer, sizeof( buffer ), timeout );
        printf( "%d bytes received\n", cnt );
        if (cnt < 0) {
            fprintf( stderr, "USB error: %s\n", usb_strerror() );
        }
        while (n < cnt) {
            int k;
            for (k = 0; k < 16 && n < cnt; ++n, ++k) {
                printf( "%c ", buffer[n] );
            }
            printf( "\n" );
        }
#if ENABLE_TEST
    }else if(strcasecmp(argv[1], "test") == 0){
        int i;
        srandomdev();
        for(i = 0; i < 50000; i++){
            int value = random() & 0xffff, index = random() & 0xffff;
            int rxValue, rxIndex;
            if((i+1) % 100 == 0){
                fprintf(stderr, "\r%05d", i+1);
                fflush(stderr);
            }
            cnt = usb_control_msg(handle, USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_IN, CUSTOM_RQ_ECHO, value, index, buffer, sizeof(buffer), 5000);
            if(cnt < 0){
                fprintf(stderr, "\nUSB error in iteration %d: %s\n", i, usb_strerror());
                break;
            }else if(cnt != 4){
                fprintf(stderr, "\nerror in iteration %d: %d bytes received instead of 4\n", i, cnt);
                break;
            }
            rxValue = ((int)buffer[0] & 0xff) | (((int)buffer[1] & 0xff) << 8);
            rxIndex = ((int)buffer[2] & 0xff) | (((int)buffer[3] & 0xff) << 8);
            if(rxValue != value || rxIndex != index){
                fprintf(stderr, "\ndata error in iteration %d:\n", i);
                fprintf(stderr, "rxValue = 0x%04x value = 0x%04x\n", rxValue, value);
                fprintf(stderr, "rxIndex = 0x%04x index = 0x%04x\n", rxIndex, index);
            }
        }
        fprintf(stderr, "\nTest completed.\n");
#endif /* ENABLE_TEST */
    } else {
        usage( argv[0] );
        return 1;
    }
    usb_close( handle );
    return 0;
}
