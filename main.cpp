// This is a simple program to record from a microphone to an Opus-encoded file.

#include <soundio/soundio.h>

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
#include <atomic>

#include "CtrlC.h"
#include "RingBuffer.h"
#include "OpusWriter.h"

using namespace std;
//using namespace std::chrono_literals;

// This is passed to the read callback in the `void* userdata` pointer.
struct RecordContext
{
	explicit RecordContext(int cap) : ring_buffer(cap) {}
	RingBuffer<uint8_t> ring_buffer;
};

static int min_int(int a, int b)
{
	return (a < b) ? a : b;
}

static atomic_bool ctrlcPressed(false);

void CtrlC()
{
	cerr << "Exiting..." << endl;
	ctrlcPressed = true;
}

// This callback is called when libsoundio has some auto data to send us.
static void read_callback(SoundIoInStream* instream, int frame_count_min, int frame_count_max)
{
	// Recover the RecordContext object.
	RecordContext* rc = static_cast<RecordContext*>(instream->userdata);

	size_t free_bytes = rc->ring_buffer.free();
	int free_count = free_bytes / instream->bytes_per_frame;
	
	if (free_count < frame_count_min)
	{
		cerr << "Ring buffer overflow :(" << endl;
		exit(1);
	}

	int write_frames = min_int(free_count, frame_count_max);
	int frames_left = write_frames;
	for (;;)
	{
		int frame_count = frames_left;
		
		SoundIoChannelArea* areas = nullptr;
		
		int err = soundio_instream_begin_read(instream, &areas, &frame_count);
		if (err != SoundIoErrorNone)
		{
			cerr << "Begin read error: " << soundio_strerror(err) << endl;
			exit(1);
		}

		if (frame_count == 0)
			break;
		
		if (areas == nullptr)
		{
			// Due to an overflow there is a hole. Fill the ring buffer with
			// silence for the size of the hole.
			for (int i = 0; i < frame_count * instream->bytes_per_frame; ++i)
			{
				if (!rc->ring_buffer.push(0))
				{
					cerr << "Ring buffer overflow :/" << endl;
					exit(1);
				}
			}
		}
		else
		{
			// Copy each frame.
			for (int frame = 0; frame < frame_count; ++frame)
			{
				// Copy each channel for the frame.
				for (int ch = 0; ch < instream->layout.channel_count; ++ch)
				{
					// Copy all the sample bytes for that sample.
					for (int z = 0; z < instream->bytes_per_sample; ++z)
					{
						if (!rc->ring_buffer.push(areas[ch].ptr[z]))
						{
							cerr << "Ring buffer overflow D:" << endl;
							exit(1);
						}
					}
					areas[ch].ptr += areas[ch].step;
				}
			}
		}
		err = soundio_instream_end_read(instream);
		if (err != SoundIoErrorNone)
		{
			cerr << "End read error: " << soundio_strerror(err) << endl;
			exit(1);
		}

		frames_left -= frame_count;
		if (frames_left <= 0)
			break;
	}
}

static void overflow_callback(SoundIoInStream* instream)
{
	(void)instream;
	static int count = 0;
	cerr << "Overflow " << ++count << endl;
}

