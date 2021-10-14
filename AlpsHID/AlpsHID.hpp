
/*This code is derived and adapted from VoodooI2CHID's Multitouch Event Driver and Precision
Touchpad Event Driver (https://github.com/alexandred/VoodooI2C) and the Linux kernel driver
for the alps t4 touchpad (https://github.com/torvalds/linux/blob/master/drivers/hid/hid-alps.c) */




#ifndef AlpsHIDEventDriver_hpp
#define AlpsHIDEventDriver_hpp

#define _IOKIT_HID_IOHIDEVENTDRIVER_H


#include <IOKit/IOLib.h>
#include <IOKit/IOKitKeys.h>
#include <IOKit/IOService.h>
#include <IOKit/IOBufferMemoryDescriptor.h>
#include <IOKit/IOCommandGate.h>
#include <IOKit/hid/IOHIDInterface.h>
#include <kern/clock.h>


#include "IOKit/hid/IOHIDEvent.h"
#include "IOKit/hid/IOHIDEventService.h"
#include "IOKit/hid/IOHIDEventTypes.h"
#include <IOKit/hidsystem/IOHIDTypes.h>
#include "IOKit/hid/IOHIDPrivateKeys.h"
#include <IOKit/hid/IOHIDUsageTables.h>
#include <IOKit/hid/IOHIDDevice.h>

#include "../VoodooInput/VoodooInput/VoodooInputMultitouch/VoodooInputTransducer.h"
#include "../VoodooInput/VoodooInput/VoodooInputMultitouch/VoodooInputMessages.h"

#include "helpers.hpp"


#define HID_PRODUCT_ID_U1_DUAL      0x120B
#define HID_PRODUCT_ID_T4_BTNLESS   0x120C
#define HID_PRODUCT_ID_G1           0x120D
#define HID_PRODUCT_ID_U1           0x1209
#define HID_PRODUCT_ID_T4_USB       0X1216
#define HID_PRODUCT_ID_U1_DUAL_PTP  0x121F
#define HID_PRODUCT_ID_U1_PTP_2     0x120A

#define ALPS_VENDOR                 0x44e


#define T4_INPUT_REPORT_LEN         sizeof(struct t4_input_report)
#define T4_FEATURE_REPORT_LEN       T4_INPUT_REPORT_LEN
#define T4_FEATURE_REPORT_ID        0x07
#define T4_CMD_REGISTER_READ        0x08
#define T4_CMD_REGISTER_WRITE       0x07
#define T4_INPUT_REPORT_ID          0x09

#define T4_ADDRESS_BASE                0xC2C0
#define PRM_SYS_CONFIG_1            (T4_ADDRESS_BASE + 0x0002)
#define T4_PRM_FEED_CONFIG_1        (T4_ADDRESS_BASE + 0x0004)
#define T4_PRM_FEED_CONFIG_4        (T4_ADDRESS_BASE + 0x001A)
#define T4_PRM_ID_CONFIG_3          (T4_ADDRESS_BASE + 0x00B0)
#define T4_GPIO_PINS                (T4_ADDRESS_BASE + 0x000C)

#define T4_FEEDCFG4_ADVANCED_ABS_ENABLE            0x01
#define T4_I2C_ABS                  0x78

#define T4_COUNT_PER_ELECTRODE      256
#define MAX_TOUCHES                 5

#define U1_MOUSE_REPORT_ID          0x01 /* Mouse data ReportID */
#define U1_ABSOLUTE_REPORT_ID       0x03 /* Absolute data ReportID */
#define U1_ABSOLUTE_REPORT_ID_SECD  0x02 /* FW-PTP Absolute data ReportID */
#define U1_FEATURE_REPORT_ID        0x05 /* Feature ReportID */
#define U1_SP_ABSOLUTE_REPORT_ID    0x06 /* Feature ReportID */

#define U1_FEATURE_REPORT_LEN       0x08 /* Feature Report Length */
#define U1_FEATURE_REPORT_LEN_ALL   0x0A
#define U1_CMD_REGISTER_READ        0xD1
#define U1_CMD_REGISTER_WRITE       0xD2

#define U1_DEVTYPE_SP_SUPPORT       0x10 /* SP Support */
#define U1_DISABLE_DEV              0x01
#define U1_TP_ABS_MODE              0x02
#define U1_SP_ABS_MODE              0x80

#define ADDRESS_U1_DEV_CTRL_1       0x00800040
#define ADDRESS_U1_DEVICE_TYP       0x00800043
#define ADDRESS_U1_NUM_SENS_X       0x00800047
#define ADDRESS_U1_NUM_SENS_Y       0x00800048
#define ADDRESS_U1_PITCH_SENS_X     0x00800049
#define ADDRESS_U1_PITCH_SENS_Y     0x0080004A
#define ADDRESS_U1_RESO_DWN_ABS     0x0080004E
#define ADDRESS_U1_PAD_BTN          0x00800052
#define ADDRESS_U1_SP_BTN           0x0080009F


