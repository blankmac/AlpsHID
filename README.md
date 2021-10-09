# AlpsT4USB

This is a satellite kext which uses VoodooI2C's multitouch engine to bring native multitouch to the Alps
T4 USB touchpad found on the Elite X2 1012 G1 and G2 devices.

# Installation Considerations

~Currently this kext only works when installed to /Library/Extensions.  VoodooI2C and VoodooI2CHID must 
also be installed to /Library/Extensions.~

Should work when injected via Clover or OC, but you do need VoodooI2C / VoodooI2CHID.

# Experimental

~The latest release of this kext *should* also work for I2C Alps touchpad devices with the following device IDs - 0x1209, 0x120b, 0x120c but it is currently untested.  Since it only subclasses the Event Service, you will always need to install VoodooI2C / VoodooI2CHID to instantiate the hid device.  If you have an I2C Alps touchpad and try the kext, please report in the VoodooI2C gitter chat whether or not it works for you.~

Ok, so that doesn't work at all unfortunately.  The code as written isn't able to kick an I2C Alps touchpad into precision mode.  The best way forward is to write a standalone kext based on one of VoodooI2C's current satellites (ie - Elan, etc).

# Building AlpsT4USB

To build --
```
git clone https://github.com/blankmac/AlpsT4USB.git
cd AlpsT4USB
git submodule init && git submodule update
```

Then add the MacKernel SDK.
```
git clone https://github.com/acidanthera/MacKernelSDK.git
```

Open the main project in Xcode and build away.  :)

# Credits
This code is derived and adapted from VoodooI2CHID's Multitouch Event Driver and Precision
Touchpad Event Driver (https://github.com/alexandred/VoodooI2C) and the Linux kernel driver
for the alps t4 touchpad (https://github.com/torvalds/linux/blob/master/drivers/hid/hid-alps.c)
- [@kprinssu](https://github.com/kprinssu) for VoodooInput and it's integration into AlpsT4USB.  Thanks!
- the [acidanthera team](https://github.com/acidanthera) for the MacKernel SDK.
