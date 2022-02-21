/*This code is derived and adapted from VoodooI2CHID's Multitouch Event Driver and Precision
 Touchpad Event Driver (https://github.com/alexandred/VoodooI2C) and the Linux kernel driver
 for the alps t4 touchpad (https://github.com/torvalds/linux/blob/master/drivers/hid/hid-alps.c) */


#include "AlpsHID.hpp"


#define super IOHIDEventService
OSDefineMetaClassAndStructors(AlpsHIDEventDriver, IOHIDEventService);

void AlpsHIDEventDriver::t4_device_init() {
    
    ready = false;
    
    UInt8 tmp = '\0', sen_line_num_x, sen_line_num_y;
    alps_dev pri_data;
    IOReturn ret = kIOReturnSuccess;
    
    if (hid_interface->getProductID() == HID_PRODUCT_ID_T4_BTNLESS) {
        ret = t4_read_write_register(T4_PRM_ID_CONFIG_3, &tmp, 0, true);
        if (ret!=kIOReturnSuccess) {
            IOLog("failed T4_PRM_ID_CONFIG_3 (%d)\n", ret);
            goto exit;
        }
        sen_line_num_x = 16 + ((tmp & 0x0F) | (tmp & 0x08 ? 0xF0 : 0));
        sen_line_num_y = 12 + (((tmp & 0xF0) >> 4) | (tmp & 0x80 ? 0xF0 : 0));
        ret = t4_read_write_register(PRM_SYS_CONFIG_1, &tmp, 0, true);
        if (ret!=kIOReturnSuccess) {
            IOLog("failed PRM_SYS_CONFIG_1 (%d)\n", ret);
            goto exit;
        }
    } else {
        sen_line_num_x = 20;
        sen_line_num_y = 12;
    }
    
    pri_data.x_max = sen_line_num_x * T4_COUNT_PER_ELECTRODE;
    pri_data.x_min = T4_COUNT_PER_ELECTRODE;
    pri_data.y_max = sen_line_num_y * T4_COUNT_PER_ELECTRODE;
    pri_data.y_min = T4_COUNT_PER_ELECTRODE;
    pri_data.x_active_len_mm = pri_data.y_active_len_mm = 0;
    pri_data.btn_cnt = 1;
    
    tmp |= 0x02;
    
    ret = t4_read_write_register(PRM_SYS_CONFIG_1, NULL, tmp, false);
    if (ret!=kIOReturnSuccess) {
        IOLog("failed PRM_SYS_CONFIG_1 (%d)\n", ret);
        goto exit;
    }
    
    ret = t4_read_write_register(T4_PRM_FEED_CONFIG_1, NULL, T4_I2C_ABS, false);
    if (ret!=kIOReturnSuccess) {
        IOLog("failed T4_PRM_FEED_CONFIG_1 (%d)\n", ret);
        goto exit;
    }
    
    ret = t4_read_write_register(T4_PRM_FEED_CONFIG_4, NULL, T4_FEEDCFG4_ADVANCED_ABS_ENABLE, false);
    if (ret!=kIOReturnSuccess) {
        IOLog("failed T4_PRM_FEED_CONFIG_4 (%d)\n", ret);
        goto exit;
    }
    pri_data.max_fingers = 5;

    setProperty(VOODOO_INPUT_LOGICAL_MAX_X_KEY, pri_data.x_max, 32);
    setProperty(VOODOO_INPUT_LOGICAL_MAX_Y_KEY, pri_data.y_max, 32);
    
    setProperty(VOODOO_INPUT_PHYSICAL_MAX_X_KEY, 10240, 32);
    setProperty(VOODOO_INPUT_PHYSICAL_MAX_Y_KEY, 6140, 32);
    
    ready = true;
    return;
    
exit:
    ready = false;
    return;
}

void AlpsHIDEventDriver::handleInterruptReport(AbsoluteTime timestamp, IOMemoryDescriptor *report, IOHIDReportType report_type, UInt32 report_id) {
    
    switch (dev_type) {
        case T4:
            t4_raw_event(timestamp, report, report_type, report_id);
            break;
        case U1:
            u1_raw_event(timestamp, report, report_type, report_id);
            break;
    }
}

