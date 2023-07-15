

#ifndef __CMA_STREAM_H__
#define __CMA_STREAM_H__

#include <queue>
#include <mutex>
#include <condition_variable>

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

#include "log.h"
#include "config.h"
#include "utils.h"
#include "executor.h"
#include "scheduler.h"

namespace CMA
{
    namespace ConfigManage
    {
        class TaskConfig;
        class CMAConfig;
    }

    namespace TaskManage
    {
        class TaskExecutor; // 前向声明 TaskExecutor
    }

    namespace StreamManage
    {

        typedef struct StreamContext
        {
            AVCodecContext *codec_ctx;
        } StreamContext;

        class PullStream
        {
        public:
            PullStream(ConfigManage::CMAConfig *config, ConfigManage::TaskConfig *taskConfig);
            ~PullStream();

        public:
            bool connect();     // 连接流
            bool disconnect();  // 断开流
            bool reconnect();   // 重连流
            int m_connectCount; // 连续连接次数

            // ref: https://blog.csdn.net/leixiaohua1020/article/details/14214705
            AVFormatContext *m_formatContext; // 存储格式化的媒体流环境对象地址
            StreamContext *stream_ctx;
            unsigned int nb_streams;

            void start(TaskManage::TaskExecutor *executor); // 读取流数据线程

            void getFrame(AVFrame *&frame, int &index); // 从数据包队列获取单帧数据包, 一定要主动释放!!!

        private:
            ConfigManage::CMAConfig *m_config;      // 全局配置参数管理对象地址
            ConfigManage::TaskConfig *m_taskConfig; // 任务配置参数管理对象地址

            std::queue<std::pair<AVFrame *, int>> stream_frameQueue;
            std::mutex m_frameQueueMutex; // 数据帧队列互斥锁
        };

        class PushStream
        {
        public:
            PushStream(ConfigManage::CMAConfig *config, ConfigManage::TaskConfig *taskConfig, const PullStream *pullStream);
            ~PushStream();

        public:
            bool connect();     // 连接流
            bool disconnect();  // 断开流
            bool reconnect();   // 重连流
            int m_connectCount; // 连续连接次数

            // ref: https://blog.csdn.net/leixiaohua1020/article/details/14214705
            AVFormatContext *m_formatContext; // 存储格式化的媒体流环境对象地址
            StreamContext *stream_ctx;
            unsigned int nb_streams;
            int m_videoIndex;
            int m_audioIndex;

            void start(TaskManage::TaskExecutor *executor); // 推流线程
            bool pushFrame(AVFrame *frame, int index);      // 推送数据包

        private:
            ConfigManage::CMAConfig *m_config;      // 全局配置参数管理对象地址
            ConfigManage::TaskConfig *m_taskConfig; // 任务配置参数管理对象地址
            const PullStream *m_pullStream;         // 拉流对象地址

            std::queue<std::pair<AVFrame *, int>> stream_frameQueue;
            std::mutex m_frameQueueMutex; // 数据帧队列互斥锁
            std::mutex m_pushMutex;       // 推流锁
        };

    } // namespace Stream
} // namespace CMA
#endif // __CMA_STREAM_H__
