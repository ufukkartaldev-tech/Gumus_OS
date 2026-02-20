#ifndef USB_AUDIO_H
#define USB_AUDIO_H

#include "usb_host.h"
#include "../core/types.h"

// USB Audio Class Constants
#define USB_AUDIO_CLASS            0x01
#define USB_AUDIO_SUBCLASS_CONTROL 0x01
#define USB_AUDIO_SUBCLASS_STREAMING 0x02
#define USB_AUDIO_SUBCLASS_MIDI_STREAMING 0x03

// USB Audio Protocol
#define USB_AUDIO_PROTOCOL_UNDEFINED 0x00
#define USB_AUDIO_PROTOCOL_VERSION_1_0 0x10
#define USB_AUDIO_PROTOCOL_VERSION_2_0 0x20

// USB Audio Class-Specific Requests
#define AUDIO_REQ_SET_CUR          0x01
#define AUDIO_REQ_GET_CUR          0x81
#define AUDIO_REQ_SET_MIN          0x02
#define AUDIO_REQ_GET_MIN          0x82
#define AUDIO_REQ_SET_MAX          0x03
#define AUDIO_REQ_GET_MAX          0x83
#define AUDIO_REQ_SET_RES          0x04
#define AUDIO_REQ_GET_RES          0x84
#define AUDIO_REQ_SET_MEM          0x05
#define AUDIO_REQ_GET_MEM          0x85
#define AUDIO_REQ_GET_STAT         0xFF

// USB Audio Control Selector
#define AUDIO_CONTROL_UNDEFINED    0x00
#define AUDIO_CONTROL_MUTE         0x01
#define AUDIO_CONTROL_VOLUME       0x02
#define AUDIO_CONTROL_BASS         0x03
#define AUDIO_CONTROL_MID          0x04
#define AUDIO_CONTROL_TREBLE       0x05
#define AUDIO_CONTROL_GRAPHIC_EQUALIZER 0x06
#define AUDIO_CONTROL_AUTOMATIC_GAIN 0x07
#define AUDIO_CONTROL_AUTOMATIC_GAIN_CONTROL 0x08
#define AUDIO_CONTROL_DELAY        0x09
#define AUDIO_CONTROL_BASS_BOOST   0x0A
#define AUDIO_CONTROL_LOUDNESS     0x0B
#define AUDIO_CONTROL_INPUT_GAIN   0x0C
#define AUDIO_CONTROL_INPUT_GAIN_PAD 0x0D
#define AUDIO_CONTROL_PHASE_INVERTER 0x0E
#define AUDIO_CONTROL_UNDERFLOW    0x0F
#define AUDIO_CONTROL_OVERFLOW     0x10
#define AUDIO_CONTROL_SAMPLING_FREQ 0x11

// USB Audio Terminal Types
#define AUDIO_TERMINAL_USB_STREAMING 0x0101
#define AUDIO_TERMINAL_USB_OUT_STREAMING 0x0102
#define AUDIO_TERMINAL_VENDOR_SPECIFIC 0x01FF
#define AUDIO_TERMINAL_UNDEFINED 0x0200
#define AUDIO_TERMINAL_STREAMING 0x0300
#define AUDIO_TERMINAL_VENDOR 0x0400
#define AUDIO_TERMINAL_MICROPHONE 0x0201
#define AUDIO_TERMINAL_DESKTOP_MICROPHONE 0x0202
#define AUDIO_TERMINAL_PERSONAL_MICROPHONE 0x0203
#define AUDIO_TERMINAL_OMNI_MICROPHONE 0x0204
#define AUDIO_TERMINAL_MICROPHONE_ARRAY 0x0205
#define AUDIO_TERMINAL_PROC_MICROPHONE_ARRAY 0x0206

// USB Audio Format Types
#define AUDIO_FORMAT_TYPE_I_UNDEFINED 0x00
#define AUDIO_FORMAT_TYPE_I_PCM 0x01
#define AUDIO_FORMAT_TYPE_I_PCM8 0x02
#define AUDIO_FORMAT_TYPE_I_IEEE_FLOAT 0x03
#define AUDIO_FORMAT_TYPE_I_ALAW 0x04
#define AUDIO_FORMAT_TYPE_I_MULAW 0x05

// USB Audio Data Format
#define AUDIO_DATA_FORMAT_TYPE_I_PCM 0x0001
#define AUDIO_DATA_FORMAT_TYPE_I_PCM8 0x0002
#define AUDIO_DATA_FORMAT_TYPE_I_IEEE_FLOAT 0x0003
#define AUDIO_DATA_FORMAT_TYPE_I_ALAW 0x0004
#define AUDIO_DATA_FORMAT_TYPE_I_MULAW 0x0005

