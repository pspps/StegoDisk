#include "carrier_files/carrier_file_ffmpeg.h"

extern "C"
{
	#include <libavutil/opt.h>
	#include <libavcodec/avcodec.h>
	#include <libavutil/avutil.h>
	#include <libavformat/avformat.h>
	#include <libswscale/swscale.h>
}

namespace stego_disk {

class InitFfmpeg {
private:
	class InitImpl {
	public:
		InitImpl() {
			av_register_all();
		}
	};

	static InitImpl instance_;
};
InitFfmpeg::InitImpl InitFfmpeg::instance_ = InitFfmpeg::InitImpl();


FfFormatContext::FfFormatContext() {
}

FfFormatContext::~FfFormatContext() {
	if (opened_output_file_) {
		av_write_trailer(format_);
	}
	if (opened_output_file_) {
		avformat_close_input(&format_);
	}
}

void FfFormatContext::OpenInputFile_internal(const std::string & path) {
	format_ = avformat_alloc_context();
	if (!format_) {
		std::cerr << "Cannot alloc AVFormatContext" << std::endl;
		throw std::exception();
	}

	int ec = avformat_open_input(&format_, path.c_str(), NULL, NULL);
	if (ec != 0)
	{
		char string[AV_ERROR_MAX_STRING_SIZE];
		char* aa = av_make_error_string(string, AV_ERROR_MAX_STRING_SIZE, ec);
		std::cerr << "could not open input file, " << aa << std::endl;
		throw std::exception();
		return;
	}
	if (avformat_find_stream_info(format_, nullptr) < 0)
	{
		std::cerr << "could not find stream info" << std::endl;
		throw std::exception();
		return;
	}
}

void FfFormatContext::OpenInputFile(const std::string & path) {
	if (!path_.empty())
		return;
	path_ = path;
	opened_input_file_ = true;

	ComputeCapacity();

	OpenInputFile_internal(path);
}

void FfFormatContext::OpenOutputFile(const std::string & path) {
	if (!path_.empty())
		return;

	path_ = path;
	opened_output_file_ = true;

	if (format_ == nullptr)
	{
		int ret = avformat_alloc_output_context2(&format_, NULL, NULL, path.c_str());

		if (ret < 0)
		{
			char string[AV_ERROR_MAX_STRING_SIZE];
			char* aa = av_make_error_string(string, AV_ERROR_MAX_STRING_SIZE, ret);
			std::cerr << "cannot alloc avformatoutputcontext, " << aa << std::endl;
			throw std::exception();
			return;
		}
	}
}

void FfFormatContext::InitOutputFileFromFromInputFIle(FfFormatContext & input_format) {
	for (int i = 0; i < input_format.format_->nb_streams; i++) {
		AVStream *in_stream = input_format.format_->streams[i];

		AVCodec* codec = avcodec_find_decoder(in_stream->codecpar->codec_id);
		if (codec == nullptr)
		{
			std::cerr << ("Error avcodec_find_decoder(). Could not allocate AVCodec") << std::endl;
			throw std::exception();
		}

		format_->bit_rate = input_format.format_->bit_rate;
		format_->ctx_flags = input_format.format_->ctx_flags;
		format_->duration = input_format.format_->duration;
		format_->flags = input_format.format_->flags;
		format_->max_delay = input_format.format_->max_delay;
		format_->start_time = input_format.format_->start_time;
		format_->start_time_realtime = input_format.format_->start_time_realtime;

		AVStream *out_stream = avformat_new_stream(format_, codec/* NULL/*m_pavformatcontext->video_codec*/);
		if (!out_stream)
		{
			std::cerr << "failed allocating output stream" << std::endl;
			throw std::exception();
		}
		avcodec_parameters_copy(out_stream->codecpar, in_stream->codecpar);

		out_stream->codecpar->codec_tag = 0;

		out_stream->time_base = in_stream->time_base;
		out_stream->avg_frame_rate = in_stream->avg_frame_rate;
		out_stream->duration = in_stream->duration;
		out_stream->first_dts = in_stream->first_dts;
		out_stream->nb_frames = in_stream->nb_frames;
		out_stream->r_frame_rate = in_stream->r_frame_rate;
		out_stream->start_time = in_stream->start_time;
	}

	if (!(format_->flags & AVFMT_NOFILE)) { //TODO je tu pouzita dobra maska?
		auto ret = avio_open(&format_->pb, path_.c_str(), AVIO_FLAG_WRITE);
		if (ret < 0)
		{
			char string[AV_ERROR_MAX_STRING_SIZE];
			char* aa = av_make_error_string(string, AV_ERROR_MAX_STRING_SIZE, ret);
			std::cerr << "could not open output file " << path_<< ", "<< aa << std::endl;
			throw std::exception();
		}
	}

	auto ret = avformat_init_output(format_, NULL);
	if (ret < 0)
	{
		char string[AV_ERROR_MAX_STRING_SIZE];
		char* aa = av_make_error_string(string, AV_ERROR_MAX_STRING_SIZE, ret);
		std::cerr << "error occurred when opening output file" << std::endl;
		throw std::exception();
	}

	ret = avformat_write_header(format_, NULL);
	if (ret < 0)
	{
		std::cerr << "error occurred when opening output file" << std::endl;
		throw std::exception();
	}

	capacity_ = input_format.capacity_;
}

void FfFormatContext::IterateOverStream(const PacketHandler & hndl) {
	AVPacket packet;
	while (1)
	{
		int ret = av_read_frame(format_, &packet); //PSTODO neprerusitelne

		if (ret < 0) {
			av_packet_unref(&packet);
			break;
		}

		hndl(&packet);

		av_packet_unref(&packet);
	}
	avformat_seek_file(format_, -1, INT64_MIN, 0, 0, AVSEEK_FLAG_BYTE|AVSEEK_FLAG_ANY|AVSEEK_FLAG_BACKWARD);//PSTODO is this safe?
}

void FfFormatContext::ComputeCapacity() {
	FfFormatContext copy{};
	copy.OpenInputFile_internal(path_);

	size_t frame_count = 0;
	copy.IterateOverStream([&frame_count](AVPacket *) { frame_count++; });

	capacity_ = frame_count;
}

void FfFormatContext::CopyDataFromInputFileWithModifier(FfFormatContext & input_format, const PacketHandler & modifier) {
	PacketHandler modifierLocal = [&](AVPacket * packet) {
		modifier(packet);
		av_interleaved_write_frame(format_, packet);
	};
	input_format.IterateOverStream(modifierLocal);
}

} // stego_disk
