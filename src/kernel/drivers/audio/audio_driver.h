#ifndef AUDIO_DRIVER_H
#define AUDIO_DRIVER_H

#include <stdint.h>
#include "driver.h"
#include "hardware_detect.h"

// Audio Constants
#define AUDIO_SAMPLE_RATE_8K     8000
#define AUDIO_SAMPLE_RATE_11K    11025
#define AUDIO_SAMPLE_RATE_16K    16000
#define AUDIO_SAMPLE_RATE_22K    22050
#define AUDIO_SAMPLE_RATE_44K    44100
#define AUDIO_SAMPLE_RATE_48K    48000
#define AUDIO_SAMPLE_RATE_96K    96000

#define AUDIO_FORMAT_8BIT        0x08
#define AUDIO_FORMAT_16BIT       0x10
#define AUDIO_FORMAT_24BIT       0x18
#define AUDIO_FORMAT_32BIT       0x20

#define AUDIO_CHANNELS_MONO      1
#define AUDIO_CHANNELS_STEREO    2
#define AUDIO_CHANNELS_5_1       6
#define AUDIO_CHANNELS_7_1       8

// AC'97 Constants
#define AC97_REG_RESET           0x00
#define AC97_REG_MASTER_VOL      0x02
#define AC97_REG_AUX_OUT_VOL     0x04
#define AC97_REG_MASTER_MONO_VOL 0x06
#define AC97_REG_MASTER_TONE     0x08
#define AC97_REG_PC_BEEP_VOL     0x0A
#define AC97_REG_PHONE_VOL       0x0C
#define AC97_REG_MIC_VOL         0x0E
#define AC97_REG_LINE_IN_VOL     0x10
#define AC97_REG_CD_VOL          0x12
#define AC97_REG_VIDEO_VOL       0x14
#define AC97_REG_AUX_IN_VOL      0x16
#define AC97_REG_PCM_OUT_VOL     0x18
#define AC97_REG_RECORD_SELECT   0x1A
#define AC97_REG_RECORD_GAIN     0x1C
#define AC97_REG_RECORD_GAIN_MIC 0x1E
#define AC97_REG_GENERAL_PURPOSE 0x20
#define AC97_REG_3D_CONTROL      0x22
#define AC97_REG_MODEM_RATE      0x24
#define AC97_REG_PCM_RATE        0x2A
#define AC97_REG_VENDOR_ID1      0x7C
#define AC97_REG_VENDOR_ID2      0x7E

// HDA Constants
#define HDA_REG_GCAP            0x00
#define HDA_REG_VMIN            0x02
#define HDA_REG_VMAJ            0x03
#define HDA_REG_OUTPAY          0x04
#define HDA_REG_INPAY           0x06
#define HDA_REG_GCTL            0x08
#define HDA_REG_WAKEEN          0x0C
#define HDA_REG_STATESTS        0x0E
#define HDA_REG_GSTS            0x10
#define HDA_REG_OUTSTRMPAY      0x18
#define HDA_REG_INSTRMPAY       0x1A
#define HDA_REG_INTCTL          0x20
#define HDA_REG_INTSTS          0x24
#define HDA_REG_WALLCLK         0x30
#define HDA_REG_OLD_SSYNC       0x34
#define HDA_REG_SSYNC           0x38
#define HDA_REG_CORBLBASE       0x40
#define HDA_REG_CORBUBASE       0x44
#define HDA_REG_CORBWP          0x48
#define HDA_REG_CORBRP          0x4A
#define HDA_REG_CORBCTL         0x4C
#define HDA_REG_CORBSTS         0x4D
#define HDA_REG_CORBSIZE        0x4E
#define HDA_REG_RIRBLBASE       0x50
#define HDA_REG_RIRBUBASE       0x54
#define HDA_REG_RIRBWP          0x58
#define HDA_REG_RINTCNT         0x5A
#define HDA_REG_RIRBCTL         0x5C
#define HDA_REG_RIRBSTS         0x5D
#define HDA_REG_RIRBSIZE        0x5E
#define HDA_REG_ICW             0x60
#define HDA_REG_IRR             0x64
#define HDA_REG_IRS             0x68
#define HDA_REG_DPLBASE         0x70
#define HDA_REG_DPUBASE         0x74

// Audio Format Structure
typedef struct {
    uint32_t sample_rate;
    uint8_t  bit_depth;
    uint8_t  channels;
    uint8_t  format;
} audio_format_t;

// Audio Buffer Structure
typedef struct {
    void*    buffer;
    uint32_t size;
    uint32_t position;
    uint32_t used;
    int      playing;
} audio_buffer_t;

// Audio Stream Structure
typedef struct {
    audio_format_t format;
    audio_buffer_t buffer;
    uint32_t stream_id;
    int active;
} audio_stream_t;

// Audio Mixer Structure
typedef struct {
    uint16_t master_volume;
    uint16_t pcm_volume;
    uint16_t line_in_volume;
    uint16_t mic_volume;
    uint16_t cd_volume;
    uint8_t  mute;
} audio_mixer_t;