bool AlpsHIDEventDriver::start(IOService* provider) {
    if (!super::start(provider))
        return false;
    
    work_loop = this->getWorkLoop();
    
    if (!work_loop)
        return false;
    
    work_loop->retain();
    
    command_gate = IOCommandGate::commandGate(this);
    if (!command_gate) {
        return false;
    }
    work_loop->addEventSource(command_gate);
    
    max_after_typing = 500000000;
    key_time = 0;
    
    // Read QuietTimeAfterTyping configuration value (if available)
    OSNumber* quietTimeAfterTyping = OSDynamicCast(OSNumber, getProperty("QuietTimeAfterTyping"));
    
    if (quietTimeAfterTyping != NULL)
        max_after_typing = quietTimeAfterTyping->unsigned64BitValue() * 1000000;
    
    setProperty("VoodooI2CServices Supported", kOSBooleanTrue);
    
    return true;
}

bool AlpsHIDEventDriver::handleStart(IOService* provider) {
    
    hid_interface = OSDynamicCast(IOHIDInterface, provider);
    
    if (!hid_interface)
        return false;

    if (hid_interface->getVendorID() != ALPS_VENDOR && hid_interface->getVendorID() != CIRQUE_VENDOR) {
        hid_interface = NULL;
        return false;
    }

    switch (hid_interface->getProductID()) {
            
        case HID_PRODUCT_ID_T4_USB:
        case HID_PRODUCT_ID_T4_USB_2:
        case HID_PRODUCT_ID_G1:
        case HID_PRODUCT_ID_T4_BTNLESS:
            dev_type = T4;
            break;
        case HID_PRODUCT_ID_U1:
        case HID_PRODUCT_ID_U1_OLDMAN:
        case HID_PRODUCT_ID_U1_UNICORN_LEGACY:
        case HID_PRODUCT_ID_U1_DUAL:
        case HID_PRODUCT_ID_U1_DUAL_PTP:
        case HID_PRODUCT_ID_U1_PTP_1:
        case HID_PRODUCT_ID_U1_PTP_2:
        case HID_PRODUCT_ID_U1_DUAL_3BTN_PTP:
        case HID_PRODUCT_ID_U1_1222:
            dev_type = U1;
            break;
            
        default:
        {   hid_interface = NULL;
            return false;
        }
     }
    
    
    if (!hid_interface->open(this, 0, OSMemberFunctionCast(IOHIDInterface::InterruptReportAction, this, &AlpsHIDEventDriver::handleInterruptReport), NULL))
        return false;
    
    name = (char*) hid_interface->getProduct();
    
    PMinit();
    
    registerPowerDriver(this, VoodooI2CIOPMPowerStates, kVoodooI2CIOPMNumberPowerStates);

    hid_interface->joinPMtree(this);
    
    switch (dev_type) {
        case T4:
            t4_device_init();
            break;
        case U1:
            u1_device_init();
            break;
    }
    
    publishMultitouchInterface();
    
    return true;
}


IOReturn AlpsHIDEventDriver::setPowerState(unsigned long whichState, IOService* whatDevice) {
    if (whatDevice != this)
        return kIOReturnInvalid;
    if (!whichState) {
        if (awake)
            awake = false;
        IOLog("%s::%s Going to sleep\n", getName(), name);
    } else {
        if (!awake) {
            IOSleep(10);
            
            IOLog("%s::%s Awake, putting device in precision mode\n", getName(), name);
            switch(dev_type){
                case T4:
                    t4_device_init();
                    break;
                case U1:
                    u1_device_init();
                    break;
            }
            
            
            
            awake = true;
        }
    }
    return kIOPMAckImplied;
}


