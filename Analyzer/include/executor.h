
#ifndef __CMA_TASKEXECUTOR_H__
#define __CMA_TASKEXECUTOR_H__

#include <thread>
#include <queue>
#include <mutex>

#include "log.h"
#include "config.h"
#include "utils.h"
#include "scheduler.h"
#include "stream.h"
#include "algorithm.h"
namespace CMA
{


    namespace AlgorithmManage
    {
        struct AlgorithmResult;
    }

    namespace ConfigManage
    {
        class TaskConfig;
        class CMAConfig;
    }

    namespace StreamManage{

        class PullStream;
        class PushStream;
    }
    

    namespace TaskManage
    {
        class TaskComparator;
        class TaskExecutor;
        class TaskScheduler;

        class TaskExecutor
        {
        public:
            explicit TaskExecutor(TaskScheduler *scheduler, ConfigManage::TaskConfig *config);
            ~TaskExecutor();

        public:
            ConfigManage::TaskConfig *m_config;       /* 当前任务的参数配置对象地址 */
            TaskScheduler *m_scheduler; /* 当前任务的任务池调度对象 */
            int priority;               // 任务优先级
            int64_t delay;              // 任务延迟执行时间

        public:
            // m_pullStream
            // m_pushStream
            // m_algrithmAnalyzer
            // m_alarmGenerator
            StreamManage::PullStream *m_pullStream; /* 当前任务的拉流对象地址 */
            StreamManage::PushStream *m_pushStream; /* 当前任务的推流对象地址 */


        private:
            std::atomic<bool> m_state = false;    /* 控制当前任务的执行状态 */
            std::vector<std::thread *> m_threads; /* 构造一个线程池, 存储子任务线程 */

        
        public:
            bool start(std::string &msg); /* 开始执行当前任务 */
            bool pause();                 /* 暂停执行任务 */
            bool getState();              /* 获取任务执行状态 */
        };

        // 任务比较函数, 用于任务优先级比较
        class TaskComparator
        {
        public:
            bool operator()(const TaskExecutor *lhs, const TaskExecutor *rhs) const;
        };

    }

}

#endif // !__CMA_TASKEXECUTOR_H__