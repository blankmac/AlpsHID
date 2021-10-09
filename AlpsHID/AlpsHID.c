//
//  AlpsHID.c
//  AlpsHID
//
//  Created by blankmac on 10/9/21.
//

#include <mach/mach_types.h>

kern_return_t AlpsHID_start(kmod_info_t * ki, void *d);
kern_return_t AlpsHID_stop(kmod_info_t *ki, void *d);

kern_return_t AlpsHID_start(kmod_info_t * ki, void *d)
{
    return KERN_SUCCESS;
}

kern_return_t AlpsHID_stop(kmod_info_t *ki, void *d)
{
    return KERN_SUCCESS;
}
