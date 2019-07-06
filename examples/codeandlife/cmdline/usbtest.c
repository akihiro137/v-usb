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
    fprintf( stderr, "  %s setled (val) .......... set LED\n", name );
    fprintf( stderr, "  %s readstat .............. ask current status\n", name );
    fprintf( stderr, "  %s writestat (4-chars) ... write the first 4 letters of the status string\n", name );
    fprintf( stderr, "  %s send (string) ......... send a text string\n", name );
    fprintf( stderr, "  %s recv .................. receive data\n", name );
    fprintf( stderr, "  %s test .................. run driver reliability test\n", name );
}

int usb_control_msg_simple( usb_dev_handle* handle, int type, int request, int value, int index, char* buf, int size ) {
    const int timeout = 5000;
    const int cnt = usb_control_msg( handle, USB_TYPE_VENDOR | USB_RECIP_DEVICE | type, request, value, index, buf, size, timeout );
    if (cnt < 0) {
        fprintf( stderr, "USB error: %s\n", usb_strerror() );
    }
    return cnt;
}

int usb_control_in( usb_dev_handle* handle, int request, int value, int index, char* buf, int size ) {
    const int cnt = usb_control_msg_simple( handle, USB_ENDPOINT_IN, request, value, index, buf, size );
    printf( "%d bytes received\n", cnt );
    return cnt;
}

int usb_control_out( usb_dev_handle* handle, int request, int value, int index, const char* buf, int size ) {
    const int cnt = usb_control_msg_simple( handle, USB_ENDPOINT_OUT, request, value, index, (char*) buf, size );
    printf( "%d bytes sent\n", cnt );
    return cnt;
}

#define BUFSIZE 512
char buffer[BUFSIZE];
char buffer2[BUFSIZE];

int main( int argc, char** argv )
{
    if (argc < 2) {/* we need at least one argument */
        usage( argv[0] );
        return 1;
    }
    usb_init();
    usb_dev_handle* handle = NULL;
    {// open device
        const unsigned char rawVid[2] = { USB_CFG_VENDOR_ID };
        const unsigned char rawPid[2] = { USB_CFG_DEVICE_ID };
        // compute VID/PID from usbconfig.h so that there is a central source of information
        const int vid = rawVid[1] * 256 + rawVid[0];
        const int pid = rawPid[1] * 256 + rawPid[0];
        char vendor[]  = { USB_CFG_VENDOR_NAME, 0 };
        char product[] = { USB_CFG_DEVICE_NAME, 0 };
        // The following function is in opendevice.c:
        if (usbOpenDevice( &handle, vid, vendor, pid, product, NULL, NULL, NULL ) != 0) {
            fprintf( stderr, "Could not find USB device \"%s\" with vid=0x%x pid=0x%x\n", product, vid, pid );
            return 1;
        }
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
    const char* command = argv[1];
    const char* option  = (argc >= 3) ? argv[2] : "3210";
    if (0) {
    } else if (strcasecmp( command, "setled" ) == 0) {
        const int value = atoi( option );
        usb_control_out( handle, USB_SET_LED, value, 0, buffer, 0 );
    } else if (strcasecmp( command, "getled" ) == 0) {
        const int cnt = usb_control_in( handle, USB_GET_LED, 0, 0, buffer, sizeof( buffer ) );
        if (cnt == 1) {
            printf( "LED = 0x%02x\n", buffer[0] );
        }
    } else if (strcasecmp( command, "readstat" ) == 0) {
        const int cnt = usb_control_in( handle, USB_STATUS_READ, 0, 0, buffer, sizeof( buffer ) );
        if (cnt >= 0) {
            buffer[cnt] = '\0';
            printf( "cnt = %d\n", cnt );
            printf( "stat = %s\n", buffer );
        }
    } else if (strcasecmp( command, "writestat" ) == 0) {
        const int value = option[0] + (option[1] << 8);
        const int index = option[2] + (option[3] << 8);
        usb_control_out( handle, USB_STATUS_WRITE, value, index, buffer, 0 );
    } else if (strcasecmp( command, "send" ) == 0) {
        usb_control_out( handle, USB_DATA_WRITE, 0, 0, option, strlen( option ) );
    } else if (strcasecmp( command, "recv" ) == 0) {
        const int cnt = usb_control_in( handle, USB_DATA_READ, 0, 0, buffer, sizeof( buffer ) );
        for (int n = 0; n < cnt;) {
            for (int k = 0; k < 16 && n < cnt; ++n, ++k) {
                printf( "%c ", buffer[n] );
            }
            printf( "\n" );
        }
    } else if (strcasecmp( command, "test" ) == 0) {
        srandomdev();
        for (int i = 0; i < 10; ++i) {
            printf( "iter = %d\n", i );
            for (int n = 0; n < sizeof( buffer ); ++n) {
                buffer[n] = random() & 0xff;
                buffer2[n] = 0xff;
            }
            const int size = sizeof( buffer );
            int bytes = 0;
            // send
            for (int k = 0; bytes < size; ++k) {
                const int cnt = usb_control_out( handle, USB_DATA_WRITE, 0, 0, buffer + bytes, size - bytes );
                if (cnt < 0) break;
                bytes += cnt;
                printf( "%d: send %d/%d\n", k, bytes, size );
            }
            // recv
            bytes = 0;
            for (int k = 0; bytes < size; ++k) {
                const int cnt = usb_control_in( handle, USB_DATA_READ, 0, 0, buffer2 + bytes, size - bytes );
                if (cnt < 0) break;
                bytes += cnt;
                printf( "%d: recv %d/%d\n", k, bytes, size );
            }
            // check
            for (int k = 0; k < size; ++k) {
                if (buffer2[k] != buffer[k]) {
                    fprintf( stderr, "buffer[%d] %0x2d != buffer2[%d] %0x2d\n", k, buffer[k], k, buffer2[k] );
                }
            }
            printf( "%d: comp done\n", i );
        }
    } else {
        usage( argv[0] );
    }
    usb_close( handle );
    return 0;
}
