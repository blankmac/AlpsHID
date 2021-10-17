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
  - Latitude 7490
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

Experimental ProductIDs:
- 0x121F
- 0x120A
- 0x121E
- 0x1215
- 0x1217
- 0x1207
- 0x1220

# Possibly supported hardware

List of devices that match the ProductIDs above, found using http://linux-hardware.org/

- 0X120B
  - Dell Precision 3520
  - Dell Precision 7520
  - Dell Precision 7720
  - Dell Latitude 5280
  - Dell Latitude 5289
  - Dell Latitude 5480
  - Dell Latitude 5580
  - Dell Latitude 7280
  - Dell Latitude 7380
  - Dell Latitude 7480
- 0X120C
  - HP EliteBook Folio G1
  - HP EliteBook Folio 1040 G3
  - HP EliteBook 1030 G1
  - HP ZBook Studio G3
  - HP ZBook Studio G4
- 0X120D
  - HP Elite x2 1012 G1
- 0X1209
  - ?
- 0X1216
  - HP Elite x2 1012 G2
- 0x121F
  - Dell Precision 3530
  - Dell Latitude 5490
  - Dell Latitude 5491
  - Dell Latitude 5495
  - Dell Latitude 5590
  - Dell Latitude 5591
  - Dell Latitude 7490
- 0x120A
  - HP EliteBook 1040 G4 
  - HP EliteBook x360 1020 G2 
  - HP EliteBook x360 1030 G3
  - Lenovo IdeaPad 330S
  - Lenovo IdeaPad 530S
  - Lenovo Yoga 530
  - Dell Latitude 3480
  - Dell Latitude 3490
  - Dell Latitude 3580
  - Dell Latitude 3590
  - Dell Latitude 5290
  - Dell Latitude 5300
  - Dell Latitude 5300 2-in-1
  - Dell Latitude 5310
  - Dell Latitude 5310 2-in-1
  - Dell Latitude 5420 Rugged
  - Dell Latitude 5424 Rugged
  - Dell Latitude 5490
  - Dell Latitude 5491
  - Dell Inspiron 5570
  - Dell Inspiron 5575
  - Dell Inspiron 5770
  - Dell Latitude 7390
  - Dell Latitude 7490
  - Dell Latitude 7390 2-in-1
- 0x121E
  - Lenovo V130
  - Lenovo V145
  - Lenovo V330
  - Lenovo IdeaPad 130
  - Lenovo IdeaPad 330
- 0x1215
  - HP EliteBook x360 1030 G2 
- 0x1217
  - HP Pro x2 612 G2 
- 0x1207
  - ?
- 0x1220
  - Dell Precision 7530
  - Dell Precision 7540
  - Dell Precision 7730
  - Dell Precision 7740 


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
- [@mbarbierato](https://github.com/mbarbierato) for fixing speed/sensitivity issues on the U1 touchpads.
