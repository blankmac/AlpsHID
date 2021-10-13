# AlpsHID

This is a satellite kext which uses VoodooI2C's multitouch engine to bring native multitouch to the Alps touchpad using the HID protocol.
Trackpoint(StickPointer) is supported aswell, including the buttons. 

# Installation Considerations

Should work when injected via Clover or OC, but you do need VoodooI2C / VoodooI2CHID.
Use the supplied VoodooI2CHID as it contains a fix not yet implemented in the main version.

# Supported hardware

- Dell
  - Latitude 5280
  - Latitude 5288
  - Latitude 5480
  - Latitude 5488
  - Latitude 5580
  - Latitude 7280
  - Latitude 7380
  - Latitude 7480
  - Precision 3520
- HP
  - ZBook G3 studio
  - Elite X2 1012 G1
  - Elite X2 1012 G2

Should work for devices with VendorID:0x44e, and one of the following ProductID's

ProductID:
- 0X120B  (confirmed)
- 0X120C  (confirmed)
- 0X120D
- 0X1209
- 0X1216

# Building AlpsHID

To build --
```
git clone https://github.com/blankmac/AlpsHID.git
cd AlpsHID
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
for the alps HID touchpad (https://github.com/torvalds/linux/blob/master/drivers/hid/hid-alps.c)
- [@kprinssu](https://github.com/kprinssu) for VoodooInput and it's integration into AlpsHID.  Thanks!
- the [acidanthera team](https://github.com/acidanthera) for the MacKernel SDK.
- [@juico](https://github.com/juico) for fixing I2C and U1 support.
- [@lorys89](https://github.com/Lorys89) [@svilen88](https://github.com/Svilen88) [@QuanTrieuPCYT](https://github.com/QuanTrieuPCYT) for testing on the U1 touchpads