UInt16 AlpsHIDEventDriver::t4_calc_check_sum(UInt8 *buffer, unsigned long offset, unsigned long length)
{
    UInt16 sum1 = 0xFF, sum2 = 0xFF;
    unsigned long i = 0;
    
    if (offset + length >= 50)
        return 0;
    
    while (length > 0) {
        unsigned long tlen = length > 20 ? 20 : length;
        
        length -= tlen;
        
        do {
            sum1 += buffer[offset + i];
            sum2 += sum1;
            i++;
        } while (--tlen > 0);
        
        sum1 = (sum1 & 0xFF) + (sum1 >> 8);
        sum2 = (sum2 & 0xFF) + (sum2 >> 8);
    }
    
    sum1 = (sum1 & 0xFF) + (sum1 >> 8);
    sum2 = (sum2 & 0xFF) + (sum2 >> 8);
    
    return(sum2 << 8 | sum1);
}

IOReturn AlpsHIDEventDriver::publishMultitouchInterface() {
    setProperty("IOFBTransform", 0ull, 32);
    setProperty("VoodooInputSupported", kOSBooleanTrue);
    
    return kIOReturnSuccess;
}

void AlpsHIDEventDriver::handleStop(IOService* provider) {
    work_loop->removeEventSource(command_gate);
    OSSafeReleaseNULL(command_gate);
    OSSafeReleaseNULL(work_loop);

    PMstop();
    super::handleStop(provider);
}

IOReturn AlpsHIDEventDriver::message(UInt32 type, IOService* provider, void* argument)
{
    switch (type)
    {
        case kKeyboardKeyPressTime:
        {
            //  Remember last time key was pressed
            key_time = *((uint64_t*)argument);
#if DEBUG
            IOLog("%s::keyPressed = %llu\n", getName(), key_time);
#endif
            break;
        }
    }
    
    return kIOReturnSuccess;
}

bool AlpsHIDEventDriver::init(OSDictionary *properties) {
    if (!super::init(properties)) {
        return false;
    }

    awake = true;
    
    return true;
}


bool AlpsHIDEventDriver::didTerminate(IOService* provider, IOOptionBits options, bool* defer) {
    if (hid_interface)
        hid_interface->close(this);
    hid_interface = NULL;
    
    return super::didTerminate(provider, options, defer);
}

void AlpsHIDEventDriver::t4_raw_event(AbsoluteTime timestamp, IOMemoryDescriptor *report, IOHIDReportType report_type, UInt32 report_id) {
    
    uint64_t now_abs;
    clock_get_uptime(&now_abs);
    uint64_t now_ns;
    absolutetime_to_nanoseconds(now_abs, &now_ns);
    
    // Ignore touchpad interaction(s) shortly after typing
    if (now_ns - key_time < max_after_typing)
        return;
    
    if (!ready || !report)
        return;
    
    if (report_type != kIOHIDReportTypeInput)
        return;
    
    if (report_id != T4_INPUT_REPORT_ID)
        return;
    
    t4_input_report reportData;
    
    unsigned int x, y, z;
    
    report->readBytes(0, &reportData, T4_INPUT_REPORT_LEN);
    
    int contactCount = 0;
    for (int i = 0; i < MAX_TOUCHES; i++) {
        VoodooInputTransducer* transducer = &inputMessage.transducers[i];

        transducer->fingerType = (MT2FingerType) (kMT2FingerTypeIndexFinger + (i % 4));
        transducer->type = VoodooInputTransducerType::FINGER;
        transducer->secondaryId = i;
        
        x = reportData.contact[i].x_hi << 8 | reportData.contact[i].x_lo;
        y = reportData.contact[i].y_hi << 8 | reportData.contact[i].y_lo;
        y = 3060 - y + 255;
        z = (reportData.contact[i].palm < 0x80 && reportData.contact[i].palm > 0) * 62;
        bool contactValid = z;
        //When palm is detected just dont send anything
        if(reportData.contact[i].palm & 0x80){
            return;
        }
        transducer->isValid = contactValid;
        transducer->timestamp = timestamp;
        transducer->supportsPressure = true;
        
        if (contactValid) {
            transducer->isTransducerActive = true;
            transducer->secondaryId = i;
            
            transducer->previousCoordinates = transducer->currentCoordinates;
            transducer->currentCoordinates.x = x;
            transducer->currentCoordinates.y = y;
            transducer->currentCoordinates.pressure = z > 105 ? 255 : 0;
            transducer->currentCoordinates.width = z / 2;
            
            transducer->isPhysicalButtonDown = reportData.button;
           
            contactCount += 1;
        } else {
            transducer->isTransducerActive =  false;
            transducer->secondaryId = i;
            transducer->currentCoordinates = transducer->previousCoordinates;
            transducer->isPhysicalButtonDown = reportData.button;
        }
    }
    
    inputMessage.contact_count = MAX_TOUCHES;
    inputMessage.timestamp = timestamp;
    
    if (contactCount >= 4 || inputMessage.transducers->isPhysicalButtonDown) {
        // simple thumb detection: to find the lowest finger touch in the vertical direction.
        UInt32 y_max = 0;
        int thumb_index = 0;
        for (int i = 0; i < contactCount; i++) {
            VoodooInputTransducer* inputTransducer = &inputMessage.transducers[i];
            if (inputTransducer->isValid && inputTransducer->currentCoordinates.y >= y_max) {
                y_max = inputTransducer->currentCoordinates.y;
                thumb_index = i;
            }
        }
        inputMessage.transducers[thumb_index].fingerType = kMT2FingerTypeThumb;
    }

    
    
    
    super::messageClient(kIOMessageVoodooInputMessage, voodooInputInstance, &inputMessage, sizeof(VoodooInputEvent));
}

