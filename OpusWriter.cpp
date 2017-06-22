#include "OpusWriter.h"

// See https://tools.ietf.org/html/rfc7845 Section 5.1
//
// outputGain is Q7.8 in dB, recommended to be 0.
static std::vector<uint8_t> OpusHeader(uint8_t channelCount, uint16_t preSkipSamples, uint32_t inputSampleRate, uint16_t outputGain)
{
	std::vector<uint8_t> head(19);

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

OpusReader::~OpusReader()
{
	close();
	if (mEncoder != nullptr)
		opus_encoder_destroy(mEncoder);
}

OpusReader::OpusReader(std::string filename,
                       OpusReader::SamplingRate samplingRate,
                       OpusReader::Channels channels,
                       OpusReader::FrameLength frameLength,
                       int bitrate,
                       OpusReader::ComputationalComplexity complexity)
{
	// samplingRate must be one of 8000, 12000, 16000, 24000, or 48000.
	if (samplingRate != Rate_8000 &&
	    samplingRate != Rate_12000 &&
	    samplingRate != Rate_16000 &&
	    samplingRate != Rate_24000 &&
	    samplingRate != Rate_48000)
	{
		mStatus = Status_OpusInvalidSamplingRate;
		return;
	}
	
	if (channels != 1 && channels != 2)
	{
		mStatus = Status_OpusInvalidChannelCount;
		return;
	}
	
	if (frameLength != Frame_2point5ms &&
	    frameLength != Frame_5ms &&
	    frameLength != Frame_10ms &&
	    frameLength != Frame_20ms &&
	    frameLength != Frame_40ms &&
	    frameLength != Frame_60ms &&
	    frameLength != Frame_80ms &&
	    frameLength != Frame_100ms &&
	    frameLength != Frame_120ms)
	{
		mStatus = Status_OpusInvalidFrameLength;
		return;
	}

	mChannels = channels;
	// This always works out to an integer:
	//
	//                     Sampling Rate (Hz): 8000   12000   16000   24000   48000
	// Frame Length (us)
	// 2500                                      20      30      40      60     120
	// 5000                                      40      60      80     120     240
	// 10000                                     80     120     160     240     480
	// 20000                                    160     240     320     480     960
	// 40000                                    320     480     640     960    1920
	// 60000                                    480     720     960    1440    2880
	//
	mSamplesPerFramePerChannel = frameLength * samplingRate / 1000000;
	mFrameLength = frameLength;
	
	// Opus initialisation.
	int error = OPUS_INTERNAL_ERROR;
	mEncoder = opus_encoder_create(samplingRate, channels, OPUS_APPLICATION_AUDIO, &error);
	
	if (error != OPUS_OK || mEncoder == nullptr)
	{
		mStatus = Status_OpusInitialisationFailed;
		return;
	}

	// Set bitrate.
	error = opus_encoder_ctl(mEncoder, OPUS_SET_BITRATE(bitrate));
	if (error != OPUS_OK)
	{
		mStatus = Status_OpusInitialisationFailed;
		return;
	}
	
	// Set the computational complexity.
	error = opus_encoder_ctl(mEncoder, OPUS_SET_COMPLEXITY(complexity));
	if (error != OPUS_OK)
	{
		mStatus = Status_OpusInitialisationFailed;
		return;
	}
	
	// Set it to automatically switch between voice and music modes.
	error = opus_encoder_ctl(mEncoder, OPUS_SET_SIGNAL(OPUS_AUTO));
	if (error != OPUS_OK)
	{
		mStatus = Status_OpusInitialisationFailed;
		return;
	}
	
	// Now initialise WebM.
	if (!mMuxer.Open(filename.c_str()))
	{
		mStatus = Status_OutputFileError;
		return;
	}

	// WebM files have one segment.
	if (!mMuxerSegment.Init(&mMuxer))
	{
		mStatus = Status_MuxerSegmentInitialisationFailed;
		return;
	}
	
	mkvmuxer::SegmentInfo* info = mMuxerSegment.GetSegmentInfo();
	if (info == nullptr)
	{
		mStatus = Status_MuxerSegmentInitialisationFailed;
		return;
	}
	
	info->set_writing_app("OpusRec");
	
	// Add an audio track.
	mTrackNumber = mMuxerSegment.AddAudioTrack(samplingRate, channels, 0); // 0 allows the muxer to device on track number.
	if (mTrackNumber == 0)
	{
		mStatus = Status_MuxerError;
		return;
	}
	
	mkvmuxer::AudioTrack* audio = static_cast<mkvmuxer::AudioTrack*>(mMuxerSegment.GetTrackByNumber(mTrackNumber));
	if (audio == nullptr)
	{
		mStatus = Status_MuxerError;
		return;
	}

	audio->set_codec_id(mkvmuxer::Tracks::kOpusCodecId);
	audio->set_bit_depth(16);

	// Delay built into the code during decoding in nanoseconds.
	audio->set_codec_delay(6500000); // TODO: How do I know this?

	// Amount of audio to discard after a seek, or something like that.
	audio->set_seek_pre_roll(80000000); // TODO: How do I know this?

	std::vector<uint8_t> opushead = OpusHeader(channels, 0, samplingRate, 0);
	if (!audio->SetCodecPrivate(opushead.data(), opushead.size()))
	{
		mStatus = Status_MuxerError;
		return;
	}
	
	mFinalize = true;
	mStatus = Status_Ok;
}

OpusReader::Status OpusReader::status() const
{
	return mStatus;
}

bool OpusReader::write(const int16_t *samples, int sampleCount)
{
	if (mEncoder == nullptr)
		return false;
	
	// We can only encode an entire frame, so store it in an internal buffer.
	mBuffer.insert(mBuffer.end(), samples, samples + sampleCount);
	
	// Now encode as many frames as we can.
	while (mBuffer.size() >= mSamplesPerFramePerChannel * mChannels)
	{
		// Encode to Opus.
		const int maxPacketLength = 1024 * 64;
		uint8_t packet[maxPacketLength];

		opus_int32 len = opus_encode(mEncoder, mBuffer.data(), mSamplesPerFramePerChannel, packet, maxPacketLength);
		if (len < 0)
		{
			mStatus = Status_OpusEncoderError;
			return false;
		}
		
		// We are allowed to ignore packets shorter than or equal to 2 bytes.
		if (len > 2)
		{
			mkvmuxer::Frame frame;
			if (!frame.Init(packet, len))
			{
				mStatus = Status_MuxerError;
				return false;
			}
	
			frame.set_track_number(mTrackNumber);
			frame.set_timestamp(mTimeCode); // In nanoseconds.
	
			frame.set_is_key(true); // Does this do anything for audio?
	
			if (!mMuxerSegment.AddGenericFrame(&frame))
			{
				mStatus = Status_MuxerError;
				return false;
			}
		}
		
		mTimeCode += mFrameLength * 1000;
	}
	return true;
}

bool OpusReader::close()
{
	bool success = true;
	if (mFinalize)
	{
		success = mMuxerSegment.Finalize();
		mMuxer.Close();
		mFinalize = false;
	}
	return success;
}

