/*
We use VID/PID 0x046D/0xC00E which is taken from a Logitech mouse. Don't
publish any hardware using these IDs! This is for demonstration only!
*/
#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>  /* for sei() */
#include <util/delay.h>     /* for _delay_ms() */

#include <avr/pgmspace.h>   /* required by usbdrv.h */
#include "usbdrv.h"
#include "oddebug.h"        /* This is also an example for using debug macros */

#include "requests.h"

#if 0
PROGMEM const char usbHidReportDescriptor[52] = { /* USB report descriptor, size must match usbconfig.h */
    0x05, 0x01,                    // USAGE_PAGE (Generic Desktop)
    0x09, 0x02,                    // USAGE (Mouse)
    0xa1, 0x01,                    // COLLECTION (Application)
    0x09, 0x01,                    //   USAGE (Pointer)
    0xA1, 0x00,                    //   COLLECTION (Physical)
    0x05, 0x09,                    //     USAGE_PAGE (Button)
    0x19, 0x01,                    //     USAGE_MINIMUM
    0x29, 0x03,                    //     USAGE_MAXIMUM
    0x15, 0x00,                    //     LOGICAL_MINIMUM (0)
    0x25, 0x01,                    //     LOGICAL_MAXIMUM (1)
    0x95, 0x03,                    //     REPORT_COUNT (3)
    0x75, 0x01,                    //     REPORT_SIZE (1)
    0x81, 0x02,                    //     INPUT (Data,Var,Abs)
    0x95, 0x01,                    //     REPORT_COUNT (1)
    0x75, 0x05,                    //     REPORT_SIZE (5)
    0x81, 0x03,                    //     INPUT (Const,Var,Abs)
    0x05, 0x01,                    //     USAGE_PAGE (Generic Desktop)
    0x09, 0x30,                    //     USAGE (X)
    0x09, 0x31,                    //     USAGE (Y)
    0x09, 0x38,                    //     USAGE (Wheel)
    0x15, 0x81,                    //     LOGICAL_MINIMUM (-127)
    0x25, 0x7F,                    //     LOGICAL_MAXIMUM (127)
    0x75, 0x08,                    //     REPORT_SIZE (8)
    0x95, 0x03,                    //     REPORT_COUNT (3)
    0x81, 0x06,                    //     INPUT (Data,Var,Rel)
    0xC0,                          //   END_COLLECTION
    0xC0,                          // END COLLECTION
};
/* This is the same report descriptor as seen in a Logitech mouse. The data
 * described by this descriptor consists of 4 bytes:
 *      .  .  .  .  . B2 B1 B0 .... one byte with mouse button states
 *     X7 X6 X5 X4 X3 X2 X1 X0 .... 8 bit signed relative coordinate x
 *     Y7 Y6 Y5 Y4 Y3 Y2 Y1 Y0 .... 8 bit signed relative coordinate y
 *     W7 W6 W5 W4 W3 W2 W1 W0 .... 8 bit signed relative coordinate wheel
 */
typedef struct{
    uchar   buttonMask;
    char    dx;
    char    dy;
    char    dWheel;
}report_t;

static report_t reportBuffer;
static int      sinus = 7 << 6, cosinus = 0;
static uchar    idleRate;   /* repeat rate for keyboards, never used for mice */

/* ------------------------------------------------------------------------- */

usbMsgLen_t usbFunctionSetup(uchar data[8])
{
usbRequest_t    *rq = (void *)data;

    /* The following requests are never used. But since they are required by
     * the specification, we implement them in this example.
     */
    if((rq->bmRequestType & USBRQ_TYPE_MASK) == USBRQ_TYPE_CLASS){    /* class request type */
        DBG1(0x50, &rq->bRequest, 1);   /* debug output: print our request */
        if(rq->bRequest == USBRQ_HID_GET_REPORT){  /* wValue: ReportType (highbyte), ReportID (lowbyte) */
            /* we only have one report type, so don't look at wValue */
            usbMsgPtr = (void *)&reportBuffer;
            return sizeof(reportBuffer);
        }else if(rq->bRequest == USBRQ_HID_GET_IDLE){
            usbMsgPtr = &idleRate;
            return 1;
        }else if(rq->bRequest == USBRQ_HID_SET_IDLE){
            idleRate = rq->wValue.bytes[1];
        }
    }else{
        /* no vendor specific requests implemented */
    }
    return 0;   /* default for not implemented requests: return no data back to host */
}
#endif

#define LED_MASK 0x1f
#define LED_BIT  0x20