void AlpsHIDEventDriver::u1_raw_event(AbsoluteTime timestamp, IOMemoryDescriptor *report, IOHIDReportType report_type, UInt32 report_id) {
    
    uint64_t now_abs;
    clock_get_uptime(&now_abs);
    uint64_t now_ns;
    absolutetime_to_nanoseconds(now_abs, &now_ns);
#if DEBUG
    IOLog("%s::%s Receiving report: %u\n",getName(),name,report_id);
#endif
    
    // Ignore touchpad interaction(s) shortly after typing
    if (now_ns - key_time < max_after_typing)
        return;
    
    if (!ready || !report)
        return;
    
    if (report_type != kIOHIDReportTypeInput)
        return;
    
    unsigned int x, y, z;
    
    if(report_id==U1_ABSOLUTE_REPORT_ID || report_id==U1_ABSOLUTE_REPORT_ID_SECD) {
        
        u1_input_report reportData;
        report->readBytes(0, &reportData, report->getLength());
#if DEBUG
        IOLog("%s::%s Getting touched\n",getName(),name);
#endif
        int contactCount = 0;
        for (int i = 0; i < MAX_TOUCHES; i++) {
            VoodooInputTransducer* transducer = &inputMessage.transducers[i];
            
            transducer->type = VoodooInputTransducerType::FINGER;
            transducer->fingerType = (MT2FingerType) (kMT2FingerTypeIndexFinger + (i % 4));
            transducer->secondaryId = i;
            
            x = reportData.contact[i].x_hi << 8 | reportData.contact[i].x_lo;
            y = reportData.contact[i].y_hi << 8 | reportData.contact[i].y_lo;
            z = reportData.contact[i].z & 0x7F;
            bool contactValid = z;
            transducer->isValid = contactValid;
            transducer->timestamp = timestamp;
            transducer->supportsPressure = true;
            
            if (contactValid) {
                transducer->isTransducerActive = true;
                transducer->secondaryId = i;
#if DEBUG
                IOLog("%s::%s Finger %i at(%u,%u)\n", getName(), name, i, x, y);
#endif
                transducer->previousCoordinates = transducer->currentCoordinates;
                transducer->currentCoordinates.x = x;
                transducer->currentCoordinates.y = y;
                transducer->currentCoordinates.pressure = z > 105 ? 255 : 0;
                transducer->currentCoordinates.width = z / 2;
                
                transducer->isPhysicalButtonDown = reportData.buttons;
                
                contactCount += 1;
            } else {
                transducer->isTransducerActive =  false;
                transducer->secondaryId = i;
                transducer->currentCoordinates = transducer->previousCoordinates;
                transducer->isPhysicalButtonDown = reportData.buttons;
            }
            
            
        }
        //send button commands as mouse input.
        dispatchRelativePointerEvent(timestamp, 0, 0, reportData.buttons & 0x07);
        
        inputMessage.contact_count = contactCount;
        inputMessage.timestamp = timestamp;
        
        if (contactCount >= 4 || inputMessage.transducers->isPhysicalButtonDown) {
            // simple thumb detection: to find the lowest finger touch in the vertical direction.
            UInt32 y_max = 0;
            int thumb_index = 0;
            for (int i = 0; i < contactCount; i++) {
                VoodooInputTransducer* inputTransducer = &inputMessage.transducers[i];
                if (inputTransducer->isValid && inputTransducer->currentCoordinates.y >= y_max) {
                    y_max = inputTransducer->currentCoordinates.y;
                    thumb_index = i;
                }
            }
            inputMessage.transducers[thumb_index].fingerType = kMT2FingerTypeThumb;
        }
        
        
        super::messageClient(kIOMessageVoodooInputMessage, voodooInputInstance, &inputMessage, sizeof(VoodooInputEvent));
        return;
    }
    
    if(report_id== U1_SP_ABSOLUTE_REPORT_ID){
        u1_sp_input_report reportData;
        report->readBytes(0, &reportData, report->getLength());
#if DEBUG
        IOLog("%s::%s Getting poked dx:%i, dy:%i \n", getName(), name, reportData.x, reportData.y);
#endif
        dispatchRelativePointerEvent(timestamp, reportData.x/4,reportData.y/4, reportData.buttons & 0x07);
        return;
    }
    return;
    
}

