#include "usb_audio.h"
#include "memory.h"
#include "string.h"
#include "stdio.h"

// Global audio driver
static usb_audio_driver_t audio_driver;

// Global device list
static usb_audio_device_t* audio_devices = NULL;

// Initialize USB Audio driver
int usb_audio_init() {
    printf("USB Audio: SÃ¼rÃ¼cÃ¼ baÅŸlatÄ±lÄ±yor\n");
    
    audio_driver.devices = NULL;
    audio_driver.device_count = 0;
    
    strcpy(audio_driver.base.name, "USB Audio Driver");
    audio_driver.base.type = DRIVER_TYPE_INPUT;
    audio_driver.base.class = DRIVER_CLASS_MULTIMEDIA;
    audio_driver.base.vendor_id = 0;
    audio_driver.base.device_id = 0;
    
    printf("USB Audio: SÃ¼rÃ¼cÃ¼ baÅŸlatÄ±ldÄ±\n");
    return 0;
}

// Get audio control value
int usb_audio_get_control(usb_audio_device_t* device, uint8_t control_selector, 
                          uint8_t channel, uint16_t* value) {
    usb_setup_packet_t setup;
    
    setup.request_type = 0xA1; // Device-to-host, Class, Interface
    setup.request = AUDIO_REQ_GET_CUR;
    setup.value = (control_selector << 8) | channel;
    setup.index = 0; // Interface number
    setup.length = 2;
    
    int result = usb_host_control_transfer(
        (usb_host_controller_t*)device->usb_device->host_controller,
        device->usb_device->address,
        0,
        &setup,
        (uint8_t*)value,
        2
    );
    
    if (result != 0) {
        printf("USB Audio: Control deÄŸeri alma baÅŸarÄ±sÄ±z\n");
        return -1;
    }
    
    return 0;
}

// Set audio control value
int usb_audio_set_control(usb_audio_device_t* device, uint8_t control_selector, 
                          uint8_t channel, uint16_t value) {
    usb_setup_packet_t setup;
    
    setup.request_type = 0x21; // Host-to-device, Class, Interface
    setup.request = AUDIO_REQ_SET_CUR;
    setup.value = (control_selector << 8) | channel;
    setup.index = 0; // Interface number
    setup.length = 2;
    
    int result = usb_host_control_transfer(
        (usb_host_controller_t*)device->usb_device->host_controller,
        device->usb_device->address,
        0,
        &setup,
        (uint8_t*)&value,
        2
    );
    
    if (result != 0) {
        printf("USB Audio: Control deÄŸeri ayarlama baÅŸarÄ±sÄ±z\n");
        return -1;
    }
    
    return 0;
}

// Set sample rate
int usb_audio_set_sample_rate(usb_audio_device_t* device, uint16_t sample_rate) {
    usb_setup_packet_t setup;
    
    setup.request_type = 0x21; // Host-to-device, Class, Interface
    setup.request = AUDIO_REQ_SET_CUR;
    setup.value = (AUDIO_CONTROL_SAMPLING_FREQ << 8);
    setup.index = 0; // Interface number
    setup.length = 3;
    
    uint8_t freq_data[3];
    freq_data[0] = sample_rate & 0xFF;
    freq_data[1] = (sample_rate >> 8) & 0xFF;
    freq_data[2] = (sample_rate >> 16) & 0xFF;
    
    int result = usb_host_control_transfer(
        (usb_host_controller_t*)device->usb_device->host_controller,
        device->usb_device->address,
        0,
        &setup,
        freq_data,
        3
    );
    
    if (result != 0) {
        printf("USB Audio: Sample rate ayarlama baÅŸarÄ±sÄ±z\n");
        return -1;
    }
    
    device->sample_rate = sample_rate;
    printf("USB Audio: Sample rate ayarlandÄ±: %d Hz\n", sample_rate);
    return 0;
}

// Set number of channels
int usb_audio_set_channels(usb_audio_device_t* device, uint8_t channels) {
    // This would typically be done by selecting the appropriate alternate setting
    // For now, just store the value
    device->channels = channels;
    printf("USB Audio: Kanal sayÄ±sÄ± ayarlandÄ±: %d\n", channels);
    return 0;
}

