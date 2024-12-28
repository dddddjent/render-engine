#include "recorder.h"
#include "core/config/config.h"
#include "core/tool/logger.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
char av_error[AV_ERROR_MAX_STRING_SIZE] = { 0 };
#undef av_err2str
#define av_err2str(errnum) av_make_error_string(av_error, AV_ERROR_MAX_STRING_SIZE, errnum)
}

void Recorder::init(const Configuration& cfg)
{
    av_log_set_level(AV_LOG_WARNING);

    JSON_GET(RecorderConfiguration, recorder_config, cfg, "recorder");

    bit_rate = recorder_config.bit_rate;
    frame_rate = recorder_config.frame_rate;
    is_recording = recorder_config.record_from_start;
    if (is_recording) {
        begin(recorder_config.output_path, cfg["width"], cfg["height"]);
    }
}

void Recorder::begin_ffmpeg(const std::filesystem::path& path, uint32_t width, uint32_t height)
{
    if (width % 2 == 1 || height % 2 == 1) {
        ERROR_FILE("Width and height must be even");
        throw std::runtime_error("Width and height must be even");
    }

    int ret = avformat_alloc_output_context2(&fmt_ctx, nullptr, nullptr, path.string().c_str());
    if (!fmt_ctx) {
        CRITICAL_FILE("Could not create output context, error: {}", av_err2str(ret));
        throw std::runtime_error("Could not create output context");
        return;
    }

    const AVCodec* codec = avcodec_find_encoder(fmt_ctx->oformat->video_codec);
    if (!codec) {
        CRITICAL_FILE("Could not find codec");
        throw std::runtime_error("Could not find codec");
        return;
    }
    INFO_FILE("Found codec: {}", codec->name);

    ost.codec_ctx = avcodec_alloc_context3(codec);
    {
        if (!ost.codec_ctx) {
            CRITICAL_FILE("Could not allocate codec context");
            throw std::runtime_error("Could not allocate codec context");
            return;
        }
        ost.codec_ctx->codec_id = fmt_ctx->oformat->video_codec;
        ost.codec_ctx->bit_rate = bit_rate;
        ost.codec_ctx->width = width;
        ost.codec_ctx->height = height;
        ost.codec_ctx->time_base = { 1, frame_rate };
        ost.codec_ctx->framerate = { frame_rate, 1 };
        ost.codec_ctx->gop_size = 10;
        ost.codec_ctx->max_b_frames = 1;
        ost.codec_ctx->pix_fmt = AV_PIX_FMT_YUV420P;

        if (codec->id == AV_CODEC_ID_H264) {
            av_opt_set(ost.codec_ctx->priv_data, "preset", "slow", 0);
        }
        if (ost.codec_ctx->codec_id == AV_CODEC_ID_MPEG2VIDEO) {
            ost.codec_ctx->max_b_frames = 2;
        }
        if (fmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
            ost.codec_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

        ret = avcodec_open2(ost.codec_ctx, codec, NULL);
        if (ret < 0) {
            CRITICAL_FILE("Could not open codec, error: {}", av_err2str(ret));
            throw std::runtime_error("Could not open codec");
            return;
        }
    }

    ost.stream = avformat_new_stream(fmt_ctx, codec);
    {
        if (!ost.stream) {
            CRITICAL_FILE("Could not create stream");
            throw std::runtime_error("Could not create stream");
            return;
        }
        ost.stream->id = fmt_ctx->nb_streams - 1;
        ret = avcodec_parameters_from_context(ost.stream->codecpar, ost.codec_ctx);
        if (ret < 0) {
            CRITICAL_FILE("Could not copy codec parameters, error: {}", av_err2str(ret));
            throw std::runtime_error("Could not copy codec parameters");
            return;
        }
    }

    ost.packet = av_packet_alloc();
    ost.frame = av_frame_alloc();
    {
        ost.frame->format = ost.codec_ctx->pix_fmt;
        ost.frame->width = ost.codec_ctx->width;
        ost.frame->height = ost.codec_ctx->height;
        ost.frame->pts = 0;
        ret = av_frame_get_buffer(ost.frame, 0);
        if (ret < 0) {
            CRITICAL_FILE("Could not allocate frame buffer, error: {}", av_err2str(ret));
            throw std::runtime_error("Could not allocate frame buffer");
            return;
        }
    }
    ost.rgb_frame = av_frame_alloc();
    {
        ost.rgb_frame->format = AV_PIX_FMT_BGRA;
        ost.rgb_frame->width = ost.codec_ctx->width;
        ost.rgb_frame->height = ost.codec_ctx->height;
        ost.rgb_frame->pts = 0;
        ret = av_frame_get_buffer(ost.rgb_frame, 0);
        if (ret < 0) {
            CRITICAL_FILE("Could not allocate frame buffer, error: {}", av_err2str(ret));
            throw std::runtime_error("Could not allocate frame buffer");
            return;
        }
        TRACE_FILE("linesize: {}", ost.rgb_frame->linesize[0]);
        TRACE_FILE("width * 4: {}", ost.rgb_frame->width * 4);
    }

    ost.sws_ctx = sws_getContext(
        ost.codec_ctx->width, ost.codec_ctx->height, AV_PIX_FMT_BGRA,
        ost.codec_ctx->width, ost.codec_ctx->height, AV_PIX_FMT_YUV420P,
        SWS_BILINEAR, NULL, NULL, NULL);

    ret = avio_open(&fmt_ctx->pb, path.string().c_str(), AVIO_FLAG_WRITE);
    if (ret < 0) {
        CRITICAL_FILE("Could not open output file, error: {}", av_err2str(ret));
        throw std::runtime_error("Could not open output file");
        return;
    }

    ret = avformat_write_header(fmt_ctx, nullptr);
    if (ret < 0) {
        CRITICAL_FILE("Could not write header, error: {}", av_err2str(ret));
        throw std::runtime_error("Could not write header");
        return;
    }
    TRACE_FILE("write header");
}

void Recorder::begin(const std::filesystem::path& path, uint32_t width, uint32_t height)
{
    auto folder = path.parent_path();
    if (!std::filesystem::exists(folder)) {
        std::filesystem::create_directories(folder);
    }

    begin_ffmpeg(path, width, height);

    is_recording = true;
    INFO_FILE("recording started");
}

void Recorder::encode_frame(AVFormatContext* fmt_ctx, AVCodecContext* codec_ctx, AVStream* stream, AVFrame* frame, AVPacket* packet)
{
    if (frame != nullptr)
        TRACE_FILE("send frame {}", frame->pts);
    else
        TRACE_FILE("send ending frame");

    auto ret = avcodec_send_frame(codec_ctx, frame);
    if (ret < 0) {
        CRITICAL_FILE("Could not send frame, error: {}", av_err2str(ret));
        throw std::runtime_error("Could not send frame");
        return;
    }

    while (ret >= 0) {
        ret = avcodec_receive_packet(codec_ctx, packet);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            TRACE_FILE("try to receive packet, ret: {}", av_err2str(ret));
            return;
        } else if (ret < 0) {
            CRITICAL_FILE("Could not receive packet, error: {}", av_err2str(ret));
            throw std::runtime_error("Could not receive packet");
            return;
        }
        av_packet_rescale_ts(packet, codec_ctx->time_base, stream->time_base);
        packet->stream_index = stream->index;

        TRACE_FILE("receive packet");
        auto ret1 = av_interleaved_write_frame(fmt_ctx, packet);
        if (ret1 < 0) {
            CRITICAL_FILE("Could not write packet, error: {}", av_err2str(ret1));
            throw std::runtime_error("Could not write packet");
            return;
        }
    }
}

