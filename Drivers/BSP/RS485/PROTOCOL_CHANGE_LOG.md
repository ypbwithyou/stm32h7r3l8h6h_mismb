# RS485 Protocol Frame Format Change Log

## Summary

Modified RS485 protocol frame format from single address to dual address (source + destination), improving protocol scalability.

## Protocol Format Comparison

### Old Format (Legacy)
```
[Header: 0x3C] [Addr] [Func] [Len:2] [Data:n] [XOR] [Tail: 0x3E]
     1B          1B      1B     2B        nB      1B       1B
```
- **Minimum Frame Length:** 7 bytes
- **Maximum Payload:** 128 bytes
- **Total Maximum:** 7 + 128 = 135 bytes

### New Format (Extended)
```
[Header: 0x3C] [SrcAddr] [DstAddr] [Func] [Len:2] [Data:n] [XOR] [Tail: 0x3E]
     1B            1B         1B       1B      2B        nB      1B       1B
```
- **Minimum Frame Length:** 8 bytes
- **Maximum Payload:** 128 bytes
- **Total Maximum:** 8 + 128 = 136 bytes

## Address Definition

| Address | Description |
|---------|-------------|
| 0x00 | Master device (default source address) |
| 0x01-0x08 | Slave device addresses (sub-devices) |
| 0xFF | Broadcast address (all slaves) |

## Modified Files

### 1. `Drivers/BSP/RS485/rs485.h`
- Added `RS485_BROADCAST_ADDR` (0xFF) definition
- Added protocol format constants (`RS485_PROTO_VERSION_OLD`, `RS485_PROTO_VERSION_NEW`)
- Added minimum frame length constants (`RS485_FRAME_MIN_LEN_OLD`, `RS485_FRAME_MIN_LEN_NEW`)
- Added address validation helper functions (`rs485_is_valid_address()`, `rs485_is_slave_address()`)

### 2. `Drivers/BSP/RS485/rs485.c`
- Modified `HAL_UART_RxCpltCallback()` to support auto-detection of both formats
- Updated frame length calculation logic to handle new format

### 3. `User/rs485_processor.h`
- Modified `rs485_packet_t` structure:
  - Added `src_addr` field (source address)
  - Renamed `address` to `dst_addr` (destination address)
  - Added `proto_format` field to track detected format
- Added `rs485_proto_format_t` enum for format selection
- Added new API functions:
  - `rs485_build_frame_ex()`: Build frame with format selection
  - `rs485_send_frame_ex()`: Send frame with explicit src/dst addresses
- Added broadcast helper macro: `rs485_send_broadcast()`
- Added legacy format helper macro: `rs485_send_frame_legacy()`

### 4. `User/rs485_processor.c`
- Rewrote `rs485_build_frame_ex()`: Supports both old and new format
- Rewrote `rs485_parse_packet()`: Auto-detects format based on frame content
- Modified `rs485_send_frame_ex()`: New extended send API
- Modified `rs485_send_frame()`: Backward compatible, uses new format internally
- Modified `rs485_parse_frame()`: Updated to use src_addr/dst_addr
- Updated `rs485_subdev_scan_once()`: Uses new format
- Updated `rs485_subdev_config_test()`: Uses new format with enhanced logging

## API Changes

### Backward Compatible (Unchanged)
```c
int8_t rs485_send_frame(uint8_t dev_num, uint8_t cmd, const uint8_t *data, uint16_t len);
```
- Internally uses new format with `src_addr = 0x00 (MASTER)`

### New Extended API
```c
int8_t rs485_send_frame_ex(uint8_t src_addr, uint8_t dst_addr, uint8_t cmd, 
                           const uint8_t *data, uint16_t len, rs485_proto_format_t format);
```
- Allows explicit control of source/destination addresses
- Supports format selection (OLD/NEW/AUTO)

### Frame Building
```c
int8_t rs485_build_frame_ex(const rs485_packet_t *packet, uint8_t *out, uint16_t out_size, 
                            uint16_t *out_len, rs485_proto_format_t format);
```

### Packet Parsing (Auto-detect)
```c
int8_t rs485_parse_packet(const uint8_t *frame, uint16_t frame_len, rs485_packet_t *packet_out);
```
- Automatically detects old or new format
- Populates `src_addr`, `dst_addr`, and `proto_format` fields

### Helper Macros
```c
// Broadcast to all slaves
rs485_send_broadcast(cmd, data, len);

// Send using old format (backward compatibility)
rs485_send_frame_legacy(dev_num, cmd, data, len);
```

## Auto-Detection Logic

The protocol automatically detects format when receiving:

1. **Check New Format First:**
   - If frame length ≥ 8 bytes
   - Check if bytes 1-2 are valid addresses (0x00, 0x01-0x08, or 0xFF)
   - Verify frame length matches: 8 + payload_len
   - Validate XOR checksum

2. **Fallback to Old Format:**
   - If new format check fails
   - Check if frame length ≥ 7 bytes
   - Verify frame length matches: 7 + payload_len
   - Validate XOR checksum

## XOR Checksum Calculation

- **Old Format:** XOR from Header (byte 0) to end of Data (byte 4 + payload_len)
- **New Format:** XOR from Header (byte 0) to end of Data (byte 5 + payload_len)

## Migration Guide

### For Existing Code

No changes required! The `rs485_send_frame()` API remains backward compatible and automatically uses the new format.

### For New Features Using Extended Addresses

Use the new extended API:
```c
// Send to specific slave
rs485_send_frame_ex(RS485_MASTER_ADDR, 0x03, cmd, data, len, RS485_PROTO_FMT_NEW);

// Broadcast to all slaves
rs485_send_broadcast(cmd, data, len);

// Use old format for compatibility
rs485_send_frame_ex(RS485_MASTER_ADDR, dev_num, cmd, data, len, RS485_PROTO_FMT_OLD);
```

## Testing

Run the test function to verify:
```c
rs485_subdev_config_test();
```

This will:
1. Send DAC configuration to all valid sub-devices
2. Send bridge configuration to all valid sub-devices
3. Send gain configuration to all valid sub-devices
4. Display response status for each command

## Benefits

1. **Scalability:** Dual address enables routing and multi-master scenarios
2. **Backward Compatibility:** Auto-detection allows seamless coexistence of old and new devices
3. **Broadcast Support:** 0xFF address enables one-to-all communication
4. **Future Extension:** Format field allows future protocol versions

## Notes

- Always include `rs485.h` before `rs485_processor.h` to access address constants
- The XOR checksum now covers one additional byte (src_addr field)
- Minimum frame size increased from 7 to 8 bytes
- Frame structure is now identical in both directions (request and response)
