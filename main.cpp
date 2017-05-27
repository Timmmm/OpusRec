// This is a simple program to record from a microphone to an Opus-encoded file.

#include <soundio/soundio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <errno.h>

#include <docopt.h>
#include <string>
#include <iostream>
#include <map>

using namespace std;

// This is passed to the read callback in the `void* userdata` pointer.
struct RecordContext
{
	SoundIoRingBuffer* ring_buffer;
};

static int min_int(int a, int b)
{
	return (a < b) ? a : b;
}

// This callback is called when libsoundio has some auto data to send us.
static void read_callback(struct SoundIoInStream* instream, int frame_count_min, int frame_count_max)
{
	// Recover the RecordContext object.
	RecordContext* rc = static_cast<RecordContext*>(instream->userdata);

	SoundIoChannelArea* areas;
	int err;
	char* write_ptr = soundio_ring_buffer_write_ptr(rc->ring_buffer);
	int free_bytes = soundio_ring_buffer_free_count(rc->ring_buffer);
	int free_count = free_bytes / instream->bytes_per_frame;
	if (free_count < frame_count_min)
	{
		fprintf(stderr, "ring buffer overflow\n");
		exit(1);
	}

	int write_frames = min_int(free_count, frame_count_max);
	int frames_left = write_frames;
	for (;;)
	{
		int frame_count = frames_left;
		if ((err = soundio_instream_begin_read(instream, &areas, &frame_count)))
		{
			fprintf(stderr, "begin read error: %s", soundio_strerror(err));
			exit(1);
		}

		if (!frame_count)
			break;
		if (!areas)
		{
			// Due to an overflow there is a hole. Fill the ring buffer with
			// silence for the size of the hole.
			memset(write_ptr, 0, frame_count * instream->bytes_per_frame);
		}
		else
		{
			for (int frame = 0; frame < frame_count; ++frame)
			{
				for (int ch = 0; ch < instream->layout.channel_count; ++ch)
				{
					memcpy(write_ptr, areas[ch].ptr, instream->bytes_per_sample);
					areas[ch].ptr += areas[ch].step;
					write_ptr += instream->bytes_per_sample;
				}
			}
		}
		if ((err = soundio_instream_end_read(instream)))
		{
			fprintf(stderr, "end read error: %s", soundio_strerror(err));
			exit(1);
		}

		frames_left -= frame_count;
		if (frames_left <= 0)
			break;
	}

	int advance_bytes = write_frames * instream->bytes_per_frame;
	soundio_ring_buffer_advance_write_ptr(rc->ring_buffer, advance_bytes);
}

static void overflow_callback(struct SoundIoInStream *instream)
{
	static int count = 0;
	fprintf(stderr, "overflow %d\n", ++count);
}

static void printChannelLayout(const SoundIoChannelLayout* layout)
{
	if (layout->name)
	{
		fprintf(stderr, "%s", layout->name);
	}
	else
	{
		fprintf(stderr, "%s", soundio_get_channel_name(layout->channels[0]));
		for (int i = 1; i < layout->channel_count; ++i)
		{
			fprintf(stderr, ", %s", soundio_get_channel_name(layout->channels[i]));
		}
	}
}

