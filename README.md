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

Configuration params:

    1. pos_track.im_width - image width (in pixels), default 1280
    2. pos_track.im_height - image height (in pixels), default 720
    3. pos_track.im_pos_left - horizontal image offset (in pixels), default 0
    4. pos_track.im_pos_top - vertical image offset (in pixels), default 0
    5. pos_track.rgb_mode - whether or not to use RGB mode for tracking (1 or 0), default 0
    6. pos_track.bin_thr_1 - detection threshold for first LED $\in [0,255]$, default 50
    7. pos_track.bin_thr_2 - detection threshold for second LED $\in [0,255]$, default 50
    8. pos_track.first_led_channel - colour channel for first LED ('red', 'green' or 'blue'), default 'red'
    9. pos_track.second_led_channel - colour channel for second LED ('red', 'green' or 'blue'), default 'blue'

Tracker can work in two modes:

    1. RGB - LEDs are distinguished based on colour and
    2. Brightness - LEDs are distinguished based on light intensity

In RGB mode (pos_track.rgb_mode = 1) parameters pos_track.bin_thr_1 and pos_track.bin_thr_2 are detection thresholds for each LED, while pos_track.first_led_channel and pos_track.second_led_channel specify colour for each LED (red, green or blue).
In brightness mode (pos_track.rgb_mode = 0) only pos_track.bin_thr_1 parameter is relevant because the algorithm uses only one threshold and distinguishes LEDs based on the light intensity.

### PositionWriterProcessor
Writing tracked animal positions to a .csv file.
This processor buffers data and writes to the file in a specified time interval.

Configuration params:

    1. pos_writer.out_file - output file path
    2. pos_writer.writing_interval - write to the file every 'n' seconds, default 10

First raw in the output file is header, followed by a raw with data for each recorded frame.
Each raw contains following fields: 

    * timestamp
    * first LED x coordinate
    * first LED y coordinate
    * second LED x coordinate
    * second LED y coordinate

### RawDataWriterProcessor
Writing raw electrophysiological data to a binary file.
Outputs are two binary files, one with electrophysiological data and one with timestamps for each sample in the first file.
This processor buffers data and writes to the files in a specified time interval.

Configuration params:

    1. raw_data_writer.data_file - electrophysiological data file path
    2. raw_data_writer.timestamps_file - timestamps file path
    3. raw_data_writer.writing_interval - write to the file every 'n' milliseconds, default 100

For each channel datum is represented as a 2-byte integer (microvolts).
For each sample all channels are written together. 
E.g. in a case of 128 channels first group of 256 bytes represent data from first sample, second group of 256 bytes represent data from second sample etc.
Timestamps are 4-byte integers and are associated to each sample in the data file.

