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
#include <thread>
#include <chrono>

using namespace std;
using namespace std::chrono_literals;

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
static void read_callback(SoundIoInStream* instream, int frame_count_min, int frame_count_max)
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
		cerr << "Ring buffer overflow" << endl;
		exit(1);
	}

	int write_frames = min_int(free_count, frame_count_max);
	int frames_left = write_frames;
	for (;;)
	{
		int frame_count = frames_left;
		if ((err = soundio_instream_begin_read(instream, &areas, &frame_count)))
		{
			cerr << "Begin read error: " << soundio_strerror(err) << endl;
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
			cerr << "End read error: " << soundio_strerror(err) << endl;
			exit(1);
		}

		frames_left -= frame_count;
		if (frames_left <= 0)
			break;
	}

	int advance_bytes = write_frames * instream->bytes_per_frame;
	soundio_ring_buffer_advance_write_ptr(rc->ring_buffer, advance_bytes);
}

static void overflow_callback(SoundIoInStream* instream)
{
	(void)instream;
	static int count = 0;
	cerr << "Overflow " << ++count << endl;
}

static void backend_disconnect_callback(SoundIo* soundio, int err)
{
	cerr << "Backend disconnected: " << soundio_strerror(err) << endl;
	exit(1);
}

static void printChannelLayout(const SoundIoChannelLayout* layout)
{
	if (layout->name != nullptr)
	{
		cerr << layout->name << endl;
	}
	else
	{
		for (int i = 0; i < layout->channel_count; ++i)
			cerr << soundio_get_channel_name(layout->channels[i]) << endl;
	}
}

static void printDevice(SoundIoDevice* device, bool is_default)
{
	const char* default_str = is_default ? " (default)" : "";
	const char* raw_str = device->is_raw ? " (raw)" : "";
	cerr << device->name << default_str << raw_str << endl;

	cerr << "  id: " << device->id << endl;
	if (device->probe_error)
	{
		cerr << "  probe error: " << soundio_strerror(device->probe_error) << endl;
	}
	else
	{
		cerr << "  channel layouts:" << endl;
		for (int i = 0; i < device->layout_count; ++i)
		{
			cerr << "    ";
			printChannelLayout(&device->layouts[i]);
			cerr << endl;
		}
		if (device->current_layout.channel_count > 0)
		{
			cerr << "  current layout: ";
			printChannelLayout(&device->current_layout);
			cerr << endl;
		}

		cerr << "  sample rates:" << endl;
		for (int i = 0; i < device->sample_rate_count; ++i)
		{
			SoundIoSampleRateRange *range = &device->sample_rates[i];
			cerr << "    " << range->min << " - " << range->max << endl;
		}

		if (device->sample_rate_current != 0)
			cerr << "  current sample rate: " << device->sample_rate_current << endl;

		cerr << "  formats: ";
		for (int i = 0; i < device->format_count; ++i)
		{
			cerr << soundio_format_string(device->formats[i]);
			if (i < device->format_count - 1)
				cerr << ",";
		}
		cerr << endl;

		if (device->current_format != SoundIoFormatInvalid)
			cerr << "  current format: " << soundio_format_string(device->current_format) << endl;

		cerr << "  min software latency: " << device->software_latency_min << endl;
		cerr << "  max software latency: " << device->software_latency_max << endl;

		if (device->software_latency_current != 0.0)
			cerr << "  current software latency: " << device->software_latency_current << endl;
	}
	cerr << endl;
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
//	for(auto const& arg : args) {
//		std::cout << arg.first << ": " << arg.second << std::endl;
//	}

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
		cerr << "Out of memory" << endl;
		return 1;
	}

	soundio->on_backend_disconnect = backend_disconnect_callback;

	cerr << "Connecting to backend: " << (backendOpt == "" ? "default" : backendOpt) << endl;
	int err = (backend == SoundIoBackendNone) ? soundio_connect(soundio) : soundio_connect_backend(soundio, backend);
	if (err)
	{
		cerr << "Error connecting: " << soundio_strerror(err) << endl;
		return 1;
	}
	cerr << "Flushing events" << endl;
	soundio_flush_events(soundio);

	if (args["devices"].asBool())
	{
		cerr << "Printing devices" << endl;
		printInputDevices(soundio);
	}
	else if (args["record"].asBool())
	{
		string device_id = args["<device_id>"].asString();
		bool is_raw = args["raw"].asBool();
		int samplingRate = args["<hz>"] ? args["<hz>"].asLong() : 44100;
		int channels = args["<channels>"].asLong();
		string outfile = args["<output_file>"].asString();

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
			cerr << "Invalid device id: " << device_id << endl;
			return 1;
		}

		cout << "Device: " << selected_device->name << (selected_device->is_raw ? " raw" : " not raw") << endl;

		if (selected_device->probe_error)
		{
			cerr << "Unable to probe device: " << soundio_strerror(selected_device->probe_error) << endl;
			return 1;
		}

		soundio_device_sort_channel_layouts(selected_device);

		SoundIoFormat fmt = SoundIoFormatS16LE;

		FILE* out_f = fopen(outfile.c_str(), "wb");
		if (out_f == nullptr)
		{
			cerr << "Unable to open " << outfile << ": " << strerror(errno) << endl;
			return 1;
		}
		SoundIoInStream* instream = soundio_instream_create(selected_device);
		if (instream == nullptr)
		{
			cerr << "Out of memory" << endl;
			return 1;
		}

		cout << "Default format: " << instream->format << " sample rate: " << instream->sample_rate << endl;

		instream->format = fmt;
		instream->sample_rate = samplingRate;
		instream->read_callback = read_callback;
		instream->overflow_callback = overflow_callback;
		instream->userdata = &rc;
		instream->layout = *soundio_channel_layout_get_default(channels);

		cout << "Opening with format: " << instream->format << " sample rate: " << instream->sample_rate << endl;
		err = soundio_instream_open(instream);
		if (err != SoundIoErrorNone)
		{
			cerr << "Unable to open input stream: " << soundio_strerror(err) << endl;
			return 1;
		}

		cerr << instream->layout.name << samplingRate << " Hz " << soundio_format_string(fmt) << " interleaved" << endl;

		const int ring_buffer_duration_seconds = 30;
		int capacity = ring_buffer_duration_seconds * instream->sample_rate * instream->bytes_per_frame;
		rc.ring_buffer = soundio_ring_buffer_create(soundio, capacity);
		if (rc.ring_buffer == nullptr)
		{
			cerr << "Out of memory" << endl;
			return 1;
		}

		err = soundio_instream_start(instream);
		if (err != SoundIoErrorNone)
		{
			cerr <<  "Unable to start input device: " << soundio_strerror(err) << endl;
			return 1;
		}

		// Note: in this example, if you send SIGINT (by pressing Ctrl+C for example)
		// you will lose up to 1 second of recorded audio data. In non-example code,
		// consider a better shutdown strategy.
		for (;;)
		{
			soundio_flush_events(soundio);
			this_thread::sleep_for(1s);

			int fill_bytes = soundio_ring_buffer_fill_count(rc.ring_buffer);
			char* read_buf = soundio_ring_buffer_read_ptr(rc.ring_buffer);

			size_t amt = fwrite(read_buf, 1, fill_bytes, out_f);

			if ((int)amt != fill_bytes)
			{
				cerr << "Write error: " << strerror(errno) << endl;
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