bool AlpsHIDEventDriver::u1_device_init() {
    
    ready = false;
    
    UInt8 tmp, dev_ctrl, sen_line_num_x, sen_line_num_y;
    UInt8 pitch_x, pitch_y, resolution;
    alps_dev pri_data;
    IOReturn ret = kIOReturnSuccess;
    
    /* Device initialization */
    ret = u1_read_write_register(ADDRESS_U1_DEV_CTRL_1, &dev_ctrl, 0, true);
    if (ret != kIOReturnSuccess) {
        IOLog("%s::%s Could not read device mode\n", getName(), name);
        goto exit;
    }
    
    dev_ctrl &= ~U1_DISABLE_DEV;
    dev_ctrl |= U1_TP_ABS_MODE;
    
    u1_read_write_register(ADDRESS_U1_DEV_CTRL_1, NULL, dev_ctrl, false);
    if (ret != kIOReturnSuccess) {
        IOLog("%s::%s Could not put device in absolute mode\n", getName(), name);
        goto exit;
    }

    u1_read_write_register(ADDRESS_U1_NUM_SENS_X, &sen_line_num_x, 0, true);
    if (ret != kIOReturnSuccess) {
        IOLog("%s::%s Could not read sen_line_num_x\n", getName(), name);
        goto exit;
    }
    
    u1_read_write_register(ADDRESS_U1_NUM_SENS_Y, &sen_line_num_y, 0, true);
    if (ret != kIOReturnSuccess) {
        IOLog("%s::%s Could not read sen_line_num_y\n", getName(), name);
        goto exit;
    }
    
    u1_read_write_register(ADDRESS_U1_PITCH_SENS_X, &pitch_x, 0, true);
    if (ret != kIOReturnSuccess) {
        IOLog("%s::%s Could not read pitch_sens_x\n", getName(), name);
        goto exit;
    }
    
    u1_read_write_register(ADDRESS_U1_PITCH_SENS_Y, &pitch_y, 0, true);
    if (ret != kIOReturnSuccess) {
        IOLog("%s::%s Could not read pitch_sens_y\n", getName(), name);
        goto exit;
    }
    
    u1_read_write_register(ADDRESS_U1_RESO_DWN_ABS, &resolution, 0, true);
    if (ret != kIOReturnSuccess) {
        IOLog("%s::%s Could not read absolute mode resolution\n", getName(), name);
        goto exit;
    }
    
    pri_data.x_active_len_mm = (pitch_x * (sen_line_num_x - 1));
    pri_data.y_active_len_mm = (pitch_y * (sen_line_num_y - 1));
    
    pri_data.x_max = (resolution << 2) * (sen_line_num_x - 1);
    pri_data.x_min = 1;
    pri_data.y_max = (resolution << 2) * (sen_line_num_y - 1);
    pri_data.y_min = 1;
    
    u1_read_write_register(ADDRESS_U1_PAD_BTN, &tmp, 0, true);
    if (ret != kIOReturnSuccess) {
        IOLog("%s::%s Could not read button count\n", getName(), name);
        goto exit;
    }

    if ((tmp & 0x0F) == (tmp & 0xF0) >> 4) {
        pri_data.btn_cnt = (tmp & 0x0F);
    } else {
        /* Button pad */
        pri_data.btn_cnt = 1;
    }
    
    u1_read_write_register(ADDRESS_U1_DEVICE_TYP, &tmp, 0, true);
    if (ret != kIOReturnSuccess) {
        IOLog("%s::%s Could not read button count\n", getName(), name);
        goto exit;
    }
    if(tmp & U1_DEVTYPE_SP_SUPPORT)
    {
        dev_ctrl |= U1_SP_ABS_MODE;
        u1_read_write_register(ADDRESS_U1_DEV_CTRL_1, 0, dev_ctrl, false);
        u1_read_write_register(ADDRESS_U1_SP_BTN, &pri_data.sp_btn_info, 0, true);
        pri_data.has_sp = 1;
    }
    
    setProperty(VOODOO_INPUT_LOGICAL_MAX_X_KEY, pri_data.x_max, 32);
    setProperty(VOODOO_INPUT_LOGICAL_MAX_Y_KEY, pri_data.y_max, 32);
       
    setProperty(VOODOO_INPUT_PHYSICAL_MAX_X_KEY, pri_data.x_active_len_mm * 10, 32);
    setProperty(VOODOO_INPUT_PHYSICAL_MAX_Y_KEY, pri_data.y_active_len_mm * 10, 32);

    ready = true;
    return true;
    
exit:
    ready = false;
    return false;
    
}