bool short_output = false;
void printDevice(SoundIoDevice* device, bool is_default)
{
	const char *default_str = is_default ? " (default)" : "";
	const char *raw_str = device->is_raw ? " (raw)" : "";
	fprintf(stderr, "%s%s%s\n", device->name, default_str, raw_str);
	if (short_output)
		return;
	fprintf(stderr, "  id: %s\n", device->id);
	if (device->probe_error) {
		fprintf(stderr, "  probe error: %s\n", soundio_strerror(device->probe_error));
	} else {
		fprintf(stderr, "  channel layouts:\n");
		for (int i = 0; i < device->layout_count; i += 1) {
			fprintf(stderr, "    ");
			printChannelLayout(&device->layouts[i]);
			fprintf(stderr, "\n");
		}
		if (device->current_layout.channel_count > 0) {
			fprintf(stderr, "  current layout: ");
			printChannelLayout(&device->current_layout);
			fprintf(stderr, "\n");
		}
		fprintf(stderr, "  sample rates:\n");
		for (int i = 0; i < device->sample_rate_count; i += 1) {
			struct SoundIoSampleRateRange *range = &device->sample_rates[i];
			fprintf(stderr, "    %d - %d\n", range->min, range->max);
		}
		if (device->sample_rate_current)
			fprintf(stderr, "  current sample rate: %d\n", device->sample_rate_current);
		fprintf(stderr, "  formats: ");
		for (int i = 0; i < device->format_count; i += 1) {
			const char *comma = (i == device->format_count - 1) ? "" : ", ";
			fprintf(stderr, "%s%s", soundio_format_string(device->formats[i]), comma);
		}
		fprintf(stderr, "\n");
		if (device->current_format != SoundIoFormatInvalid)
			fprintf(stderr, "  current format: %s\n", soundio_format_string(device->current_format));
		fprintf(stderr, "  min software latency: %0.8f sec\n", device->software_latency_min);
		fprintf(stderr, "  max software latency: %0.8f sec\n", device->software_latency_max);
		if (device->software_latency_current != 0.0)
			fprintf(stderr, "  current software latency: %0.8f sec\n", device->software_latency_current);
	}
	fprintf(stderr, "\n");
}

// Print a list of all the input devices.
void printInputDevices(SoundIo* soundio)
{
	int deviceCount = soundio_input_device_count(soundio);
	int defaultDevice = soundio_default_input_device_index(soundio);

	for (int i = 0; i < deviceCount; ++i)
	{
		SoundIoDevice* device = soundio_get_input_device(soundio, i);

		printDevice(device, i == defaultDevice);

		soundio_device_unref(device);
	}
}

static const char USAGE[] =
R"(OpusRec

    Usage:
      OpusRec record [--raw] [--rate=<hz>] [--depth=<depth>] [--channels=<channels>] [--backend=<backend>] <device_id> <output_file>
      OpusRec devices
      OpusRec (-h | --help)
      OpusRec --version

    Options:
      -h --help              Show this screen.
      --rate=<hz>            Set the sampling rate in Hz
      --depth=<depth>        Set the sample depth in bits
      --channels=<channels>  Set the number of channels
      --backend=<backend>    Set the audio system to use
      --raw                  Use the raw input from the device
)";

