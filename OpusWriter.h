#pragma once

#include <string>

// Simple class to write audio to a WebM file (basically Matroska)
// encoded in Opus.
class OpusWriter
{
public:
	enum FrameLength
	{
		Frame_2point5ms,
		Frame_5ms,
		Frame_10ms,
		Frame_20ms,
		Frame_40ms,
		Frame_60ms,
	};

	static OpusWriter create(std::string filename,
	                        int samplingRate,
	                        int channels,
	                        int bitDepth,
	                        FrameLength frameLength,
	                        int bitrate,
	                        int complexity
	                        );

	~OpusWriter();

	void write(int16_t* samples, int sampleCount);

	// Close is called automatically on destruction.
	void close();

private:
	OpusWriter();

};
