#ifndef STEGODISK_CARRIERFILES_CARRIERFILEFFMPEG_H_
#define STEGODISK_CARRIERFILES_CARRIERFILEFFMPEG_H_

#include <stdint.h>

#include <iostream>
#include <string>
#include <memory>
#include <functional>

extern "C" {
	#include <libavformat/avformat.h>
}

namespace stego_disk {

class FfFormatContext {
public:
	typedef std::function<void(AVPacket *)> PacketHandler;
public:
	FfFormatContext();
	FfFormatContext(FfFormatContext &) = delete;
	~FfFormatContext();

    void OpenInputFile(const std::string & path);
    void OpenOutputFile(const std::string & path);

	void InitOutputFileFromFromInputFIle(FfFormatContext & input_format);
	void CopyDataFromInputFileWithModifier(FfFormatContext & input_format, const PacketHandler & modifier);

	void IterateOverStream(const PacketHandler & hndl);

private:
	void ComputeCapacity();
	void OpenInputFile_internal(const std::string & path);

private:
	AVFormatContext * format_ = nullptr;
	size_t capacity_ = 0;
	std::string path_;
	bool opened_output_file_ = false;
	bool opened_input_file_ = false;
};

} // stego_disk

#endif // STEGODISK_CARRIERFILES_CARRIERFILEFFMPEG_H_
