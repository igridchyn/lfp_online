# LFP online
Software system for closed-loop experiments.
Processing is separated in different purpouse-specific 'processors'.
Data flows downstream from source to sink processor.
All processors share single preallocated buffer, e.g. processor takes it's inputs from the shared buffer and writes results back to the buffer.
Together processors form a processing pipeline.
The pipeline is configured through a simple configuration file.

## Processors
This is a (still) non-exhaustive list of available processors.

### IntanInputProcessor
Source processor for data acquisition using Intan RHD2000 board.
Reads data from all available channels and puts it to the shared buffer.

Configuration params:

    1. intan.empty_fifo_step - empty board FIFO buffer every 'n' steps, default 5

### PositionTrackingProcessor
Animal position tracking using a FireWire camera.
Works with up to two LEDs (useful for determining head direction).
Two tracking modes are possible:
    1. RGB - LEDs are discriminated based on color and
    2. Brightness - LEDs are discriminated based on light intensity

Configuration params:
    1. pos_track.im_width - image width (in pixels), default 1280
    2. pos_track.im_height - image height (in pixels), default 720
    3. pos_track.im_pos_left - horizontal image offset (in pixels), default 0
    4. pos_track.im_pos_top - vertical image offset (in pixels), default 0
    5. pos_track.bin_thr_1 - binarization threshold for first LED
    6. pos_track.bin_thr_2 - binarization threshold for second LED
    7. pos_track.rgb_mode - whether or not to use rgb mode for tracking (1 or 0), default 1
    8. pos_track.first_led_channel - color channel for first LED ('red', 'green' or 'blue'), default 'red'
    9. pos_track.second_led_channel - color channel for second LED ('red', 'green' or 'blue'), default 'blue'

### PositionWriterProcessor
Writing tracked animal positions to a file.
File is written to in a specified time interval.

Configuration params:
    1. pos_writer.out_file - output file path
    2. pos_writer.writing_interval - write to the file every 'n' seconds, default 10