static void backend_disconnect_callback(SoundIo* soundio, int err)
{
	(void)soundio;
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

void record(SoundIo* soundio, string device_id, bool is_raw, int samplingRate, int channels, int complexity, int bitrate, int duration, string outfile)
{
	// Find the device.
	std::vector<int> selected_devices;

	for (int i = 0; i < soundio_input_device_count(soundio); ++i)
	{
		SoundIoDevice* device = soundio_get_input_device(soundio, i);
		if (device == nullptr)
		{
			cerr << "Error getting device: " << i << endl;
			return;
		}

		if (device->is_raw == is_raw && (device_id.empty() || device->id == device_id))
			selected_devices.push_back(i);

		soundio_device_unref(device);
	}

	if (selected_devices.size() != 1)
	{
		cerr << "Device not found, or too many devices: " << device_id << endl;
		return;
	}

	SoundIoDevice* device = soundio_get_input_device(soundio, selected_devices[0]);
	if (device == nullptr)
	{
		cerr << "Error getting device 0" << endl;
		return;
	}

	cout << "Device: " << device->name << (device->is_raw ? " raw" : " not raw") << endl;

	if (device->probe_error)
	{
		cerr << "Unable to probe device: " << soundio_strerror(device->probe_error) << endl;
		return;
	}

	soundio_device_sort_channel_layouts(device);

	SoundIoFormat fmt = SoundIoFormatS16LE;

	// Open the input stream.
	SoundIoInStream* instream = soundio_instream_create(device);
	if (instream == nullptr)
	{
		cerr << "Out of memory" << endl;
		return;
	}

	cout << "Default format: " << instream->format << " sample rate: " << instream->sample_rate << endl;

	int capacity = samplingRate * 30 * 4;

	RecordContext rc(capacity);

	instream->format = fmt;
	instream->sample_rate = samplingRate;
	instream->read_callback = read_callback;
	instream->overflow_callback = overflow_callback;
	instream->userdata = &rc;
	instream->layout = *soundio_channel_layout_get_default(channels);

	cout << "Opening with format: " << instream->format << " sample rate: " << instream->sample_rate << endl;

	int err = soundio_instream_open(instream);
	if (err != SoundIoErrorNone)
	{
		cerr << "Unable to open input stream: " << soundio_strerror(err) << endl;
		return;
	}

	cerr << instream->layout.name << " " << samplingRate << " Hz " << soundio_format_string(fmt) << endl;

	err = soundio_instream_start(instream);
	if (err != SoundIoErrorNone)
	{
		cerr <<  "Unable to start input device: " << soundio_strerror(err) << endl;
		return;
	}


	// Ok now initialise Opus. Default 20ms is best.
	const OpusWriter::FrameLength frameLen = OpusWriter::Frame_20ms;
	
	OpusWriter writer(outfile,
	                  static_cast<OpusWriter::SamplingRate>(samplingRate),
		              static_cast<OpusWriter::Channels>(channels),
		              frameLen,
		              bitrate,
		              static_cast<OpusWriter::ComputationalComplexity>(complexity));
	
	if (writer.status() != OpusWriter::Status_Ok)
	{
		cerr << "Opus error: " << writer.status() << endl; // TODO: Convert to readable string.
		return;
	}

	// The next frame to write.
	const int samplesPerFramePerChannel = samplingRate * frameLen / 1000000;
	// The audio from libsoundio for one frame.
	uint8_t audio_frame_input[samplesPerFramePerChannel * sizeof(int16_t) * channels];

	// Number of bytes in the frame we have filled.
	unsigned int frameFilled = 0;

	// Set up ctrl-c handler.
	SetCtrlCHandler(CtrlC);
	
	std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
	
	while (!ctrlcPressed)
	{
		int secondsPassed = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - start).count();
		cerr << secondsPassed << endl;

		if (duration >= 0 && secondsPassed >= duration)
			break;
		
		soundio_flush_events(soundio);
		
		this_thread::sleep_for(chrono::seconds(1));

		int available = rc.ring_buffer.size();

		// There are `available` bytes available.
		for (int i = 0; i < available; ++i)
		{
			if (!rc.ring_buffer.pop(audio_frame_input[frameFilled]))
			{
				// This should never happen. TODO: Make this an assert.
				cerr << "Error reading ring buffer." << endl;
				exit(1);
			}
			
			++frameFilled;

			// If the frame is full, write it.
			if (frameFilled >= sizeof(audio_frame_input))
			{
				frameFilled = 0;

				// The audio mixed down to mono.
				int16_t audio_frame[frameLen];

				for (int s = 0; s < frameLen; ++s)
				{
					// Just take the first channel for now. TODO: Fix this.
					int32_t av = 0;
					for (int c = 0; c < channels; ++c)
					{
						int idx = (s * channels + c) * sizeof(int16_t);
						int16_t sample = static_cast<int16_t>(audio_frame_input[idx+1] << 8) + audio_frame_input[idx];
						av += sample;
					}
					av /= channels;
					audio_frame[s] = av;
				}

				// Encode to Opus.
				
				writer.write(audio_frame, frameLen);
				
				if (writer.status() != OpusWriter::Status_Ok)
				{
					// TODO: Convert to string.
					cerr << "Opus writer error: " << writer.status() << endl;
					return;
				}
			}
		}
	}


	soundio_instream_destroy(instream);
	soundio_device_unref(device);
	
	if (!writer.close())
	{
		cerr << "Error closing file." << endl;
	}
}

