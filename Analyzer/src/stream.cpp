extern "C"
{
#include <libavutil/imgutils.h>
#include "libswscale/swscale.h"
#include <libswresample/swresample.h>
}

#include "log.h"
#include "config.h"
#include "utils.h"
#include "stream.h"

using namespace CMA;
using ConfigManage::CMAConfig; // 全局配置参数管理
using ConfigManage::TaskConfig;

namespace CMA
{

    namespace StreamManage
    {

        PullStream::PullStream(CMAConfig *config, TaskConfig *taskConfig)
            : m_config(config), m_taskConfig(taskConfig), m_connectCount(0), m_formatContext(nullptr), stream_ctx(nullptr)
        {
            LOG(INFO) << "PullStream constructor.";
        }

        PullStream::~PullStream()
        {
            LOG(INFO) << "PullStream destructor.";
            disconnect();
        }

        bool PullStream::connect()
        {

            disconnect(); // 先断开已有连接

            int ret;
            unsigned int i;

            m_formatContext = avformat_alloc_context(); // 分配一个 AVFormatContext 对象

            if (m_formatContext == nullptr)
            {
                LOG(ERROR) << "avformat_alloc_context failed.";
                return false;
            }

            if ((ret = avformat_open_input(&m_formatContext, m_taskConfig->config_mgr->get<std::string>("pullStream_address")->getValue().data(), nullptr, nullptr)) < 0)
            {
                LOG(ERROR) << "avformat_open_input failed, ret: " << ret;
                return false;
            }

            if ((ret = avformat_find_stream_info(m_formatContext, nullptr)) < 0)
            {
                LOG(ERROR) << "avformat_find_stream_info failed, ret: " << ret;
                return false;
            }

            stream_ctx = (StreamContext *)av_calloc(m_formatContext->nb_streams, sizeof(*stream_ctx));
            if (!stream_ctx)
            {
                LOG(ERROR) << "av_calloc failed.";
                return false;
            }

            for (i = 0; i < m_formatContext->nb_streams; i++)
            {
                AVStream *stream = m_formatContext->streams[i];
                if (stream->codecpar->codec_type != AVMEDIA_TYPE_VIDEO && stream->codecpar->codec_type != AVMEDIA_TYPE_AUDIO)
                {
                    LOG(ERROR) << "stream->codecpar->codec_type != AVMEDIA_TYPE_VIDEO && stream->codecpar->codec_type != AVMEDIA_TYPE_AUDIO, continue.";
                    stream_ctx[i].codec_ctx = nullptr;
                    continue;
                }

                const AVCodec *dec = avcodec_find_decoder(stream->codecpar->codec_id);
                AVCodecContext *codec_ctx;

                if (!dec)
                {
                    LOG(ERROR) << "avcodec_find_decoder failed. streamId: " << i;
                    return false;
                }

                codec_ctx = avcodec_alloc_context3(dec);
                if (!codec_ctx)
                {
                    LOG(ERROR) << "avcodec_alloc_context3 failed.";
                    return false;
                }

                ret = avcodec_parameters_to_context(codec_ctx, stream->codecpar);
                if (ret < 0)
                {
                    LOG(ERROR) << "avcodec_parameters_to_context failed, ret: " << ret;
                    return false;
                }

                if (codec_ctx->codec_type == AVMEDIA_TYPE_VIDEO || codec_ctx->codec_type == AVMEDIA_TYPE_AUDIO)
                {
                    if (codec_ctx->codec_type == AVMEDIA_TYPE_VIDEO)
                        codec_ctx->framerate = av_guess_frame_rate(m_formatContext, stream, NULL);

                    ret = avcodec_open2(codec_ctx, dec, NULL);
                    if (ret < 0)
                    {
                        LOG(ERROR) << "Failed to open decoder for stream " << i << ", ret: " << ret;
                        return false;
                    }
                }

                stream_ctx[i].codec_ctx = codec_ctx;
            }

            av_dump_format(m_formatContext, 0, m_taskConfig->config_mgr->get<std::string>("pullStream_address")->getValue().data(), 0);

            // 5. 获取音视频流信息
            int video_stream_index = av_find_best_stream(m_formatContext, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
            int audio_stream_index = av_find_best_stream(m_formatContext, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
            if (video_stream_index < 0 && audio_stream_index < 0)
            {
                LOG(ERROR) << "av_find_best_stream failed, video_stream_index: " << video_stream_index << ", audioStreamIndex: " << audio_stream_index;
                return false;
            }

            m_taskConfig->config_mgr->get<int>("videoIndex")->setValue(video_stream_index);
            m_taskConfig->config_mgr->get<int>("audioIndex")->setValue(audio_stream_index);

            if (video_stream_index > -1) // 有视频流
            {
                if (m_formatContext->streams[video_stream_index]->avg_frame_rate.den > 0)
                {
                    int fps = m_formatContext->streams[video_stream_index]->avg_frame_rate.num / m_formatContext->streams[video_stream_index]->avg_frame_rate.den;
                    m_taskConfig->config_mgr->get<int>("videoFps")->setValue(fps);
                }
                else
                {
                    m_taskConfig->config_mgr->get<int>("videoFps")->setValue(25);
                    LOG(WARNING) << "m_videoStream->avg_frame_rate.den: " << m_formatContext->streams[video_stream_index]->avg_frame_rate.den << " not support.";
                }
                m_taskConfig->config_mgr->get<int>("videoWidth")->setValue(m_formatContext->streams[video_stream_index]->codecpar->width);
                m_taskConfig->config_mgr->get<int>("videoHeight")->setValue(m_formatContext->streams[video_stream_index]->codecpar->height);
                m_taskConfig->config_mgr->get<int>("videoChannel")->setValue(m_formatContext->streams[video_stream_index]->codecpar->ch_layout.nb_channels);
            }

            if (audio_stream_index > -1)
            {
                // 7. 获取音频流信息
                m_taskConfig->config_mgr->get<int>("audioChannel")->setValue(m_formatContext->streams[audio_stream_index]->codecpar->ch_layout.nb_channels);
                m_taskConfig->config_mgr->get<int>("audioSampleRate")->setValue(m_formatContext->streams[audio_stream_index]->codecpar->sample_rate);
            }

            // 连接次数+1
            m_connectCount++;
            return true;
        }

        bool PullStream::disconnect()
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            if (m_formatContext != nullptr && stream_ctx != nullptr)
            {
                unsigned int i;
                for (i = 0; i < m_formatContext->nb_streams; i++)
                {
                    if (stream_ctx[i].codec_ctx != nullptr)
                    {
                        avcodec_free_context(&stream_ctx[i].codec_ctx);
                    }
                }
                av_free(stream_ctx);
            }

            while (!stream_frameQueue.empty())
            {
                AVFrame *frame = stream_frameQueue.front().first;
                int index = stream_frameQueue.front().second;
                av_frame_free(&frame);
                stream_frameQueue.pop();
            }

            if (m_formatContext != nullptr)
            {
                avformat_close_input(&m_formatContext);
                avformat_free_context(m_formatContext);
                m_formatContext = nullptr;
            } // if

            return true;
        } // PullStream::disconnect()

        bool PullStream::reconnect()
        {
            disconnect();
            return connect();
        }

        void PullStream::getFrame(AVFrame *&frame, int &index)
        {
            m_frameQueueMutex.lock();
            if (!stream_frameQueue.empty())
            {
                frame = stream_frameQueue.front().first;
                index = stream_frameQueue.front().second;
                stream_frameQueue.pop();
            }
            m_frameQueueMutex.unlock();
        }

        void PullStream::start(TaskManage::TaskExecutor *executor)
        {
            int continuityErrorCount = 0;
            int ret;

            int width = -1, height = -1;
            int frame_size = -1, sample_rate = -1;
            AVChannelLayout ch_layout;

            AVPacket *packet = nullptr;
            AVFrame *frame_yuv420p = nullptr;
            AVFrame *frame_bgr = nullptr;
            AVFrame *frame_pcm = nullptr;
            AVFrame *frame_s16 = nullptr;
            SwsContext *sws_ctx_yuv420p2bgr = nullptr;
            struct SwrContext *swr_ctx_pcm2s16 = nullptr;

            int video_stream_index = m_taskConfig->config_mgr->get<int>("videoIndex")->getValue();
            int audio_stream_index = m_taskConfig->config_mgr->get<int>("audioIndex")->getValue();

            packet = av_packet_alloc();

            if (packet == nullptr)
            {
                LOG(ERROR) << "av_packet_alloc failed.";
                goto end;
            } // if

            if (video_stream_index > -1)
            {
                frame_yuv420p = av_frame_alloc();

                if (frame_yuv420p == nullptr)
                {
                    LOG(ERROR) << "av_frame_alloc failed.";
                    goto end;
                }
                width = m_formatContext->streams[video_stream_index]->codecpar->width;
                height = m_formatContext->streams[video_stream_index]->codecpar->height;

                frame_yuv420p->format = m_formatContext->streams[video_stream_index]->codecpar->format;
                frame_yuv420p->width = width;
                frame_yuv420p->height = height;

                av_frame_get_buffer(frame_yuv420p, 1); // 为视频帧数据 AVFrame 分配内存

                // 转换图像像素格式, 将解码后的视频帧数据 AVFrame 转换为 BGR 格式
                sws_ctx_yuv420p2bgr = sws_getContext(
                    width, height, AVPixelFormat(frame_yuv420p->format),
                    width, height, AV_PIX_FMT_BGR24,
                    SWS_BICUBIC, NULL, NULL, NULL);
            }

            if (audio_stream_index > -1)
            {
                frame_pcm = av_frame_alloc();
                if (frame_pcm == nullptr)
                {
                    LOG(ERROR) << "av_frame_alloc failed.";
                    goto end;
                }

                swr_ctx_pcm2s16 = swr_alloc();
                if (swr_ctx_pcm2s16 == nullptr)
                {
                    LOG(ERROR) << "swr_alloc failed.";
                    goto end;
                }

                frame_size = m_formatContext->streams[audio_stream_index]->codecpar->frame_size;
                ch_layout = m_formatContext->streams[audio_stream_index]->codecpar->ch_layout;
                sample_rate = m_formatContext->streams[audio_stream_index]->codecpar->sample_rate;

                frame_pcm->format = m_formatContext->streams[audio_stream_index]->codecpar->format;
                frame_pcm->nb_samples = frame_size;
                frame_pcm->ch_layout = ch_layout;
                frame_pcm->sample_rate = sample_rate;

                av_frame_get_buffer(frame_pcm, 1);

                ret = swr_alloc_set_opts2(&swr_ctx_pcm2s16,
                                          &ch_layout, AV_SAMPLE_FMT_S16, sample_rate,
                                          &ch_layout, AVSampleFormat(frame_pcm->format), sample_rate,
                                          0, NULL);

                if (ret < 0)
                {
                    LOG(ERROR) << "swr_alloc_set_opts2 failed, ret: " << ret;
                    goto end;
                }

                if ((swr_init(swr_ctx_pcm2s16)) < 0)
                {
                    goto end;
                }
            }

            while (executor->getState())
            {

                if ((ret = av_read_frame(m_formatContext, packet)) < 0)
                {
                    av_packet_unref(packet);
                    continuityErrorCount++;
                    if (continuityErrorCount > 5)
                    {
                        LOG(ERROR) << "av_read_frame failed, ret: " << ret << ", continuityErrorCount: " << continuityErrorCount << " > 5, try reconnect.";
                        if (executor->m_pullStream->reconnect())
                        {
                            continuityErrorCount = 0;
                            LOG(INFO) << "reconnect success.";
                        }
                        else
                        {
                            LOG(ERROR) << "reconnect failed.";
                            break;
                        }
                        std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    }
                    continue;
                }

                if (packet->stream_index == video_stream_index)
                {

                    StreamContext *stream = &stream_ctx[video_stream_index];

                    av_packet_rescale_ts(
                        packet,
                        m_formatContext->streams[video_stream_index]->time_base,
                        stream->codec_ctx->time_base);

                    int ret = avcodec_send_packet(stream->codec_ctx, packet); // 将数据包发送到解码器

                    if (ret < 0)
                    {
                        LOG(ERROR) << "avcodec_send_packet failed, ret: " << ret;
                        goto end;
                    }

                    while (ret >= 0)
                    {
                        ret = avcodec_receive_frame(stream->codec_ctx, frame_yuv420p); // 从解码器接收解码后的视频帧数据, 自带 av_frame_unref

                        if (ret == AVERROR_EOF || ret == AVERROR(EAGAIN))
                            break;

                        if (ret < 0)
                        {
                            LOG(ERROR) << "avcodec_receive_frame failed, ret: " << ret;
                            goto end;
                        }

                        frame_bgr = av_frame_alloc();
                        if (frame_bgr == nullptr)
                        {
                            LOG(ERROR) << "av_frame_alloc failed.";
                            goto end;
                        }

                        frame_bgr->format = AV_PIX_FMT_BGR24;
                        frame_bgr->width = width;
                        frame_bgr->height = height;
                        av_frame_get_buffer(frame_bgr, 1); // 为视频帧数据 AVFrame 分配内存

                        // 将解码后的视频帧数据 AVFrame 转换为 BGR 格式
                        sws_scale(sws_ctx_yuv420p2bgr, (const uint8_t *const *)frame_yuv420p->data, frame_yuv420p->linesize, 0, height, frame_bgr->data, frame_bgr->linesize);

                        // 将解码后的视频帧数据 AVFrame 放入视频帧数据队列

                        m_frameQueueMutex.lock();
                        stream_frameQueue.push(std::pair<AVFrame *, int>(frame_bgr, video_stream_index));
                        m_frameQueueMutex.unlock();
                    }

                } // 有视频流, 解码视频帧数据
                else if (packet->stream_index == audio_stream_index)
                {
                    StreamContext *stream = &stream_ctx[audio_stream_index];

                    int ret = avcodec_send_packet(stream->codec_ctx, packet);

                    if (ret < 0)
                    {
                        LOG(ERROR) << "avcodec_send_packet failed, ret: " << ret;
                        goto end;
                    }

                    while (ret >= 0)
                    {
                        ret = avcodec_receive_frame(stream->codec_ctx, frame_pcm);

                        if (ret == AVERROR_EOF || ret == AVERROR(EAGAIN))
                            break;

                        if (ret < 0)
                        {
                            LOG(ERROR) << "avcodec_receive_frame failed, ret: " << ret;
                            goto end;
                        }

                        frame_s16 = av_frame_alloc();

                        if (frame_pcm == nullptr || frame_s16 == nullptr)
                        {
                            LOG(ERROR) << "av_frame_alloc failed.";
                            goto end;
                        }

                        frame_s16->format = AV_SAMPLE_FMT_S16;
                        frame_s16->nb_samples = frame_size;
                        frame_s16->ch_layout = ch_layout;
                        frame_s16->sample_rate = sample_rate;

                        av_frame_get_buffer(frame_s16, 1);

                        swr_convert(swr_ctx_pcm2s16, frame_s16->data, frame_s16->nb_samples, (const uint8_t **)frame_pcm->data, frame_pcm->nb_samples);

                        m_frameQueueMutex.lock();
                        stream_frameQueue.push(std::pair<AVFrame *, int>(frame_s16, audio_stream_index));
                        m_frameQueueMutex.unlock();
                    }
                } // 有音频流, 解码音频帧数据

                av_packet_unref(packet);
            }

        end:
            if (packet != nullptr)
                av_packet_free(&packet);

            if (frame_yuv420p != nullptr)
                av_frame_free(&frame_yuv420p);
            if (frame_bgr != nullptr)
                av_frame_free(&frame_bgr);
            if (frame_pcm != nullptr)
                av_frame_free(&frame_pcm);
            if (frame_s16 != nullptr)
                av_frame_free(&frame_s16);

            if (sws_ctx_yuv420p2bgr != nullptr)
                sws_freeContext(sws_ctx_yuv420p2bgr);
            if (swr_ctx_pcm2s16 != nullptr)
                swr_free(&swr_ctx_pcm2s16);

            disconnect();
            executor->pause();

        } // PullStream::start()

        PushStream::PushStream(CMAConfig *config, TaskConfig *taskConfig, const PullStream *pullStream)
            : m_config(config), m_taskConfig(taskConfig), m_pullStream(pullStream), m_connectCount(0), m_formatContext(nullptr), stream_ctx(nullptr)
        {
            LOG(INFO) << "PushStream constructor.";
        }

        PushStream::~PushStream()
        {
            LOG(INFO) << "PushStream destructor.";
            disconnect();
        }

        bool PushStream::connect()
        {
            disconnect();

            int ret;
            unsigned int i;

            if (avformat_alloc_output_context2(&m_formatContext, nullptr, "flv", m_taskConfig->config_mgr->get<std::string>("pushStream_address")->getValue().data()) < 0)
            {
                LOG(ERROR) << "avformat_alloc_output_context2 failed.";
                return false;
            }

            stream_ctx = (StreamContext *)av_calloc(m_pullStream->m_formatContext->nb_streams, sizeof(*stream_ctx));

            for (i = 0; i < m_pullStream->m_formatContext->nb_streams; i++)
            {

                AVStream *in_stream;
                AVStream *out_stream;
                AVCodecContext *dec_ctx = m_pullStream->stream_ctx[i].codec_ctx;
                AVCodecContext *enc_ctx;
                const AVCodec *encoder;

                out_stream = avformat_new_stream(m_formatContext, nullptr);
                if (!out_stream)
                {
                    LOG(ERROR) << "avformat_new_stream failed.";
                    return false;
                }

                in_stream = m_pullStream->m_formatContext->streams[i];

                if (in_stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO || in_stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
                {

                    encoder = avcodec_find_encoder(dec_ctx->codec_id);
                    if (!encoder)
                    {
                        LOG(ERROR) << "avcodec_find_encoder failed.";
                        return false;
                    }

                    enc_ctx = avcodec_alloc_context3(encoder);
                    if (!enc_ctx)
                    {
                        LOG(ERROR) << "avcodec_alloc_context3 failed.";
                        return false;
                    }

                    if (dec_ctx->codec_type == AVMEDIA_TYPE_VIDEO)
                    {

                        int bit_rate = 300 * 1024 * 8; // 压缩后每秒视频的bit位大小 300kB
                        enc_ctx->flags |= AV_CODEC_FLAG_QSCALE;

                        enc_ctx->rc_min_rate = bit_rate / 2;
                        enc_ctx->rc_max_rate = bit_rate / 2 + bit_rate;
                        enc_ctx->bit_rate = bit_rate;

                        enc_ctx->height = dec_ctx->height;
                        enc_ctx->width = dec_ctx->width;
                        enc_ctx->pix_fmt = dec_ctx->pix_fmt;
                        enc_ctx->time_base = av_inv_q(dec_ctx->framerate);
                        enc_ctx->sample_aspect_ratio = dec_ctx->sample_aspect_ratio;
                        enc_ctx->framerate = dec_ctx->framerate;
                    }
                    else
                    {

                        ret = av_channel_layout_copy(&enc_ctx->ch_layout, &dec_ctx->ch_layout);
                        if (ret < 0)
                            return ret;
                        enc_ctx->bit_rate = 128000;
                        enc_ctx->sample_fmt = encoder->sample_fmts[0];
                        enc_ctx->sample_rate = dec_ctx->sample_rate;

                        enc_ctx->frame_size = 1024;
                        enc_ctx->time_base = {1024, enc_ctx->sample_rate};
                        enc_ctx->framerate = {enc_ctx->sample_rate, 1024};
                    }

                    if (m_formatContext->oformat->flags & AVFMT_GLOBALHEADER)
                        enc_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

                    ret = avcodec_open2(enc_ctx, encoder, nullptr);
                    if (ret < 0)
                    {
                        LOG(ERROR) << "avcodec_open2 failed, ret: " << ret;
                        return false;
                    }

                    ret = avcodec_parameters_from_context(out_stream->codecpar, enc_ctx);
                    if (ret < 0)
                    {
                        LOG(ERROR) << "avcodec_parameters_from_context failed, ret: " << ret;
                        return false;
                    }

                    out_stream->time_base = enc_ctx->time_base;
                }
                else if (in_stream->codecpar->codec_type == AVMEDIA_TYPE_UNKNOWN)
                {
                    LOG(ERROR) << "dec_ctx->codec_type == AVMEDIA_TYPE_UNKNOWN";
                    return false;
                }
                else
                {
                    ret = avcodec_parameters_copy(out_stream->codecpar, in_stream->codecpar);
                    if (ret < 0)
                    {
                        LOG(ERROR) << "avcodec_parameters_copy failed, ret: " << ret;
                        return false;
                    }
                    enc_ctx = nullptr;
                    out_stream->time_base = in_stream->time_base;
                }
                stream_ctx[i].codec_ctx = enc_ctx;
            }

            av_dump_format(m_formatContext, 0, m_taskConfig->config_mgr->get<std::string>("pushStream_address")->getValue().data(), 1);

            if (!(m_formatContext->oformat->flags & AVFMT_NOFILE))
            {
                ret = avio_open(&m_formatContext->pb, m_taskConfig->config_mgr->get<std::string>("pushStream_address")->getValue().data(), AVIO_FLAG_WRITE);
                if (ret < 0)
                {
                    LOG(ERROR) << "avio_open failed, ret: " << ret;
                    return false;
                }
            }

            ret = avformat_write_header(m_formatContext, nullptr);
            if (ret < 0)
            {
                LOG(ERROR) << "avformat_write_header failed, ret: " << ret;
                return false;
            }

            m_connectCount++;
            return true;
        }

        bool PushStream::disconnect()
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));

