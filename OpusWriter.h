#pragma once

#include <string>

#include <opus.h>

#include <mkvmuxer/mkvwriter.h>

#include <memory>
#include <vector>

// Simple class to write audio to a WebM file (basically Matroska)
// encoded in Opus. It always uses 16-bit samples and supports mono
// and stereo.
class OpusReader
{
public:
	// Frame length in microseconds.
	// Frame sizes about 20ms (the default) give lower quality.
	// See https://wiki.xiph.org/Opus_Recommended_Settings#Framesize_Tweaking
	enum FrameLength
	{
		Frame_2point5ms = 2500,
		Frame_5ms = 5000,
		Frame_10ms = 10000,
		Frame_20ms = 20000,
		Frame_40ms = 40000,
		Frame_60ms = 60000,
		Frame_80ms = 60000,
		Frame_100ms = 60000,
		Frame_120ms = 60000,
	};
	
	enum SamplingRate
	{
		Rate_8000 = 8000,
		Rate_12000 = 12000,
		Rate_16000 = 16000,
		Rate_24000 = 24000,
		Rate_48000 = 48000,
	};
	
	enum Channels
	{
		Channels_Mono = 1,
		Channels_Stereo = 2,
	};
	
	enum ComputationalComplexity
	{
		Complexity_0 = 0,
		Complexity_1 = 1,
		Complexity_2 = 2,
		Complexity_3 = 3,
		Complexity_4 = 4,
		Complexity_5 = 5,
		Complexity_6 = 6,
		Complexity_7 = 7,
		Complexity_8 = 8,
		Complexity_9 = 9,
		Complexity_10 = 10,
	};
	
	OpusReader(std::string filename,
	           SamplingRate samplingRate,
	           Channels channels,
	           FrameLength frameLength,
	           int bitrate,
	           ComputationalComplexity complexity);
	~OpusReader();
	
	enum Status
	{
		Status_Ok,
		Status_Error,
		Status_OpusInvalidSamplingRate,
		Status_OpusInvalidChannelCount,
		Status_OpusInvalidFrameLength,
		Status_OpusInitialisationFailed,
		Status_OpusEncoderError,
		Status_OutputFileError,
		Status_MuxerSegmentInitialisationFailed,
		Status_MuxerError,
	};
	
	Status status() const;
	
	// Add some samples! If these are stereo they should be interleaved, starting with the left channel.
	bool write(const int16_t* samples, int sampleCount);

	// Close is called automatically on destruction.
	bool close();

private:
	OpusReader(const OpusReader&) = delete;
	OpusReader& operator=(const OpusReader&) = delete;
	
	Status mStatus = Status_Error;
	
	// This should be a unique_ptr but it's not as ergonomic to use a custom deleter.
	OpusEncoder* mEncoder = nullptr;
	mkvmuxer::MkvWriter mMuxer;
	mkvmuxer::Segment mMuxerSegment;
	bool mFinalize = false;
	
	std::vector<int16_t> mBuffer;
	
	int mChannels = 1;
	int mSamplesPerFramePerChannel = 1;
	FrameLength mFrameLength = Frame_10ms;
	
	uint64_t mTrackNumber = 0;
	
	// Time of next frame in nanoseconds.
	uint64_t mTimeCode = 0;
};