IOReturn AlpsHIDEventDriver::u1_read_write_register(UInt32 address, UInt8 *read_val, UInt8 write_val, bool read_flag) {
    
    UInt8 check_sum;
    UInt8 input[U1_FEATURE_REPORT_LEN] = {};
    UInt8 readbuf[U1_FEATURE_REPORT_LEN] = {};
    IOReturn ret = kIOReturnSuccess;
    
    
    input[0] = U1_FEATURE_REPORT_ID;
    if (read_flag) {
        input[1] = U1_CMD_REGISTER_READ;
        input[6] = 0x00;
    } else {
        input[1] = U1_CMD_REGISTER_WRITE;
        input[6] = write_val;
    }
    
    put_unaligned_le32(address, input + 2);
    
    /* Calculate the checksum */
    check_sum = U1_FEATURE_REPORT_LEN_ALL;
    for (int i = 0; i < U1_FEATURE_REPORT_LEN - 1; i++)
        check_sum += input[i];
    
    input[7] = check_sum;
    
    
    
    OSData* input_updated = OSData::withBytes(input, U1_FEATURE_REPORT_LEN);
    IOBufferMemoryDescriptor* report = IOBufferMemoryDescriptor::withBytes(input_updated->getBytesNoCopy(0, U1_FEATURE_REPORT_LEN), input_updated->getLength(), kIODirectionInOut);
    
    input_updated->release();
    
    ret = hid_interface->setReport(report, kIOHIDReportTypeFeature, U1_FEATURE_REPORT_ID);
    
    if (read_flag) {
        
        ret = hid_interface->getReport(report, kIOHIDReportTypeFeature, U1_FEATURE_REPORT_ID);
        
        report->readBytes(0, &readbuf, U1_FEATURE_REPORT_LEN);
        
        *read_val = readbuf[6];
        
    }
    
    report->release();
    
    return ret;
}