void Recorder::fill_rgb(AVFrame* frame, uint8_t* data, size_t width, size_t height)
{
    //! linesize[0] != width * 4, because of padding
    for (int y = 0; y < height; y++) {
        memcpy(frame->data[0] + y * frame->linesize[0], // Destination row in AVFrame
            data + y * width * 4, // Source row in BGRA data
            width * 4); // Number of bytes to copy per row
    }
}

void Recorder::append(std::vector<uint8_t>& data)
{
    if (!is_recording) {
        CRITICAL_FILE("recording not started");
        throw std::runtime_error("recording not started");
        return;
    }

    auto ret = av_frame_make_writable(ost.frame);
    if (ret < 0) {
        CRITICAL_FILE("Could not make frame writable, error: {}", av_err2str(ret));
        throw std::runtime_error("Could not make frame writable");
        return;
    }

    fill_rgb(ost.rgb_frame, data.data(), ost.codec_ctx->width, ost.codec_ctx->height);
    ret = sws_scale(
        ost.sws_ctx,
        (const uint8_t* const*)ost.rgb_frame->data, ost.rgb_frame->linesize,
        0, ost.codec_ctx->height,
        ost.frame->data, ost.frame->linesize);
    if (ret < 0) {
        CRITICAL_FILE("Could not scale frame");
        throw std::runtime_error("Could not scale frame");
        return;
    }
    TRACE_FILE("sws_scale frame");
    ost.frame->pts = ost.next_pts++;

    encode_frame(fmt_ctx, ost.codec_ctx, ost.stream, ost.frame, ost.packet);
}

void Recorder::end_ffmpeg()
{
    // flush the final frames
    encode_frame(fmt_ctx, ost.codec_ctx, ost.stream, nullptr, ost.packet);

    auto ret = av_write_trailer(fmt_ctx);
    if (ret < 0) {
        CRITICAL_FILE("Could not write trailer, error: {}", av_err2str(ret));
        throw std::runtime_error("Could not write trailer");
        return;
    }
    TRACE_FILE("write trailer");

    avcodec_free_context(&ost.codec_ctx);
    av_frame_free(&ost.frame);
    av_frame_free(&ost.rgb_frame);
    av_packet_free(&ost.packet);
    sws_freeContext(ost.sws_ctx);
    TRACE_FILE("free stream");

    ret = avio_close(fmt_ctx->pb);
    if (ret < 0) {
        CRITICAL_FILE("Could not close output file, error: {}", av_err2str(ret));
        throw std::runtime_error("Could not close output file");
        return;
    }
    TRACE_FILE("close avio");

    avformat_free_context(fmt_ctx);
    TRACE_FILE("free all the contexts");
}

void Recorder::end()
{
    end_ffmpeg();

    is_recording = false;
    INFO_FILE("recording ended");
}

void Recorder::destroy()
{
}