int main(int argc, char* argv[])
{
	// Parse the command line using the USAGE above.
	std::map<std::string, docopt::value> args =
	        docopt::docopt(USAGE,
	                       { argv + 1, argv + argc },
	                       true,             // Show help if requested
	                       "OpusRec 1.0");  // Version string

	// Print the arguments. Todo: remove.
	for(auto const& arg : args) {
		std::cout << arg.first << ": " << arg.second << std::endl;
	}

	char* exe = argv[0];

	enum SoundIoBackend backend = SoundIoBackendNone;
	string backendOpt = args["backed"] ? args["backend"].asString() : "";
	if (backendOpt != "")
	{
		if (backendOpt == "dummy")
			backend = SoundIoBackendDummy;
		else if (backendOpt == "alsa")
			backend = SoundIoBackendAlsa;
		else if (backendOpt == "pulseaudio")
			backend = SoundIoBackendPulseAudio;
		else if (backendOpt == "jack")
			backend = SoundIoBackendJack;
		else if (backendOpt == "coreaudio")
			backend = SoundIoBackendCoreAudio;
		else if (backendOpt == "wasapi")
			backend = SoundIoBackendWasapi;
		else
		{
			fprintf(stderr, "Invalid backend\n");
			return 1;
		}
	}

	// Initialise soundio.
	RecordContext rc;
	SoundIo* soundio = soundio_create();
	if (soundio == nullptr)
	{
		fprintf(stderr, "out of memory\n");
		return 1;
	}
	int err = (backend == SoundIoBackendNone) ? soundio_connect(soundio) : soundio_connect_backend(soundio, backend);
	if (err)
	{
		fprintf(stderr, "error connecting: %s", soundio_strerror(err));
		return 1;
	}
	soundio_flush_events(soundio);



	if (args["devices"].asBool())
	{
		printInputDevices(soundio);
	}
	else if (args["record"].asBool())
	{
		string device_id = args["<device_id>"].asString();
		bool is_raw = args["raw"].asBool();
		string outfile = args["<output_file>"].asString();
		int samplingRate = 44100; // args["<hz>"] ? args["<hz>"].asLong() : 48000; // Why doesn't this work?
//		int channels = args["<channels>"].asLong();

		// Find the device.
		struct SoundIoDevice* selected_device = NULL;
		for (int i = 0; i < soundio_input_device_count(soundio); ++i)
		{
			struct SoundIoDevice *device = soundio_get_input_device(soundio, i);
			cout << "Checking device id: " << device->id << ", " << device->is_raw <<
			        " against " << device_id << ", " << is_raw << endl;
			if (device->is_raw == is_raw && device->id == device_id)
			{
				selected_device = device;
				break;
			}
			soundio_device_unref(device);
		}

		if (!selected_device)
		{
			fprintf(stderr, "Invalid device id: %s\n", device_id.c_str());
			return 1;
		}

		cout << "Device: " << selected_device->name << (selected_device->is_raw ? " raw" : " not raw") << endl;


		if (selected_device->probe_error)
		{
			fprintf(stderr, "Unable to probe device: %s\n", soundio_strerror(selected_device->probe_error));
			return 1;
		}

		soundio_device_sort_channel_layouts(selected_device);

		SoundIoFormat fmt = SoundIoFormatS16LE;

		FILE* out_f = fopen(outfile.c_str(), "wb");
		if (out_f == nullptr)
		{
			fprintf(stderr, "unable to open %s: %s\n", outfile.c_str(), strerror(errno));
			return 1;
		}
		SoundIoInStream* instream = soundio_instream_create(selected_device);
		if (instream == nullptr)
		{
			fprintf(stderr, "out of memory\n");
			return 1;
		}

		cout << "Default format: " << instream->format << " sample rate: " << instream->sample_rate << endl;

		instream->format = fmt;
		instream->sample_rate = samplingRate;
		instream->read_callback = read_callback;
		instream->overflow_callback = overflow_callback;
		instream->userdata = &rc;

		cout << "Opening with format: " << instream->format << " sample rate: " << instream->sample_rate << endl;
		if ((err = soundio_instream_open(instream))) {
			fprintf(stderr, "unable to open input stream: %s", soundio_strerror(err));
			return 1;
		}
		fprintf(stderr, "%s %dHz %s interleaved\n",
		        instream->layout.name, samplingRate, soundio_format_string(fmt));
		const int ring_buffer_duration_seconds = 30;
		int capacity = ring_buffer_duration_seconds * instream->sample_rate * instream->bytes_per_frame;
		rc.ring_buffer = soundio_ring_buffer_create(soundio, capacity);
		if (!rc.ring_buffer) {
			fprintf(stderr, "out of memory\n");
			return 1;
		}
		if ((err = soundio_instream_start(instream))) {
			fprintf(stderr, "unable to start input device: %s", soundio_strerror(err));
			return 1;
		}
		// Note: in this example, if you send SIGINT (by pressing Ctrl+C for example)
		// you will lose up to 1 second of recorded audio data. In non-example code,
		// consider a better shutdown strategy.
		for (;;) {
			soundio_flush_events(soundio);
			sleep(1);
			int fill_bytes = soundio_ring_buffer_fill_count(rc.ring_buffer);
			char *read_buf = soundio_ring_buffer_read_ptr(rc.ring_buffer);
			size_t amt = fwrite(read_buf, 1, fill_bytes, out_f);
			if ((int)amt != fill_bytes) {
				fprintf(stderr, "write error: %s\n", strerror(errno));
				return 1;
			}
			soundio_ring_buffer_advance_read_ptr(rc.ring_buffer, fill_bytes);
		}
		soundio_instream_destroy(instream);
		soundio_device_unref(selected_device);


	}

	soundio_destroy(soundio);
	return 0;
}