// Audio Statistics
typedef struct {
    uint32_t samples_played;
    uint32_t samples_recorded;
    uint32_t bytes_transferred;
    uint32_t underruns;
    uint32_t overruns;
    uint32_t errors;
} audio_stats_t;

// Audio Driver Interface
typedef struct {
    driver_t base;
    audio_format_t current_format;
    audio_mixer_t mixer;
    audio_stats_t stats;
    audio_stream_t playback_stream;
    audio_stream_t capture_stream;
    int initialized;
    uint32_t capabilities;
} audio_driver_t;

// Hardware-specific driver interfaces
typedef struct {
    int (*init)(audio_driver_t* driver);
    int (*set_format)(audio_driver_t* driver, audio_format_t* format);
    int (*start_playback)(audio_driver_t* driver);
    int (*stop_playback)(audio_driver_t* driver);
    int (*start_capture)(audio_driver_t* driver);
    int (*stop_capture)(audio_driver_t* driver);
    int (*write_samples)(audio_driver_t* driver, void* samples, uint32_t count);
    int (*read_samples)(audio_driver_t* driver, void* samples, uint32_t count);
    int (*set_volume)(audio_driver_t* driver, uint16_t volume);
    int (*get_status)(audio_driver_t* driver);
    int (*reset)(audio_driver_t* driver);
} audio_hw_interface_t;

// AC'97 Driver
typedef struct {
    audio_driver_t base;
    uint32_t io_base;
    uint32_t codec_base;
    uint16_t* codec_regs;
    uint32_t buffer_size;
    void* playback_buffer;
    void* capture_buffer;
    uint32_t buffer_pos;
    int codec_ready;
} ac97_driver_t;

// HDA Driver
typedef struct {
    audio_driver_t base;
    uint32_t mmio_base;
    void* corb_buffer;
    void* rirb_buffer;
    uint32_t corb_size;
    uint32_t rirb_size;
    uint16_t corb_write;
    uint16_t corb_read;
    uint16_t rirb_write;
    uint16_t rirb_read;
    uint8_t codec_count;
    uint8_t current_codec;
} hda_driver_t;

// AC'97 Functions
int ac97_init(audio_driver_t* driver);
int ac97_set_format(audio_driver_t* driver, audio_format_t* format);
int ac97_start_playback(audio_driver_t* driver);
int ac97_stop_playback(audio_driver_t* driver);
int ac97_start_capture(audio_driver_t* driver);
int ac97_stop_capture(audio_driver_t* driver);
int ac97_write_samples(audio_driver_t* driver, void* samples, uint32_t count);
int ac97_read_samples(audio_driver_t* driver, void* samples, uint32_t count);
int ac97_set_volume(audio_driver_t* driver, uint16_t volume);
int ac97_get_status(audio_driver_t* driver);
int ac97_reset(audio_driver_t* driver);
uint16_t ac97_read_codec(ac97_driver_t* ac97, uint8_t reg);
void ac97_write_codec(ac97_driver_t* ac97, uint8_t reg, uint16_t value);

// HDA Functions
int hda_init(audio_driver_t* driver);
int hda_set_format(audio_driver_t* driver, audio_format_t* format);
int hda_start_playback(audio_driver_t* driver);
int hda_stop_playback(audio_driver_t* driver);
int hda_start_capture(audio_driver_t* driver);
int hda_stop_capture(audio_driver_t* driver);
int hda_write_samples(audio_driver_t* driver, void* samples, uint32_t count);
int hda_read_samples(audio_driver_t* driver, void* samples, uint32_t count);
int hda_set_volume(audio_driver_t* driver, uint16_t volume);
int hda_get_status(audio_driver_t* driver);
int hda_reset(audio_driver_t* driver);
int hda_send_command(hda_driver_t* hda, uint32_t command);
uint32_t hda_get_response(hda_driver_t* hda);

// Audio Core Functions
int audio_init(audio_driver_t* driver);
int audio_set_format(audio_driver_t* driver, audio_format_t* format);
int audio_start_playback(audio_driver_t* driver);
int audio_stop_playback(audio_driver_t* driver);
int audio_start_capture(audio_driver_t* driver);
int audio_stop_capture(audio_driver_t* driver);
int audio_write_samples(audio_driver_t* driver, void* samples, uint32_t count);
int audio_read_samples(audio_driver_t* driver, void* samples, uint32_t count);
int audio_driver_set_volume(audio_driver_t* driver, uint16_t volume);
int audio_set_mute(audio_driver_t* driver, uint8_t mute);
int audio_get_buffer_size(audio_driver_t* driver);
int audio_get_buffer_position(audio_driver_t* driver);

// Utility Functions
uint32_t audio_calculate_buffer_size(audio_format_t* format, uint32_t duration_ms);
uint32_t audio_samples_to_bytes(audio_format_t* format, uint32_t samples);
uint32_t audio_bytes_to_samples(audio_format_t* format, uint32_t bytes);
void audio_convert_format(void* input, void* output, audio_format_t* input_format, audio_format_t* output_format, uint32_t samples);

// Driver Creation Functions
driver_t* create_ac97_driver(pci_device_t* device);
driver_t* create_hda_driver(pci_device_t* device);

#endif