IOReturn AlpsHIDEventDriver::t4_read_write_register(UInt32 address, UInt8 *read_val, UInt8 write_val, bool read_flag) {
    
    UInt16 check_sum;
    UInt8 input[T4_FEATURE_REPORT_LEN] = {};
    UInt8 readbuf[T4_FEATURE_REPORT_LEN] = {};
    IOReturn ret = kIOReturnSuccess;
    
    input[0] = T4_FEATURE_REPORT_ID;
    if (read_flag) {
        input[1] = T4_CMD_REGISTER_READ;
        input[8] = 0x00;
    } else {
        input[1] = T4_CMD_REGISTER_WRITE;
        input[8] = write_val;
    }
    put_unaligned_le32(address, input + 2);
    input[6] = 1;
    input[7] = 0;
    
    /* Calculate the checksum */
    check_sum = t4_calc_check_sum(input, 1, 8);
    input[9] = (UInt8)check_sum;
    input[10] = (UInt8)(check_sum >> 8);
    input[11] = 0;
    
    OSData* input_updated = OSData::withBytes(input, T4_FEATURE_REPORT_LEN);
    IOBufferMemoryDescriptor* report = IOBufferMemoryDescriptor::withBytes(input_updated->getBytesNoCopy(0, T4_FEATURE_REPORT_LEN), input_updated->getLength(), kIODirectionInOut);
    
    input_updated->release();
    
    hid_interface->setReport(report, kIOHIDReportTypeFeature, T4_FEATURE_REPORT_ID);


    if (read_flag) {
        
 
        ret = hid_interface->getReport(report, kIOHIDReportTypeFeature, T4_FEATURE_REPORT_ID);
        
        report->readBytes(0, &readbuf, T4_FEATURE_REPORT_LEN);

        if (*(UInt32 *)&readbuf[6] != address) {
            IOLog("read register address error (%x,%x)\n", *(UInt32 *)&readbuf[6], address);
            goto exit_readbuf;
        }
        
        if (*(UInt16 *)&readbuf[10] != 1) {
            IOLog("read register size error (%x)\n", *(UInt16 *)&readbuf[10]);
            goto exit_readbuf;
        }
        
        check_sum = t4_calc_check_sum(readbuf, 6, 7);
        if (*(UInt16 *)&readbuf[13] != check_sum) {
            IOLog("read register checksum error (%x,%x)\n", *(UInt16 *)&readbuf[13], check_sum);
            goto exit_readbuf;
        }
        
        *read_val = readbuf[12];
    }
    
    report->release();
    return ret;
    
exit_readbuf:
exit:
    report->release();
    return ret;
}


void AlpsHIDEventDriver::put_unaligned_le32(uint32_t val, void *p)
{
    __put_unaligned_le32(val, (uint8_t*)p);
}

void AlpsHIDEventDriver::__put_unaligned_le16(uint16_t val, uint8_t *p)
{
    *p++ = val;
    *p++ = val >> 8;
}

void AlpsHIDEventDriver::__put_unaligned_le32(uint32_t val, uint8_t *p)
{
    __put_unaligned_le16(val >> 16, p + 2);
    __put_unaligned_le16(val, p);
}

UInt16 AlpsHIDEventDriver::get_unaligned_le16(const void *p)
{
    return __get_unaligned_le16((const UInt8 *)p);
}

UInt16 AlpsHIDEventDriver::__get_unaligned_le16(const UInt8 *p)
{
    return p[0] | p[1] << 8;
}

bool AlpsHIDEventDriver::handleOpen(IOService *forClient, IOOptionBits options, void *arg) {
    if (forClient && forClient->getProperty(VOODOO_INPUT_IDENTIFIER)) {
        voodooInputInstance = forClient;
        voodooInputInstance->retain();
        
        return true;
    }
    
    return super::handleOpen(forClient, options, arg);
}

void AlpsHIDEventDriver::handleClose(IOService *forClient, IOOptionBits options) {
    OSSafeReleaseNULL(voodooInputInstance);
    super::handleClose(forClient, options);
}
