extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

#include "log.h"
#include "config.h"
#include "utils.h"
#include "algorithm.h"
#include "analyzer.h"

namespace CMA
{

    namespace AnalyzerManage
    {
        Analyzer::Analyzer(ConfigManage::CMAConfig *config, ConfigManage::TaskConfig *taskConfig)
        {
            std::string algorithmName = taskConfig->config_mgr->get<std::string>("algorithmName")->getValue();
            if (algorithmName == "ObjectDetect")
                m_algorithm = new AlgorithmManage::ObjectDetect();
            else{
                m_algorithm = nullptr;
            }
        }

        Analyzer::~Analyzer()
        {
            delete m_algorithm;
        }

        AlgorithmManage::AlgorithmResult* Analyzer::processFrame(const AVFrame *frame)
        {
            if(m_algorithm == nullptr)
                return nullptr;
            return m_algorithm->process(frame);
        }

    }

}