            if (m_formatContext != nullptr && stream_ctx != nullptr)
            {
                unsigned int i;
                for (i = 0; i < m_formatContext->nb_streams; i++)
                {
                    avcodec_free_context(&stream_ctx[i].codec_ctx);
                }
                av_free(stream_ctx);
            }

            while (!stream_frameQueue.empty())
            {
                AVFrame *frame = stream_frameQueue.front().first;
                int index = stream_frameQueue.front().second;
                av_frame_free(&frame);
                stream_frameQueue.pop();
            }

            if (m_formatContext != nullptr)
            {
                av_write_trailer(m_formatContext);
                if (!(m_formatContext->oformat->flags & AVFMT_NOFILE))
                {
                    avio_close(m_formatContext->pb);
                }
                avformat_free_context(m_formatContext);
                m_formatContext = nullptr;
            } // if

            return true;

        } // PushStream::disconnect()

        bool PushStream::reconnect()
        {
            disconnect();
            return connect();
        }

        bool PushStream::pushFrame(AVFrame *frame, int index)
        {
            if (frame == nullptr)
            {
                return false;
            }

            m_frameQueueMutex.lock();
            stream_frameQueue.push(std::pair<AVFrame *, int>(frame, index));
            m_frameQueueMutex.unlock();

            return true;
        }