// for USB_DATA_WRITE
static uchar statusBuf[16] = "Hello, USB!";
static uchar dataBuf[512];
static uchar ledBuf[1];

static usbMsgLen_t dataLength = 0;
static usbMsgLen_t dataTransferred = 0;

USB_PUBLIC usbMsgLen_t usbFunctionSetup( uchar data[8] )
{
    usbRequest_t* rq = (usbRequest_t*) data;// cast data to correct type
    switch (rq->bRequest) {// custom command is in the bRequest field
        case USB_SET_LED:
            PORTC = (PORTC & ~LED_MASK) | (rq->wValue.bytes[0] & LED_MASK);
            return 0;
        case USB_GET_LED:
            ledBuf[0] = PORTC & LED_MASK;
            usbMsgPtr = ledBuf; // tell the driver which data to return
            return 1;           // tell the driver to send 1 byte
        case USB_STATUS_READ:// send data to PC
            usbMsgPtr = statusBuf;
            return sizeof( statusBuf );
        case USB_STATUS_WRITE:// modify reply buffer
            statusBuf[0] = rq->wValue.bytes[0];
            statusBuf[1] = rq->wValue.bytes[1];
            statusBuf[2] = rq->wIndex.bytes[0];
            statusBuf[3] = rq->wIndex.bytes[1];
            return 0;
        case USB_DATA_WRITE:// receive data from PC
        case USB_DATA_READ:// send data to PC
            dataTransferred = 0;
            dataLength = rq->wLength.word;
            if (dataLength > sizeof( dataBuf )) {// limit to buffer size
                dataLength = sizeof( dataBuf );
            }
            return USB_NO_MSG;//usbFunctionWrite/Read will be called now
    }
    return 0;// should not get here
}

USB_PUBLIC uchar usbFunctionWrite( uchar* data, uchar len )
{
    uchar i;
    for (i = 0; dataTransferred < dataLength && i < len; ++i, ++dataTransferred) {
        dataBuf[dataTransferred] = data[i];
    }
    return (dataTransferred == dataLength) ? 1 : 0;// 1 if we received it all, 0 if not
}

USB_PUBLIC uchar usbFunctionRead( uchar* data, uchar len )
{
    uchar i;
    for (i = 0; dataTransferred < dataLength && i < len; ++i, ++dataTransferred) {
        data[i] = dataBuf[dataTransferred];
    }
    return i;// the amount of bytes written
}

int __attribute__((noreturn)) main( void )
{
    uchar i;
    uchar led = 0;
    short cnt = 0;
    wdt_enable( WDTO_1S );
    /* If you don't use the watchdog, replace the call above with a wdt_disable().
     * On newer devices, the status of the watchdog (on/off, period) is PRESERVED
     * OVER RESET!
     */
    /* RESET status: all port bits are inputs without pull-up.
     * That's the way we need D+ and D-. Therefore we don't need any
     * additional hardware initialization.
     */
    odDebugInit();
    DBG1( 0x00, 0, 0 );// debug output: main starts
    usbInit();
    usbDeviceDisconnect();// enforce re-enumeration, do this while interrupts are disabled!
    for (i = 0; i < 255; ++i) {// fake USB disconnect for > 250 ms
        wdt_reset();
        _delay_ms( 1 );
    }
    usbDeviceConnect();
    sei();// enable interrupts after re-enumeration
    DBG1( 0x01, 0, 0 );// debug output: main loop starts
	DDRC |= LED_MASK | LED_BIT; // LEDs on PC*
    while (1) {// main event loop
        DBG1( 0x02, 0, 0 );// debug output: main loop iterates
        wdt_reset();
        usbPoll();
        // if (usbInterruptIsReady()) {
        //     // called after every poll of the interrupt endpoint
        //     //advanceCircleByFixedAngle();
        //     DBG1( 0x03, 0, 0 );// debug output: interrupt report prepared
        //     usbSetInterrupt( (void*) &reportBuffer, sizeof( reportBuffer ) );
        // }
        cnt++;
        if (cnt == 0) {
            if (led == 0) {
                led = 1;
                PORTC |= LED_BIT;// turn LED on
            } else {
                led = 0;
                PORTC &= ~LED_BIT;// turn LED off
            }
        }
    }
#if 0
	while (1) {
		PORTC |= 1; // Turn LED on
		_delay_ms( 500 );
		PORTC &= ~1; // Turn LED off
		_delay_ms( 500 );
	}
#endif
}
