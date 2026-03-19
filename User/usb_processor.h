#ifndef __USB_PROCESSOR_H
#define __USB_PROCESSOR_H

#include <stdint.h>
#include "./LIBS/lib_usb_protocol/usb_protocol.h"
#include "collector_processor.h"
 
void usb_init(void);
void usb_device_info_reload_from_file(void);

void SystemStatusInit(void);

void USB_Display_All(uint32_t run_flag);

void IdaProcessor(void);

void on_frame(uint8_t *frame, uint32_t frame_len);

#endif /* __USB_PROCESSOR_H */
