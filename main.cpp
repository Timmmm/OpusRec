// This is a simple program to record from a microphone to an Opus-encoded file.

#include <soundio/soundio.h>

#include <opus.h>

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

// libwebm
#include <mkvparser/mkvparser.h>
#include <mkvparser/mkvreader.h>
#include <mkvmuxer/mkvmuxer.h>
#include <mkvmuxer/mkvmuxertypes.h>
#include <mkvmuxer/mkvwriter.h>

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


// See https://tools.ietf.org/html/rfc7845 Section 5.1
//
// outputGain is Q7.8 in dB, recommended to be 0.
vector<uint8_t> OpusHeader(uint8_t channelCount, uint16_t preSkipSamples, uint32_t inputSampleRate, uint16_t outputGain)
{
	vector<uint8_t> head(19);

	head[0] = 'O';
	head[1] = 'p';
	head[2] = 'u';
	head[3] = 's';
	head[4] = 'H';
	head[5] = 'e';
	head[6] = 'a';
	head[7] = 'd';
	head[8] = 1; // Version
	head[9] = channelCount;
	head[10] = (preSkipSamples >> 0) & 0xFF;
	head[11] = (preSkipSamples >> 8) & 0xFF;
	head[12] = (inputSampleRate >> 0) & 0xFF;
	head[13] = (inputSampleRate >> 8) & 0xFF;
	head[14] = (inputSampleRate >> 16) & 0xFF;
	head[15] = (inputSampleRate >> 24) & 0xFF;
	head[16] = (outputGain >> 0) & 0xFF;
	head[17] = (outputGain >> 8) & 0xFF;
	head[18] = 0; // Mapping family - none.

	return head;
}


void record(SoundIo* soundio, string device_id, bool is_raw, int samplingRate, int channels, int complexity, int bitrate, string outfile)
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

	// Open the output file.
	FILE* out_f = fopen(outfile.c_str(), "wb");
	if (out_f == nullptr)
	{
		cerr << "Unable to open " << outfile << ": " << strerror(errno) << endl;
		return;
	}

	// Open the input stream.
	SoundIoInStream* instream = soundio_instream_create(device);
	if (instream == nullptr)
	{
		cerr << "Out of memory" << endl;
		return;
	}

	cout << "Default format: " << instream->format << " sample rate: " << instream->sample_rate << endl;

	RecordContext rc;

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

	cerr << instream->layout.name << samplingRate << " Hz " << soundio_format_string(fmt) << " interleaved" << endl;

	const int ring_buffer_duration_seconds = 30;
	int capacity = ring_buffer_duration_seconds * instream->sample_rate * instream->bytes_per_frame;
	rc.ring_buffer = soundio_ring_buffer_create(soundio, capacity);
	if (rc.ring_buffer == nullptr)
	{
		cerr << "Out of memory" << endl;
		return;
	}

	err = soundio_instream_start(instream);
	if (err != SoundIoErrorNone)
	{
		cerr <<  "Unable to start input device: " << soundio_strerror(err) << endl;
		return;
	}


	// Ok now initialise Opus.
	const int frameLenMs = 20; // Must be 2.5, 5, 10, 20, 40 or 60.

	// samplingRate must be one of 8000, 12000, 16000, 24000, or 48000.
	if (samplingRate != 8000 && samplingRate != 12000 && samplingRate != 16000 && samplingRate != 24000 && samplingRate != 48000)
	{
		cerr << "Unsupported sampling rate: " << samplingRate << endl;
		return;
	}
	// Channels must be 1 or 2 apparently. I guess to do 5.1 or whatever you encode the channels in pairs.