        void PushStream::start(TaskManage::TaskExecutor *executor)
        {

            int continuityErrorCount = 0;

            AVPacket *packet = nullptr;
            AVFrame *frame_in = nullptr;
            AVFrame *frame_yuv420p = nullptr;
            AVFrame *frame_pcm = nullptr;

            SwsContext *sws_ctx_bgr2yuv420p = nullptr;
            struct SwrContext *swr_ctx_s16_2pcm = nullptr;

            int video_stream_index = m_taskConfig->config_mgr->get<int>("videoIndex")->getValue();
            int audio_stream_index = m_taskConfig->config_mgr->get<int>("audioIndex")->getValue();

            int64_t encodeVideoSuccessCount = 0; /* 对成功编码的视频帧进行计数 */
            int64_t videoFrameCount = 0;         /* 对视频帧进行计数 */
            int64_t encodeAudioSuccessCount = 0; /* 对成功编码的音频帧进行计数 */
            int64_t audioFrameCount = 0;         /* 对音频帧进行计数 */
            int ret = -1;

            packet = av_packet_alloc();

            if (packet == nullptr)
            {
                LOG(ERROR) << "av_packet_alloc failed.";
                return;
            }

            if (video_stream_index > -1)
            {
                frame_yuv420p = av_frame_alloc();
                if (frame_yuv420p == nullptr)
                {
                    LOG(ERROR) << "av_frame_alloc failed.";
                    goto end;
                }

                frame_yuv420p->format = m_pullStream->m_formatContext->streams[video_stream_index]->codecpar->format;
                frame_yuv420p->width = m_pullStream->m_formatContext->streams[video_stream_index]->codecpar->width;
                frame_yuv420p->height = m_pullStream->m_formatContext->streams[video_stream_index]->codecpar->height;
                av_frame_get_buffer(frame_yuv420p, 1);

                sws_ctx_bgr2yuv420p = sws_getContext(
                    frame_yuv420p->width, frame_yuv420p->height, AV_PIX_FMT_BGR24,
                    frame_yuv420p->width, frame_yuv420p->height, AVPixelFormat(frame_yuv420p->format),
                    SWS_BICUBIC, NULL, NULL, NULL);

                if (sws_ctx_bgr2yuv420p == nullptr)
                {
                    LOG(ERROR) << "sws_getContext failed.";
                    goto end;
                }
            }

            if (audio_stream_index > -1)
            {
                frame_pcm = av_frame_alloc();
                if (frame_pcm == nullptr)
                {
                    LOG(ERROR) << "av_frame_alloc failed.";
                    goto end;
                }

                frame_pcm->format = m_formatContext->streams[audio_stream_index]->codecpar->format;
                frame_pcm->nb_samples = m_pullStream->m_formatContext->streams[audio_stream_index]->codecpar->frame_size;
                frame_pcm->ch_layout = m_pullStream->m_formatContext->streams[audio_stream_index]->codecpar->ch_layout;
                frame_pcm->sample_rate = m_pullStream->m_formatContext->streams[audio_stream_index]->codecpar->sample_rate;

                av_frame_get_buffer(frame_pcm, 1);

                ret = swr_alloc_set_opts2(
                    &swr_ctx_s16_2pcm,
                    &frame_pcm->ch_layout, AVSampleFormat(frame_pcm->format), frame_pcm->sample_rate,
                    &frame_pcm->ch_layout, AV_SAMPLE_FMT_S16, frame_pcm->sample_rate,
                    0, NULL);

                if (swr_ctx_s16_2pcm == nullptr)
                {
                    LOG(ERROR) << "swr_alloc_set_opts failed.";
                    goto end;
                }

                ret = swr_init(swr_ctx_s16_2pcm);
                if (ret < 0)
                {
                    LOG(ERROR) << "swr_init failed.";
                    goto end;
                }
            }

            while (executor->getState())
            {

                if (!stream_frameQueue.empty())
                {
                    m_frameQueueMutex.lock();
                    frame_in = stream_frameQueue.front().first;
                    int stream_index = stream_frameQueue.front().second;
                    stream_frameQueue.pop();
                    m_frameQueueMutex.unlock();

                    StreamContext *stream = &stream_ctx[stream_index];

                    if (stream_index == video_stream_index)
                    {

                        sws_scale(sws_ctx_bgr2yuv420p, (const uint8_t *const *)frame_in->data, frame_in->linesize, 0, frame_yuv420p->height, frame_yuv420p->data, frame_yuv420p->linesize);

                        frame_yuv420p->pts = frame_yuv420p->pkt_dts = av_rescale_q_rnd(
                            videoFrameCount,
                            stream->codec_ctx->time_base,
                            m_formatContext->streams[video_stream_index]->time_base,
                            (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));

                        frame_yuv420p->pkt_duration = av_rescale_q_rnd(1,
                                                                       stream->codec_ctx->time_base,
                                                                       m_formatContext->streams[video_stream_index]->time_base,
                                                                       (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
                        frame_yuv420p->pkt_pos = -1;

                        ret = avcodec_send_frame(stream->codec_ctx, frame_yuv420p);

                        if (ret < 0)
                        {
                            LOG(ERROR) << "avcodec_send_frame failed, ret: " << ret;
                            goto end;
                        }

                        while (ret >= 0)
                        {
                            ret = avcodec_receive_packet(stream->codec_ctx, packet);
                            if (ret == AVERROR_EOF || ret == AVERROR(EAGAIN))
                                break;

                            if (ret < 0)
                            {
                                LOG(ERROR) << "avcodec_receive_packet failed, ret: " << ret;
                                goto end;
                            }
                            packet->stream_index = video_stream_index;

                            ret = av_interleaved_write_frame(m_formatContext, packet);
                            if (ret < 0)
                            {
                                LOG(ERROR) << "av_interleaved_write_frame failed, ret: " << ret;
                                goto end;
                            }
                        }
                        // av_frame_unref(frame_yuv420p);
                        videoFrameCount++;
                        encodeVideoSuccessCount++;
                    }
                    else if (stream_index == audio_stream_index)
                    {

                        swr_convert(swr_ctx_s16_2pcm, frame_pcm->data, frame_pcm->nb_samples, (const uint8_t **)frame_in->data, frame_in->nb_samples);

                        frame_pcm->pts = frame_pcm->pkt_dts = av_rescale_q_rnd(
                            audioFrameCount,
                            stream->codec_ctx->time_base,
                            m_formatContext->streams[audio_stream_index]->time_base,
                            (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));

                        frame_pcm->pkt_duration = av_rescale_q_rnd(1,
                                                                   stream->codec_ctx->time_base,
                                                                   m_formatContext->streams[audio_stream_index]->time_base,
                                                                   (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));

                        frame_pcm->pkt_pos = -1;

                        ret = avcodec_send_frame(stream->codec_ctx, frame_pcm);

                        if (ret < 0)
                        {
                            LOG(ERROR) << "avcodec_send_frame failed, ret: " << ret;
                            goto end;
                        }

                        while (ret >= 0)
                        {
                            ret = avcodec_receive_packet(stream->codec_ctx, packet);
                            if (ret == AVERROR_EOF || ret == AVERROR(EAGAIN))
                                break;

                            if (ret < 0)
                            {
                                LOG(ERROR) << "avcodec_receive_packet failed, ret: " << ret;
                                goto end;
                            }
                            packet->stream_index = audio_stream_index;

                            ret = av_interleaved_write_frame(m_formatContext, packet);
                            if (ret < 0)
                            {
                                LOG(ERROR) << "av_interleaved_write_frame failed, ret: " << ret;
                                goto end;
                            }
                        }
                        // av_frame_unref(frame_pcm);
                        audioFrameCount++;
                        encodeAudioSuccessCount++;
                    }

                    av_packet_unref(packet);
                    av_frame_free(&frame_in);
                    frame_in = nullptr;
                }
            }

        end:
            if (packet != nullptr)
                av_packet_free(&packet);
            if (frame_in != nullptr)
                av_frame_free(&frame_in);
            if (frame_yuv420p != nullptr)
                av_frame_free(&frame_yuv420p);
            if (frame_pcm != nullptr)
                av_frame_free(&frame_pcm);
            if (sws_ctx_bgr2yuv420p != nullptr)
                sws_freeContext(sws_ctx_bgr2yuv420p);
            if (swr_ctx_s16_2pcm != nullptr)
                swr_free(&swr_ctx_s16_2pcm);

            packet = nullptr;
            frame_in = nullptr;
            frame_yuv420p = nullptr;
            frame_pcm = nullptr;
            sws_ctx_bgr2yuv420p = nullptr;
            swr_ctx_s16_2pcm = nullptr;

            disconnect();
            executor->pause();
        }

    } // namespace Stream
} // namespace CMA