// Set volume
int usb_audio_set_volume(usb_audio_device_t* device, uint16_t channel, uint16_t volume) {
    int result = usb_audio_set_control(device, AUDIO_CONTROL_VOLUME, channel, volume);
    if (result == 0) {
        device->volume_control = volume;
        printf("USB Audio: Volume ayarlandÄ±: Kanal %d, DeÄŸer %d\n", channel, volume);
    }
    return result;
}

// Set mute
int usb_audio_set_mute(usb_audio_device_t* device, uint16_t channel, uint8_t mute) {
    int result = usb_audio_set_control(device, AUDIO_CONTROL_MUTE, channel, mute ? 1 : 0);
    if (result == 0) {
        device->mute_control = mute;
        printf("USB Audio: Mute ayarlandÄ±: Kanal %d, %s\n", channel, mute ? "Evet" : "HayÄ±r");
    }
    return result;
}

// Start audio streaming
int usb_audio_start_streaming(usb_audio_device_t* device) {
    // This would typically involve selecting the appropriate alternate setting
    // and starting the isochronous transfers
    
    device->streaming = 1;
    printf("USB Audio: Streaming baÅŸlatÄ±ldÄ±\n");
    return 0;
}

// Stop audio streaming
int usb_audio_stop_streaming(usb_audio_device_t* device) {
    device->streaming = 0;
    device->capture_enabled = 0;
    device->playback_enabled = 0;
    printf("USB Audio: Streaming durduruldu\n");
    return 0;
}

// Start audio capture
int usb_audio_capture_start(usb_audio_device_t* device) {
    if (!device->streaming) {
        if (usb_audio_start_streaming(device) != 0) {
            return -1;
        }
    }
    
    device->capture_enabled = 1;
    printf("USB Audio: Capture baÅŸlatÄ±ldÄ±\n");
    return 0;
}

// Stop audio capture
int usb_audio_capture_stop(usb_audio_device_t* device) {
    device->capture_enabled = 0;
    printf("USB Audio: Capture durduruldu\n");
    return 0;
}

// Start audio playback
int usb_audio_playback_start(usb_audio_device_t* device) {
    if (!device->streaming) {
        if (usb_audio_start_streaming(device) != 0) {
            return -1;
        }
    }
    
    device->playback_enabled = 1;
    printf("USB Audio: Playback baÅŸlatÄ±ldÄ±\n");
    return 0;
}

// Stop audio playback
int usb_audio_playback_stop(usb_audio_device_t* device) {
    device->playback_enabled = 0;
    printf("USB Audio: Playback durduruldu\n");
    return 0;
}

// Read audio samples
int usb_audio_read_samples(usb_audio_device_t* device, uint8_t* buffer, uint32_t length) {
    if (!device->capture_enabled) {
        printf("USB Audio: Capture etkin deÄŸil\n");
        return -1;
    }
    
    // This would use isochronous transfers to read audio data
    // For now, return a placeholder
    printf("USB Audio: %d bayt sample okuma\n", length);
    return 0;
}

// Write audio samples
int usb_audio_write_samples(usb_audio_device_t* device, const uint8_t* buffer, uint32_t length) {
    if (!device->playback_enabled) {
        printf("USB Audio: Playback etkin deÄŸil\n");
        return -1;
    }
    
    // This would use isochronous transfers to write audio data
    // For now, return a placeholder
    printf("USB Audio: %d bayt sample yazma\n", length);
    return 0;
}