//	if (channels != 1 && channels != 2)
//	{
//		cerr << "Unsupported channel count: " << channels << endl;
//		return;
//	}

	// We'll just mix down to 1 channel anyway.

	// Opus encoder initialisation.
	int error = OPUS_INTERNAL_ERROR;
	OpusEncoder* enc = opus_encoder_create(samplingRate, 1, OPUS_APPLICATION_AUDIO, &error);
	if (error != OPUS_OK)
	{
		cerr << "Error initialising Opus: " << error << endl;
		return;
	}

	// Set bitrate etc.
	opus_encoder_ctl(enc, OPUS_SET_BITRATE(bitrate)); // In bits per second
	opus_encoder_ctl(enc, OPUS_SET_COMPLEXITY(complexity)); // 0-10.
	opus_encoder_ctl(enc, OPUS_SET_SIGNAL(OPUS_AUTO)); // Can set to voice or music manually.

	// Now initialise WebM.

	mkvmuxer::MkvWriter writer;

	if (!writer.Open(outfile.c_str()))
	{
		cerr << "Error opening output file" << endl;
		return;
	}

	// WebM files have one segment.
	mkvmuxer::Segment muxer_segment;
	if (!muxer_segment.Init(&writer))
	{
		cerr << "Could not initialise muxer segment. Whatever that means." << endl;
		return;
	}

	// Flag whether or not the muxer should output a Cues element. It's like an index. Not sure we need it for audio.
//	muxer_segment.OutputCues(true);

	mkvmuxer::SegmentInfo* const info = muxer_segment.GetSegmentInfo();
	info->set_writing_app("OpusRec");

	// Add an audio track.
	uint64_t aud_track = muxer_segment.AddAudioTrack(samplingRate, channels, 0); // 0 means "next available" I think.
	if (aud_track == 0)
	{
		cerr << "Could not add audio track." << endl;
		return;
	}
	mkvmuxer::AudioTrack* audio = static_cast<mkvmuxer::AudioTrack*>(muxer_segment.GetTrackByNumber(aud_track));
	if (audio == nullptr)
	{
		cerr << "Could not get audio track." << endl;
		return;
	}

	audio->set_codec_id(mkvmuxer::Tracks::kOpusCodecId);

	audio->set_bit_depth(16);

	// Delay built into the code during decoding in nanoseconds.
	audio->set_codec_delay(6500000);

	// ?
	audio->set_seek_pre_roll(80000000);

	vector<uint8_t> opushead = OpusHeader(channels, 0, samplingRate, 0);
	audio->SetCodecPrivate(opushead.data(), opushead.size());

	// The next frame to write.
	const int frameLen = samplingRate * frameLenMs / 1000;
	// The audio from libsoundio for one frame.
	uint8_t audio_frame_input[frameLen * sizeof(int16_t) * channels];

	// Number of bytes in the frame we have filled.
	int frameFilled = 0;

	// Frame time in nanoseconds.
	uint64_t timecode = 0;

	// Set up ctrl-c handler.

	SetCtrlCHandler(CtrlC);

	// Note: in this example, if you send SIGINT (by pressing Ctrl+C for example)
	// you will lose up to 1 second of recorded audio data. In non-example code,
	// consider a better shutdown strategy.
	while (!ctrlcPressed)
	{
		soundio_flush_events(soundio);
		this_thread::sleep_for(1s);

		int fill_bytes = soundio_ring_buffer_fill_count(rc.ring_buffer);
		char* read_buf = soundio_ring_buffer_read_ptr(rc.ring_buffer);

		// The audio data is now in: read_buf, and there are fill_bytes of it.
		for (int i = 0; i < fill_bytes; ++i)
		{
			audio_frame_input[frameFilled] = read_buf[i];
			++frameFilled;

			// If the frame is full, write it.
			if (frameFilled >= sizeof(audio_frame_input))
			{
				frameFilled = 0;

				// The audio mixed down to mono.
				int16_t audio_frame[frameLen];

				for (int s = 0; s < frameLen; ++s)
				{
					// Just take the first channel for now.
					int o = s * sizeof(int16_t) * channels;
					audio_frame[s] = static_cast<int16_t>(audio_frame_input[o+1] << 8) + audio_frame_input[o];
				}

				// Encode to Opus.
				const int max_packet = 1024 * 128;
				uint8_t packet[max_packet];

				opus_int32 len = opus_encode(enc, reinterpret_cast<const int16_t*>(audio_frame), frameLen, packet, max_packet);
				if (len < 0)
				{
					// Error.
					cerr << "Opus encoder error" << endl;
					return;
				}
				if (len <= 2)
				{
					// Ignore packet (it's silent).
					// TODO: How do you handle this?
				}

				// Save it into a WebM stream.
				mkvmuxer::Frame muxer_frame;
				if (!muxer_frame.Init(packet, len))
				{
					cerr << "Couldn't initialise muxer frame." << endl;
					return;
				}

				muxer_frame.set_track_number(aud_track);
				muxer_frame.set_timestamp(timecode); // In nanoseconds.

				timecode += frameLenMs * 1000000;

				muxer_frame.set_is_key(true); // Does this do anything for audio?

				if (!muxer_segment.AddGenericFrame(&muxer_frame))
				{
					cerr << "Could not add frame." << endl;
					return;
				}
			}
		}

		soundio_ring_buffer_advance_read_ptr(rc.ring_buffer, fill_bytes);
	}


	// Destroy Opus encoder.
	opus_encoder_destroy(enc);

	if (!muxer_segment.Finalize())
	{
		cerr << "Finalization of segment failed." << endl;
		return;
	}

	writer.Close();

	soundio_instream_destroy(instream);
	soundio_device_unref(device);
}