// Message types defined by ApplePS2Keyboard
enum
{
    // from keyboard to mouse/touchpad
    kKeyboardSetTouchStatus = iokit_vendor_specific_msg(100),   // set disable/enable touchpad (data is bool*)
    kKeyboardGetTouchStatus = iokit_vendor_specific_msg(101),   // get disable/enable touchpad (data is bool*)
    kKeyboardKeyPressTime = iokit_vendor_specific_msg(110)      // notify of timestamp a non-modifier key was pressed (data is uint64_t*)
};

enum dev_num {
    U1,
    T4,
};

struct alps_dev {
    UInt8     max_fingers;
    UInt8     has_sp;
    UInt8     sp_btn_info;
    UInt32    x_active_len_mm;
    UInt32    y_active_len_mm;
    UInt32    x_max;
    UInt32    y_max;
    UInt32    x_min;
    UInt32    y_min;
    UInt32    btn_cnt;
    UInt32    sp_btn_cnt;

};


struct __attribute__((__packed__)) t4_contact_data {
    UInt8  palm;
    UInt8    x_lo;
    UInt8    x_hi;
    UInt8    y_lo;
    UInt8    y_hi;
};

struct __attribute__((__packed__)) t4_input_report {
    UInt8  reportID;
    UInt8  numContacts;
    struct t4_contact_data contact[5];
    UInt8  button;
    UInt8  track[5];
    UInt8  zx[5], zy[5];
    UInt8  palmTime[5];
    UInt8  kilroy;
    UInt16 timeStamp;
};

struct __attribute__((__packed__)) u1_contact_data {
    UInt8    x_lo;
    UInt8    x_hi;
    UInt8    y_lo;
    UInt8    y_hi;
    UInt8       z;
};

struct __attribute__((__packed__)) u1_input_report {
    UInt8  reportID;
    UInt8  buttons;
    UInt8  numContacts;
    struct u1_contact_data contact[5];
};

struct __attribute__((__packed__)) u1_sp_input_report{
    UInt8 reportID;
    UInt8 buttons;
    short x;
    short y;
    UInt16 z;
};

class AlpsHIDEventDriver : public IOHIDEventService {
    OSDeclareDefaultStructors(AlpsHIDEventDriver);
    
public:
    
    void handleInterruptReport(AbsoluteTime timestamp, IOMemoryDescriptor *report, IOHIDReportType report_type, UInt32 report_id);
    
    bool init(OSDictionary *properties) override;
    
    bool start(IOService* provider) override;
    
    bool handleStart(IOService* provider) override;
    void handleStop(IOService* provider) override;
    
    bool handleOpen(IOService *forClient, IOOptionBits options, void *arg) override;
    void handleClose(IOService *forClient, IOOptionBits options) override;
    
    virtual IOReturn message(UInt32 type, IOService* provider, void* argument) override;
    bool didTerminate(IOService* provider, IOOptionBits options, bool* defer) override;

    IOReturn setPowerState(unsigned long whichState, IOService* whatDevice) override;
    
    
    IOReturn publishMultitouchInterface();
    
    void put_unaligned_le32(uint32_t val, void *p);
    void __put_unaligned_le16(uint16_t val, uint8_t *p);
    void __put_unaligned_le32(uint32_t val, uint8_t *p);
    UInt16 get_unaligned_le16(const void *p);
    UInt16 __get_unaligned_le16(const UInt8 *p);

    
    
protected:
    const char* name;
    IOHIDInterface* hid_interface;
    
private:
    void t4_raw_event(AbsoluteTime timestamp, IOMemoryDescriptor *report, IOHIDReportType report_type, UInt32 report_id);
    void u1_raw_event(AbsoluteTime timestamp, IOMemoryDescriptor *report, IOHIDReportType report_type, UInt32 report_id);
    bool ready;
    uint64_t max_after_typing;
    uint64_t key_time;
    bool awake;
    dev_num dev_type;
    IOWorkLoop* work_loop;
    IOCommandGate* command_gate;
    
    VoodooInputEvent inputMessage;
    IOService* voodooInputInstance;
    
    UInt16 t4_calc_check_sum(UInt8 *buffer, unsigned long offset, unsigned long length);
    /* Sends a report to the device to instruct it to enter Touchpad mode */
    void t4_device_init();
    bool u1_device_init();
    
    IOReturn u1_read_write_register(UInt32 address, UInt8 *read_val, UInt8 write_val, bool read_flag);
    IOReturn t4_read_write_register(UInt32 address, UInt8 *read_val, UInt8 write_val, bool read_flag);
    
};


#endif /* AlpsHIDEventDriver_hpp */

