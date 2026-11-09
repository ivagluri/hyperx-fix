// Keeps the HyperX Alloy FPS Pro (0951:16d2) in HID report protocol. Its firmware boots in
// boot protocol and disables the lighting Fn layer and media keys there; macOS (unlike
// Linux/Windows) never sends SET_PROTOCOL(report). On macOS 26 the HID driver can also put
// it back into boot protocol after attach without any reconnect, so instead of acting only on
// connect we check GET_PROTOCOL every few seconds and correct it when needed.
#include <stdio.h>
#include <time.h>
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/usb/IOUSBLib.h>
#include <IOKit/IOCFPlugIn.h>

#define VID 0x0951
#define PID 0x16d2
#define INTERVAL 2.0 // seconds between checks

static void logmsg(const char *msg, int iface) {
    char ts[32]; time_t t = time(NULL);
    strftime(ts, sizeof ts, "%Y-%m-%d %H:%M:%S", localtime(&t));
    printf("%s hyperx-fix: interface %d %s\n", ts, iface, msg);
    fflush(stdout);
}

static void check(CFRunLoopTimerRef timer, void *info) {
    CFMutableDictionaryRef m = IOServiceMatching("IOUSBHostDevice");
    int vid = VID, pid = PID;
    CFNumberRef v = CFNumberCreate(NULL, kCFNumberIntType, &vid), p = CFNumberCreate(NULL, kCFNumberIntType, &pid);
    CFDictionarySetValue(m, CFSTR("idVendor"), v);
    CFDictionarySetValue(m, CFSTR("idProduct"), p);
    CFRelease(v); CFRelease(p);
    io_service_t svc = IOServiceGetMatchingService(kIOMainPortDefault, m); // consumes m
    if (!svc) return; // keyboard not connected

    IOCFPlugInInterface **plug; SInt32 score; IOUSBDeviceInterface **dev = NULL;
    kern_return_t kr = IOCreatePlugInInterfaceForService(svc, kIOUSBDeviceUserClientTypeID, kIOCFPlugInInterfaceID, &plug, &score);
    IOObjectRelease(svc);
    if (kr) return;
    (*plug)->QueryInterface(plug, CFUUIDGetUUIDBytes(kIOUSBDeviceInterfaceID), (LPVOID *)&dev);
    (*plug)->Release(plug);
    if (!dev) return;

    if ((*dev)->USBDeviceOpen(dev) == kIOReturnSuccess) {
        for (int i = 0; i < 2; i++) {
            UInt8 proto = 0xff;
            IOUSBDevRequest get = { 0xA1, 0x03, 0, i, 1, &proto, 0 }; // GET_PROTOCOL
            if ((*dev)->DeviceRequest(dev, &get) || proto != 0) continue; // 0 = boot
            IOUSBDevRequest set = { 0x21, 0x0B, 1, i, 0, NULL, 0 };       // SET_PROTOCOL(report)
            logmsg((*dev)->DeviceRequest(dev, &set) ? "was in boot protocol, SET_PROTOCOL(report) failed"
                                                   : "was in boot protocol, switched to report", i);
        }
        (*dev)->USBDeviceClose(dev);
    }
    (*dev)->Release(dev);
}

int main(void) {
    CFRunLoopTimerRef t = CFRunLoopTimerCreate(NULL, CFAbsoluteTimeGetCurrent(), INTERVAL, 0, 0, check, NULL);
    CFRunLoopTimerSetTolerance(t, 0.5);
    CFRunLoopAddTimer(CFRunLoopGetCurrent(), t, kCFRunLoopDefaultMode);
    CFRunLoopRun();
}
