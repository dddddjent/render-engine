#pragma once

#include "core/config/config.h"
#include <cstdint>
#include <filesystem>
#include <vector>

struct AVFormatContext;
struct AVCodecContext;
struct AVStream;
struct AVFrame;
struct AVPacket;
struct SwsContext;

struct OutputStream {
    AVStream* stream;
    AVCodecContext* codec_ctx;

    AVFrame* rgb_frame;
    AVFrame* frame;
    AVPacket* packet;

    SwsContext* sws_ctx;

    int64_t next_pts = 0;
};

class Recorder {
public:
    void init(const Configuration& config);
    void begin(const std::filesystem::path& path, uint32_t width, uint32_t height);
    void append(std::vector<uint8_t>& data);
    void end();
    void destroy();

    void begin_ffmpeg(const std::filesystem::path& path, uint32_t width, uint32_t height);
    void end_ffmpeg();

    bool is_recording = false;

private:
    static void encode_frame(AVFormatContext* fmt_ctx, AVCodecContext* codec_ctx, AVStream* stream, AVFrame* frame, AVPacket* packet);
    static void fill_rgb(AVFrame* frame, uint8_t* data, size_t width, size_t height);

    AVFormatContext* fmt_ctx = nullptr;
    OutputStream ost;

    int64_t bit_rate = 0;
    int frame_rate = 0;
};