// USB Audio Endpoint Attributes
#define AUDIO_ENDPOINT_SAMPLE_FREQ 0x01
#define AUDIO_ENDPOINT_PITCH 0x02
#define AUDIO_ENDPOINT_DATA_PACKETS 0x80

// USB Audio Interface Descriptor
typedef struct {
    uint8_t length;
    uint8_t type;
    uint8_t interface_number;
    uint8_t alternate_setting;
    uint8_t num_endpoints;
    uint8_t interface_class;
    uint8_t interface_subclass;
    uint8_t interface_protocol;
    uint8_t interface;
} __attribute__((packed)) usb_audio_interface_descriptor_t;

// USB Audio Class-Specific Interface Descriptor
typedef struct {
    uint8_t length;
    uint8_t type;
    uint8_t subtype;
    uint16_t version;
    uint16_t total_length;
    uint8_t collection;
    uint8_t interface_number;
    uint8_t clock_source_id;
    uint8_t num_controls;
    uint8_t feature_unit_id;
} __attribute__((packed)) usb_audio_class_interface_descriptor_t;

// USB Audio Class-Specific Endpoint Descriptor
typedef struct {
    uint8_t length;
    uint8_t type;
    uint8_t subtype;
    uint8_t attributes;
    uint8_t lock_delay_units;
    uint16_t lock_delay;
} __attribute__((packed)) usb_audio_class_endpoint_descriptor_t;

// USB Audio Format Type Descriptor
typedef struct {
    uint8_t length;
    uint8_t type;
    uint8_t subtype;
    uint8_t format_type;
    uint8_t channels;
    uint8_t subframe_size;
    uint8_t bit_resolution;
    uint8_t freq_type;
    uint8_t freq_low;
    uint16_t freq_mid;
    uint16_t freq_high;
} __attribute__((packed)) usb_audio_format_descriptor_t;

// USB Audio Device Structure
typedef struct usb_audio_device {
    usb_device_t* usb_device;
    
    // Audio endpoints
    uint8_t audio_in_endpoint;
    uint8_t audio_out_endpoint;
    uint16_t max_packet_size_in;
    uint16_t max_packet_size_out;
    uint8_t poll_interval;
    
    // Audio format
    uint16_t sample_rate;
    uint8_t channels;
    uint8_t bit_depth;
    uint8_t format_type;
    
    // Audio controls
    uint8_t mute_control;
    uint16_t volume_control;
    uint8_t bass_control;
    uint8_t treble_control;
    
    // Buffers
    uint8_t* input_buffer;
    uint8_t* output_buffer;
    uint32_t buffer_size;
    
    // State
    uint8_t initialized;
    uint8_t streaming;
    uint8_t capture_enabled;
    uint8_t playback_enabled;
    
    struct usb_audio_device* next;
} usb_audio_device_t;

// USB Audio Driver Structure
typedef struct {
    driver_t base;
    usb_audio_device_t* devices;
    uint32_t device_count;
} usb_audio_driver_t;

// Function Prototypes
int usb_audio_init();
int usb_audio_probe(usb_device_t* usb_device);
int usb_audio_remove(usb_device_t* usb_device);
int usb_audio_set_sample_rate(usb_audio_device_t* device, uint16_t sample_rate);
int usb_audio_set_channels(usb_audio_device_t* device, uint8_t channels);
int usb_audio_set_volume(usb_audio_device_t* device, uint16_t channel, uint16_t volume);
int usb_audio_set_mute(usb_audio_device_t* device, uint16_t channel, uint8_t mute);
int usb_audio_start_streaming(usb_audio_device_t* device);
int usb_audio_stop_streaming(usb_audio_device_t* device);
int usb_audio_capture_start(usb_audio_device_t* device);
int usb_audio_capture_stop(usb_audio_device_t* device);
int usb_audio_playback_start(usb_audio_device_t* device);
int usb_audio_playback_stop(usb_audio_device_t* device);

// Audio control functions
int usb_audio_get_control(usb_audio_device_t* device, uint8_t control_selector, 
                          uint8_t channel, uint16_t* value);
int usb_audio_set_control(usb_audio_device_t* device, uint8_t control_selector, 
                          uint8_t channel, uint16_t value);

// Audio streaming functions
int usb_audio_read_samples(usb_audio_device_t* device, uint8_t* buffer, uint32_t length);
int usb_audio_write_samples(usb_audio_device_t* device, const uint8_t* buffer, uint32_t length);

// Device management
usb_audio_device_t* usb_audio_find_device(usb_device_t* usb_device);
int usb_audio_add_device(usb_audio_device_t* device);
int usb_audio_remove_device(usb_audio_device_t* device);
void usb_audio_list_devices();

// Driver interface functions
driver_t* create_usb_audio_driver();

#endif