void OpusWebMTest()
{
	const int samplingRate = 44100;
	const int channels = 1;

	const int frameLenMs = 20; // Must be 2.5, 5, 10, 20, 40 or 60.

	// samplingRate must be one of 8000, 12000, 16000, 24000, or 48000.
	if (samplingRate != 8000 && samplingRate != 12000 && samplingRate != 16000 && samplingRate != 24000 && samplingRate != 48000)
	{
		cerr << "Unsupported sampling rate: " << samplingRate << endl;
		return;
	}
	// Channels must be 1 or 2 apparently. I guess to do 5.1 or whatever you encode the channels in pairs.
	if (channels != 1 && channels != 2)
	{
		cerr << "Unsupported channel count: " << channels << endl;
		return;
	}

	// Opus encoder initialisation.
	int error = OPUS_INTERNAL_ERROR;
	OpusEncoder* enc = opus_encoder_create(samplingRate, channels, OPUS_APPLICATION_AUDIO, &error);
	if (error != OPUS_OK)
	{
		cerr << "Error initialising Opus: " << error << endl;
		return;
	}

	// Set bitrate etc.
	opus_encoder_ctl(enc, OPUS_SET_BITRATE(32000)); // In bits per second
	opus_encoder_ctl(enc, OPUS_SET_COMPLEXITY(8)); // 0-10.
	opus_encoder_ctl(enc, OPUS_SET_SIGNAL(OPUS_AUTO)); // Can set to voice or music manually.

	double t = 0.0;

	vector<vector<uint8_t>> packets;

	for (int i = 0; i < 100; ++i)
	{
		// Encode a frame. This has to be exactly one frame.
		// A frame can be 2.5, 5, 10, 20, 40 or 60 ms of audio.

		const int frameLen = samplingRate * frameLenMs / 1000;
		opus_int16 audio_frame[frameLen];
		for (int f = 0; f < frameLen; ++f)
		{
			double val = sin(t) + cos(2*t);
			audio_frame[f] = static_cast<opus_int16>(val * 0x1000);
			t += 0.05;
		}

		const int max_packet = 1024 * 128;
		uint8_t packet[max_packet];

		opus_int32 len = opus_encode(enc, audio_frame, frameLen, packet, max_packet);
		if (len < 0)
		{
			// Error.
			cerr << "Opus encoder error" << endl;
			return;
		}
		if (len <= 2)
		{
			// Ignore packet (it's silent).
		}
		packets.emplace_back(packet, packet + len);
	}

	// Destroy encoder.
	opus_encoder_destroy(enc);

	// Now save the packets using WebM.

	string outfile = "test.webm";

	// Write shit to a webm file.
	mkvmuxer::MkvWriter writer;

	if (!writer.Open(outfile.c_str()))
	{
		cerr << "Error opening output file" << endl;
		return;
	}

	// WebM files have one segment.
	mkvmuxer::Segment muxer_segment;
	if (!muxer_segment.Init(&writer))
	{
		cerr << "Could not initialise muxer segment. Whatever that means." << endl;
		return;
	}

	// Flag whether or not the muxer should output a Cues element.
	muxer_segment.OutputCues(true);

	mkvmuxer::SegmentInfo* const info = muxer_segment.GetSegmentInfo();
	info->set_writing_app("OpusRec");

	// Add an audio track.
	uint64_t aud_track = muxer_segment.AddAudioTrack(samplingRate, channels, 1); // 0 means "next available" I think.
	if (aud_track == 0)
	{
		cerr << "Could not add audio track." << endl;
		return;
	}
	mkvmuxer::AudioTrack* audio = static_cast<mkvmuxer::AudioTrack*>(muxer_segment.GetTrackByNumber(aud_track));
	if (audio == nullptr)
	{
		cerr << "Could not get audio track." << endl;
		return;
	}

	audio->set_codec_id(mkvmuxer::Tracks::kOpusCodecId);

	audio->set_bit_depth(16);

	// Delay built into the code during decoding in nanoseconds.
	audio->set_codec_delay(6500000);

	// ?
	audio->set_seek_pre_roll(80000000);

	vector<uint8_t> opushead = OpusHeader(channels, 0, samplingRate, 0);
	audio->SetCodecPrivate(opushead.data(), opushead.size());

	for (int i = 0; i < 100; ++i)
	{
		// Add each packet as a frame.

		mkvmuxer::Frame muxer_frame;
		if (!muxer_frame.Init(packets[i].data(), packets[i].size()))
		{
			cerr << "Couldn't initialise muxer frame." << endl;
			return;
		}

		muxer_frame.set_track_number(aud_track);
		muxer_frame.set_timestamp(i * frameLenMs * 1000000); // In nanoseconds.

		muxer_frame.set_is_key(true); // Does this do anything for audio?

		if (!muxer_segment.AddGenericFrame(&muxer_frame))
		{
			cerr << "Could not add frame." << endl;
			return;
		}
	}

	// The duration does not need to be explicitly set.
//	muxer_segment.set_duration(100 * frameLenMs * 0.001);
	if (!muxer_segment.Finalize())
	{
		cerr << "Finalization of segment failed." << endl;
		return;
	}

	writer.Close();


}