static const char USAGE[] =
R"(OpusRec

    Usage:
      OpusRec record [--raw] [--rate=<hz>] [--channels=<n>] [--complexity=<n>] [--bitrate=<bps>] [--backend=<backend>] [--device=<id>] [--duration=<s>] <output_file>
      OpusRec devices [--backend=<backend>]
      OpusRec (-h | --help)
      OpusRec --version

    Options:
      -h --help              Show this screen.
      --version              Print the version and exit.
      --raw                  Use the raw input from the device.
      --rate=<hz>            Set the sampling rate in Hz. Must be one of 8000, 12000, 16000, 24000, or 48000. Defaults to the highest supported value.
      --channels=<channels>  Set the number of channels. Must be 2 or 1. Defaults to the highest supported number. If the device supports stereo and you use --channels 1 it will be downmixed.
      --complexity=<n>       An integer from 0-10 inclusive. The computational effort that is used for encoding. Default 7.
      --bitrate=<bps>        Average bitrate in bits per second. Default 64000.
      --backend=<backend>    Set the audio system to use. Defaults to the first one that works.
      --device=<device_id>   Select a specific device from its device ID (use `OpusRec devices`). Required if there is more than one device.
      --duration=<s>         Stop recording after the given number of seconds. Default to infinite (stop with Ctrl-C).
)";

static const std::map<std::string, SoundIoBackend> backends = {
    {"dummy", SoundIoBackendDummy},
    {"alsa", SoundIoBackendAlsa},
    {"pulseaudio", SoundIoBackendPulseAudio},
    {"jack", SoundIoBackendJack},
    {"coreaudio", SoundIoBackendCoreAudio},
    {"wasapi", SoundIoBackendWasapi},
};

int main(int argc, char* argv[])
{
	// Parse the command line using the USAGE above.
	std::map<std::string, docopt::value> args =
	        docopt::docopt(USAGE,
	                       { argv + 1, argv + argc },
	                       true,             // Show help if requested
	                       "OpusRec 1.0");  // Version string
	
	auto stringOpt = [&](string key, string def) -> string {
		if (args.count(key) != 1)
			return def;
		if (!args[key].isString())
			return def;
		return args[key].asString();
	};
	auto intOpt = [&](string key, int def) -> int {
		if (args.count(key) != 1)
			return def;
		if (!args[key].isString())
			return def;
		string val = args[key].asString();
		try
		{
			size_t processed = 0;
			int x = std::stoi(val, &processed, 10);
			if (processed != val.size())
				return def;
			return x;
		}
		catch (std::exception& e)
		{
		}
		return def;
	};
	
	enum SoundIoBackend backend = SoundIoBackendNone;
	string backendOpt = args["--backend"].isString() ? args["--backend"].asString() : "";
	
//	for (auto&& it : args) cerr << it.first << ": " << it.second << endl;
	
	if (backendOpt != "")
	{
		if (backends.count(backendOpt) != 1)
		{
			cerr << "Invalid backend: " << backendOpt << endl;
			cerr << "Valid options: \n\n";
			for (auto it : backends)
			{
				cerr << "    " << it.first;
				if (!soundio_have_backend(it.second))
					cerr << " [not supported]";
				cerr << endl;
			}
			return 1;
		}
		backend = backends.at(backendOpt);
		if (!soundio_have_backend(backend))
		{
			cerr << "Backend not supported." << endl;
			return 1;
		}
	}

	// Initialise soundio.
	SoundIo* soundio = soundio_create();
	if (soundio == nullptr)
	{
		cerr << "Error initialising libsoundio: Out of memory" << endl;
		return 1;
	}

	soundio->on_backend_disconnect = backend_disconnect_callback;

	cerr << "Connecting to backend: " << (backendOpt == "" ? "default" : backendOpt) << endl;

	int err = (backend == SoundIoBackendNone) ? soundio_connect(soundio) : soundio_connect_backend(soundio, backend);
	if (err)
	{
		cerr << "Error connecting to backend: " << soundio_strerror(err) << endl;
		return 1;
	}

	soundio_flush_events(soundio);

	if (args["devices"].asBool())
	{
		printInputDevices(soundio);
	}
	else if (args["record"].asBool())
	{
		string device_id = args["--device"].isString() ? args["--device"].asString() : "";
		bool is_raw = args["--raw"].isBool() ? args["--raw"].asBool() : false;
		int samplingRate = intOpt("--rate", 48000);
		int channels = intOpt("--channels", 2);
		int complexity = intOpt("--complexity", 10);
		int bitrate = intOpt("--bitrate", 64000);
		int duration = intOpt("--duration", -1);
		string outfile = stringOpt("<output_file>", "");
		
		cerr << "Duration: " << duration << endl;

		record(soundio, device_id, is_raw, samplingRate, channels, complexity, bitrate, duration, outfile);
	}

	soundio_destroy(soundio);

	return 0;
}

