
#ifndef __CMA_TASKSCHEDULER_H__
#define __CMA_TASKSCHEDULER_H__

#include <map>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <vector>
#include <thread>
#include <atomic>

#include "log.h"
#include "config.h"
#include "utils.h"
#include "executor.h"

using namespace CMA;
using ConfigManage::CMAConfig; // 全局配置参数管理
using ConfigManage::TaskConfig;

namespace CMA
{

    namespace TaskManage
    {

        class TaskComparator;
        class TaskExecutor;
        class TaskScheduler;

        class TaskScheduler
        {
        public:
            // friend class TaskExecutor; /* 指明其他的类（或者）函数能够直接访问该类中的 private 和 protected 成员 */

            TaskScheduler(CMAConfig *config); // 构造函数, 初始化任务池和线程
            ~TaskScheduler();                 // 析构函数, 停止线程

            void loop(); // 开始任务调度循环
            bool getState();           // 获取任务调度器状态
            void setState(bool state); // 设置任务调度器状态
            CMAConfig *getConfig();    // 获取全局配置参数管理对象地址

            bool addTask(TaskConfig *taskConfig, int &result_code, std::string &result_msg);            // 添加任务接口
            bool addTask(TaskConfig *taskConfig);                                                       // 添加任务
            bool pauseTask(std::string &taskId, int &result_code, std::string &result_msg);          // 暂停任务接口
            bool pauseTask(std::string &taskId);                                                     // 停止任务
            bool startTask(std::string &taskId, int &result_code, std::string &result_msg);
            bool startTask(std::string &taskId);                                                     // 开始任务
            bool removeTask(std::string &taskId, int &result_code, std::string &result_msg);         // 删除任务接口
            bool removeTask(std::string &taskId);                                                    // 删除任务
            bool getTaskState(std::string &taskId, int &result_code, std::string &result_msg);       // 获取任务状态接口
            TaskConfig * getTaskConfig(std::string &taskId, int &result_code, std::string &result_msg);  // 获取任务接口
            // TaskConfig * getTaskInfo(TaskConfig *taskConfig, int &result_code, std::string &result_msg); // 获取任务信息接口
            std::vector<TaskConfig *> getTaskList(int &result_code, std::string &result_msg); // 获取任务列表接口

        private:
            CMAConfig *m_config; // 全局配置参数管理对象地址

            std::map<std::string, TaskExecutor *> m_executorMap; // 任务存储队列(任务池)
            std::mutex m_executorMap_mutex;                      // 任务存储队列(任务池)

            // std::priority_queue<TaskExecutor *, std::vector<TaskExecutor *>, TaskComparator> m_executorPqueue; // 任务优先级队列, 存储待执行任务对象
            // std::condition_variable m_condition;

            std::atomic<bool> m_state; // 是否运行中

            std::queue<TaskExecutor *> m_deletedExecutorQueue;        //  删除任务队列
            std::mutex m_deletedExecutorQueue_mutex;                  // 删除任务队列锁
            std::condition_variable m_deletedExecutorQueue_condition; // 删除任务队列条件变量
            void deleteTaskThread();                                  // 删除任务函数
            std::thread *m_deleteTaskThread;                          // 删除任务线程对象
        };

    }

}

#endif // !__CMA_TASKSCHEDULER_H__