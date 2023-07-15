#ifndef __CMA_ALGORITHM_H__
#define __CMA_ALGORITHM_H__

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

#include "log.h"
#include "config.h"
#include "utils.h"

namespace CMA
{

    namespace AlgorithmManage
    {
        class Algorithm;

        struct Area
        {
            int x;
            int y;
            int width;
            int height;
        };

        struct AlgorithmResult
        {
            unsigned char *data;
            int probability;
            string category;
            Area *area;
        };

        class Algorithm
        {
        public:
            virtual AlgorithmResult *process(const AVFrame *frame) = 0;
        };

        class ObjectDetect : public Algorithm
        {
        public:
            AlgorithmResult *process(const AVFrame *frame) override;
        };

        class EmotionRecognition : public Algorithm
        {
        public:
            AlgorithmResult *process(const AVFrame *audioFrame, const AVFrame *videoFrame);
        };

    } // namespace AlgorithmManage

} // namespace CMA

#endif // !__CMA_ALGORITHM_H__