static const char USAGE[] =
R"(OpusRec

    Usage:
      OpusRec record [--raw] [--rate=<hz>] [--channels=<n>] [--complexity=<n>] [--bitrate=<bps>] [--backend=<backend>] [--device=<id>] <output_file>
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
//	OpusWebMTest();
//	return 0;

	// Parse the command line using the USAGE above.
	std::map<std::string, docopt::value> args =
	        docopt::docopt(USAGE,
	                       { argv + 1, argv + argc },
	                       true,             // Show help if requested
	                       "OpusRec 1.0");  // Version string
	
	enum SoundIoBackend backend = SoundIoBackendNone;
	string backendOpt = args["--backend"].isString() ? args["--backend"].asString() : "";
	
	
	
	if (backendOpt != "")
	{
		if (backends.count(backendOpt) != 1)
		{
			cerr << "Invalid backend: " << backendOpt << endl;
			cerr << "Valid options: ";
			for (auto it : backends)
			{
				cerr << it.first;
				if (!soundio_have_backend(it.second))
					cerr << " [not supported]";
				cerr << endl;
			}
			return 1;
		}
		backend = backends[backendOpt];
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
		int samplingRate = args["--rate"] ? args["--rate"].asLong() : 44100;
		int channels = args["--channels"].isLong() ? args["--channels"].asLong() : 2;
		int complexity = args["--complexity"].isLong() ? args["--complexity"].asLong() : 7;
		int bitrate = args["--bitrate"].isLong() ? args["--bitrate"].asLong() : 64000;
		string outfile = args["<output_file>"].isString() ? args["<output_file>"].asString() : "";
		cout << outfile << endl;

		record(soundio, device_id, is_raw, samplingRate, channels, complexity, bitrate, outfile);
	}

	soundio_destroy(soundio);

	return 0;
}