// Probe USB device for audio support
int usb_audio_probe(usb_device_t* usb_device) {
    if (!usb_device) {
        return -1;
    }
    
    // Check if this is an audio device
    if (usb_device->device_desc.class_code != USB_AUDIO_CLASS) {
        return -1;
    }
    
    printf("USB Audio: AygÄ±t tespit edildi: VID:PID=%04X:%04X\n",
           usb_device->device_desc.vendor_id, usb_device->device_desc.product_id);
    
    // Create audio device structure
    usb_audio_device_t* device = malloc(sizeof(usb_audio_device_t));
    if (!device) {
        printf("USB Audio: Bellek tahsis hatasÄ±\n");
        return -1;
    }
    
    memset(device, 0, sizeof(usb_audio_device_t));
    device->usb_device = usb_device;
    
    // TODO: Get audio interface descriptor
    // For now, use default values
    device->audio_in_endpoint = 0x81; // EP1 IN
    device->audio_out_endpoint = 0x02; // EP2 OUT
    device->max_packet_size_in = 64;
    device->max_packet_size_out = 64;
    device->poll_interval = 1;
    
    // Default audio format
    device->sample_rate = 44100;
    device->channels = 2;
    device->bit_depth = 16;
    device->format_type = AUDIO_FORMAT_TYPE_I_PCM;
    
    // Allocate audio buffers
    device->buffer_size = 4096; // 4KB buffer
    device->input_buffer = malloc(device->buffer_size);
    device->output_buffer = malloc(device->buffer_size);
    
    if (!device->input_buffer || !device->output_buffer) {
        printf("USB Audio: Buffer bellek tahsis hatasÄ±\n");
        if (device->input_buffer) free(device->input_buffer);
        if (device->output_buffer) free(device->output_buffer);
        free(device);
        return -1;
    }
    
    // Initialize audio controls
    device->mute_control = 0;
    device->volume_control = 0xFFFF; // Max volume
    device->bass_control = 0;
    device->treble_control = 0;
    
    // Set default audio parameters
    usb_audio_set_sample_rate(device, device->sample_rate);
    usb_audio_set_channels(device, device->channels);
    usb_audio_set_volume(device, 0, device->volume_control);
    usb_audio_set_mute(device, 0, device->mute_control);
    
    device->initialized = 1;
    
    // Add to device list
    device->next = audio_devices;
    audio_devices = device;
    audio_driver.device_count++;
    
    printf("USB Audio: AygÄ±t eklendi\n");
    printf("  Format: %d Hz, %d kanal, %d-bit\n", 
           device->sample_rate, device->channels, device->bit_depth);
    
    return 0;
}

// Remove audio device
int usb_audio_remove(usb_device_t* usb_device) {
    usb_audio_device_t** current = &audio_devices;
    
    while (*current) {
        if ((*current)->usb_device == usb_device) {
            usb_audio_device_t* to_remove = *current;
            *current = (*current)->next;
            audio_driver.device_count--;
            
            printf("USB Audio: AygÄ±t kaldÄ±rÄ±ldÄ±\n");
            
            // Free buffers
            if (to_remove->input_buffer) {
                free(to_remove->input_buffer);
            }
            if (to_remove->output_buffer) {
                free(to_remove->output_buffer);
            }
            
            free(to_remove);
            return 0;
        }
        current = &(*current)->next;
    }
    
    return -1;
}

// List all audio devices
void usb_audio_list_devices() {
    printf("\n=== USB Audio AygÄ±tlarÄ± ===\n");
    
    usb_audio_device_t* current = audio_devices;
    int count = 1;
    
    while (current) {
        printf("%d. VID:PID=%04X:%04X\n", count++,
               current->usb_device->device_desc.vendor_id,
               current->usb_device->device_desc.product_id);
        printf("   Format: %d Hz, %d kanal, %d-bit\n", 
               current->sample_rate, current->channels, current->bit_depth);
        printf("   Streaming: %s, Capture: %s, Playback: %s\n",
               current->streaming ? "Evet" : "HayÄ±r",
               current->capture_enabled ? "Evet" : "HayÄ±r",
               current->playback_enabled ? "Evet" : "HayÄ±r");
        printf("   Volume: %d, Mute: %s\n",
               current->volume_control,
               current->mute_control ? "Evet" : "HayÄ±r");
        
        current = current->next;
    }
    
    if (count == 1) {
        printf("HiÃ§bir USB Audio aygÄ±tÄ± baÄŸlÄ± deÄŸil\n");
    }
    
    printf("==========================\n");
}

// Find device by USB device
usb_audio_device_t* usb_audio_find_device(usb_device_t* usb_device) {
    usb_audio_device_t* current = audio_devices;
    
    while (current) {
        if (current->usb_device == usb_device) {
            return current;
        }
        current = current->next;
    }
    
    return NULL;
}

// Create USB Audio driver
driver_t* create_usb_audio_driver() {
    if (usb_audio_init() != 0) {
        return NULL;
    }
    
    return (driver_t*)&audio_driver;
}
