#ifndef __CMA_ANALYZER_H__
#define __CMA_ANALYZER_H__

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

#include "log.h"
#include "config.h"
#include "utils.h"
#include "algorithm.h"

namespace CMA
{

    namespace AlgorithmManage
    {
        class ObjectDetect;
        class Algorithm;
        struct AlgorithmResult;

    }
    namespace ConfigManage
    {
        class TaskConfig;
        class CMAConfig;
    }

    namespace TaskManage
    {
        class TaskExecutor; // 前向声明 TaskExecutor
    }
    namespace AnalyzerManage
    {
        class Analyzer;

        class Analyzer
        {
        public:
            Analyzer(ConfigManage::CMAConfig *config, ConfigManage::TaskConfig *taskConfig);
            ~Analyzer();
            

            AlgorithmManage::Algorithm *m_algorithm;
            std::string m_algorithmName;

            AlgorithmManage::AlgorithmResult *processFrame(const AVFrame *frame);
        };

    }

}

#endif // !__CMA_ANALYZER_H__