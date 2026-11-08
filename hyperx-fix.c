// Watches for the HyperX Alloy FPS Pro (0951:16d2) and switches it from boot to report
// protocol on every connect. Its firmware boots in HID boot protocol and disables the
// lighting Fn layer there; macOS (unlike Linux/Windows) never sends SET_PROTOCOL(report).
#include <stdio.h>
#include <unistd.h>
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/usb/IOUSBLib.h>
#include <IOKit/IOCFPlugIn.h>

static void fix(io_service_t svc) {
    IOCFPlugInInterface **plug; SInt32 score; IOUSBDeviceInterface **dev = NULL;
    for (int attempt = 0; attempt < 5; attempt++) {
        sleep(1); // let macOS finish attaching its HID drivers first
        if (IOCreatePlugInInterfaceForService(svc, kIOUSBDeviceUserClientTypeID, kIOCFPlugInInterfaceID, &plug, &score)) continue;
        (*plug)->QueryInterface(plug, CFUUIDGetUUIDBytes(kIOUSBDeviceInterfaceID), (LPVOID *)&dev);
        (*plug)->Release(plug);
        if (!dev) continue;
        (*dev)->USBDeviceOpen(dev);
        kern_return_t kr = 0;
        for (int i = 0; i < 2; i++) {
            IOUSBDevRequest r = { 0x21, 0x0B, 0x0001, i, 0, NULL }; // SET_PROTOCOL(report)
            kr |= (*dev)->DeviceRequest(dev, &r);
        }
        (*dev)->USBDeviceClose(dev); (*dev)->Release(dev); dev = NULL;
        printf("hyperx-fix: SET_PROTOCOL(report) %s (attempt %d)\n", kr ? "failed" : "ok", attempt + 1);
        fflush(stdout);
        if (!kr) return;
    }
}

static void added(void *ref, io_iterator_t it) {
    io_service_t s;
    while ((s = IOIteratorNext(it))) { fix(s); IOObjectRelease(s); }
}

int main(void) {
    CFMutableDictionaryRef m = IOServiceMatching("IOUSBHostDevice");
    int vid = 0x0951, pid = 0x16d2;
    CFDictionarySetValue(m, CFSTR("idVendor"), CFNumberCreate(NULL, kCFNumberIntType, &vid));
    CFDictionarySetValue(m, CFSTR("idProduct"), CFNumberCreate(NULL, kCFNumberIntType, &pid));
    IONotificationPortRef port = IONotificationPortCreate(kIOMainPortDefault);
    CFRunLoopAddSource(CFRunLoopGetCurrent(), IONotificationPortGetRunLoopSource(port), kCFRunLoopDefaultMode);
    io_iterator_t it;
    IOServiceAddMatchingNotification(port, kIOFirstMatchNotification, m, added, NULL, &it);
    added(NULL, it); // handle already-connected keyboard, arm notification
    CFRunLoopRun();
}
