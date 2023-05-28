#include "log.h"
#include "config.h"
#include <executor.h>
#include "utils.h"

using namespace CMA;

namespace CMA
{
    namespace TaskManage
    {

        TaskScheduler::TaskScheduler(CMAConfig *config)
            : m_config(config), m_state(false)
        {
            LOG(INFO) << "TaskScheduler constructor.";
        }

        TaskScheduler::~TaskScheduler()
        {
            m_state = false;
            LOG(INFO) << "TaskScheduler destructor.";

            if (m_deleteTaskThread != nullptr)
            {
                LOG(INFO) << "TaskScheduler deleteTaskThread join.";
                m_deletedExecutorQueue_condition.notify_all();
                m_deleteTaskThread->join();
                delete m_deleteTaskThread;
                m_deleteTaskThread = nullptr;
                LOG(INFO) << "TaskScheduler deleteTaskThread destructed.";
            }

            std::unique_lock<std::mutex> lock(m_executorMap_mutex);
            for (auto it = m_executorMap.begin(); it != m_executorMap.end(); it++)
            {
                if (it->second != nullptr)
                {
                    delete it->second;
                    it->second = nullptr;
                }
                LOG(INFO) << "TaskScheduler executorMap destructed.";
            }
            m_executorMap.clear();
            m_config = nullptr;
        }

        /* 开始任务调度循环 */
        void TaskScheduler::loop()
        {
            LOG(INFO) << "TaskScheduler loop.";
            LOG(INFO) << "Press 'q' or 'esc' to quit.";
            m_deleteTaskThread = new std::thread(&TaskScheduler::deleteTaskThread, this);

#ifdef _MSC_VER

            HANDLE threadHandle = m_deleteTaskThread->native_handle(); // 获取线程句柄
#else
            pthread_t threadHandle = m_deleteTaskThread->native_handle(); // 获取线程句柄

#endif

            // std::cout << "Press 'q' or 'esc' to quit." << std::endl;

            // m_state = true;
            while (m_state)
            {
                if (Utils::is_escape_key_pressed())
                {
                    m_state = false;
                    break;
                }

                // if(Utils::is_key_pressed('a') || Utils::is_key_pressed('A'))
                // {
                //     std::string taskId;
                //     std::cout << "Add task. Please input task id: " << std::endl;
                //     std::cin >> taskId;
                //     json root;
                //     // "id", "priority", "delay", "pullStream_address", "pushStream_enable", "pushStream_address", "algorithmCode", "process_minInterval"
                //     root["id"] = taskId;
                //     root["priority"] = 1;
                //     root["delay"] = 0;
                //     root["pullStream_address"] = "rtsp://0.0.0.0";
                //     root["pushStream_enable"] = false;
                //     root["pushStream_address"] = "rtmp://0.0.0.0";
                //     root["algorithmCode"] = "test";
                //     root["process_minInterval"] = 0;
                //     TaskConfig *taskConfig = new TaskConfig(root);
                //     int result_code = 0;
                //     std::string result_msg;
                //     addTask(taskConfig, result_code, result_msg);
                // }

                // if(Utils::is_key_pressed('p') || Utils::is_key_pressed('P'))
                // {
                //     std::string taskId;
                //     std::cout << "Pause task. Please input task id: " << std::endl;
                //     std::cin >> taskId;

                //     int result_code = 0;
                //     std::string result_msg;
                //     ConfigManage::TaskConfig* taskCfg = getTaskConfig(taskId, result_code, result_msg);
                //     pauseTask(taskId, result_code, result_msg);

                // }
                // if(Utils::is_key_pressed('d') || Utils::is_key_pressed('D'))
                // {
                //     std::string taskId;
                //     std::cout << "Delete task. Please input task id: " << std::endl;
                //     std::cin >> taskId;
                //     int result_code = 0;
                //     std::string result_msg;
                //     ConfigManage::TaskConfig* taskCfg = getTaskConfig(taskId, result_code, result_msg);
                //     removeTask(taskId, result_code, result_msg);
                // }

                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            }

            LOG(INFO) << "TaskScheduler loop end.";
        }

        /* 添加任务接口 */

        bool TaskScheduler::addTask(TaskConfig *taskConfig, int &result_code, std::string &result_msg)
        {
            LOG(INFO) << "TaskScheduler api_addTask.";

            std::unique_lock<std::mutex> lock(m_executorMap_mutex);

            if (taskConfig == nullptr)
            {

                result_code = 1;
                result_msg = "taskConfig is nullptr.";
                LOG(ERROR) << result_msg;
                return false;
            }

            bool isAdd = m_executorMap.end() != m_executorMap.find(taskConfig->get_id());
            if (isAdd)
            {
                result_code = 2;
                result_msg = "task_id is exist.";
                LOG(WARNING) << result_msg;
                return true;
            }

            if (m_executorMap.size() >= m_config->config_mgr->get<int>("control_executor_maxNum")->getValue())
            {

                result_code = 3;
                result_msg = std::string("The number of tasks exceeds the maximum limit (") + std::to_string(m_config->config_mgr->get<int>("control_executor_maxNum")->getValue()) + ")";
                LOG(ERROR) << result_msg;
                return false;
            }

            // 构建任务执行对象
            TaskExecutor *taskExecutor = new TaskExecutor(this, taskConfig);

            try
            {
                m_executorMap.insert(std::pair<std::string, TaskExecutor *>(taskConfig->get_id(), taskExecutor));
                result_msg = "Task added successfully. ";
                LOG(INFO) << result_msg;
            }
            catch (const std::exception &e)
            {
                delete taskExecutor;
                taskExecutor = nullptr;
                result_code = 4;
                result_msg = std::string("Task addition failed.") + e.what();
                LOG(ERROR) << result_msg;
                return false;
            }

            if (taskExecutor->start(result_msg) && taskExecutor->getState())
            {
                result_code = 0;
                result_msg = "Task started successfully." + result_msg;
                LOG(INFO) << result_msg;
                return true;
            }
            else
            {
                result_code = 5;
                result_msg = std::string("Task launch failed.") + result_msg;
                LOG(ERROR) << result_msg;

                delete taskExecutor;
                taskExecutor = nullptr;
                m_executorMap.erase(taskConfig->get_id());
                return false;
            }

            result_code = -1;
            result_msg = "Unknown error.";
            LOG(ERROR) << result_msg;
            return false;
        }

        bool TaskScheduler::addTask(TaskConfig *taskConfig)
        {
            int result_code = 0;
            std::string result_msg = "";
            return addTask(taskConfig, result_code, result_msg);
        }

        /* 停止任务接口 */
        bool TaskScheduler::pauseTask(std::string &taskId, int &result_code, std::string &result_msg)
        {
            LOG(INFO) << "TaskScheduler api_stopTask.";

            std::unique_lock<std::mutex> lock(m_executorMap_mutex);
            bool isAdd = m_executorMap.end() != m_executorMap.find(taskId);

            if (!isAdd)
            {
                result_code = 2;
                result_msg = "task_id is not exist.";
                LOG(ERROR) << result_msg;
                return false;
            }

            TaskExecutor *taskExecutor = m_executorMap[taskId];

            if (taskExecutor == nullptr)
            {
                result_code = 3;
                result_msg = "taskExecutor is nullptr.";
                LOG(ERROR) << result_msg;
                return false;
            }

            if (taskExecutor->pause() && !taskExecutor->getState())
            {
                result_code = 0;
                result_msg = "Task paused successfully." + result_msg;
                LOG(INFO) << result_msg;
                return true;
            }
            else
            {
                result_code = 4;
                result_msg = std::string("Task pause failed.") + result_msg;
                LOG(ERROR) << result_msg;
                return false;
            }

            result_code = -1;
            result_msg = "Unknown error.";
            LOG(ERROR) << result_msg;
            return false;
        }

        bool TaskScheduler::pauseTask(std::string &taskId)
        {
            int result_code = 0;
            std::string result_msg = "";
            return pauseTask(taskId, result_code, result_msg);
        }

        /* 恢复任务接口 */
        bool TaskScheduler::startTask(std::string &taskId, int &result_code, std::string &result_msg)
        {
            LOG(INFO) << "TaskScheduler api_startTask.";

            std::unique_lock<std::mutex> lock(m_executorMap_mutex);
            bool isAdd = m_executorMap.end() != m_executorMap.find(taskId);

            if (!isAdd)
            {
                result_code = 2;
                result_msg = "task_id is not exist.";
                LOG(ERROR) << result_msg;
                return false;
            }

            TaskExecutor *taskExecutor = m_executorMap[taskId];

            if (taskExecutor == nullptr)
            {
                result_code = 3;
                result_msg = "taskExecutor is nullptr.";
                LOG(ERROR) << result_msg;
                return false;
            }
            if (taskExecutor->start(result_msg) && taskExecutor->getState())
            {
                result_code = 0;
                result_msg = "Task started successfully." + result_msg;
                LOG(INFO) << result_msg;
                return true;
            }
            else
            {
                result_code = 4;
                result_msg = std::string("Task launch failed.") + result_msg;
                LOG(ERROR) << result_msg;
                return false;
            }

            result_code = -1;
            result_msg = "Unknown error.";
            LOG(ERROR) << result_msg;
            return false;
        }

        bool TaskScheduler::startTask(std::string &taskId)
        {
            int result_code = 0;
            std::string result_msg = "";
            return startTask(taskId, result_code, result_msg);
        }

        /* 删除任务接口 */
        bool TaskScheduler::removeTask(std::string &taskId, int &result_code, std::string &result_msg)
        {
            LOG(INFO) << "TaskScheduler api_removeTask.";

            std::unique_lock<std::mutex> lock(m_executorMap_mutex);
            bool isAdd = m_executorMap.end() != m_executorMap.find(taskId);

            if (!isAdd)
            {
                result_code = 2;
                result_msg = "task_id is not exist.";
                LOG(ERROR) << result_msg;
                return false;
            }

            TaskExecutor *taskExecutor = m_executorMap[taskId];

            if (taskExecutor == nullptr)
            {
                result_code = 3;
                result_msg = "taskExecutor is nullptr.";
                LOG(ERROR) << result_msg;
                return false;
            }

            try
            {
                delete taskExecutor;
                taskExecutor = nullptr;
                m_executorMap.erase(taskId); // 删除元素
                result_code = 0;
                result_msg = "Task stopped successfully." + result_msg;
                LOG(INFO) << result_msg;
                return true;
            }
            catch (const std::exception &e)
            {
                result_code = 4;
                result_msg = std::string("Task stop failed. cerr: ") + e.what() + result_msg;
                LOG(ERROR) << result_msg;
                return false;
            }

            result_code = -1;
            result_msg = "Unknown error.";
            LOG(ERROR) << result_msg;
            return false;
        }

        bool TaskScheduler::removeTask(std::string &taskId)
        {
            int result_code = 0;
            std::string result_msg = "";
            return removeTask(taskId, result_code, result_msg);
        }

        bool TaskScheduler::getTaskState(std::string &taskId, int &result_code, std::string &result_msg)
        {
            LOG(INFO) << "TaskScheduler api_getTaskState.";

            std::unique_lock<std::mutex> lock(m_executorMap_mutex);
            bool isAdd = m_executorMap.end() != m_executorMap.find(taskId);

            if (!isAdd)
            {
                result_code = 2;
                result_msg = "task_id is not exist.";
                LOG(ERROR) << result_msg;
                return false;
            }

            TaskExecutor *taskExecutor = m_executorMap[taskId];

            if (taskExecutor == nullptr)
            {
                result_code = 3;
                result_msg = "taskExecutor is nullptr.";
                LOG(ERROR) << result_msg;
                return false;
            }
            result_code = 0;
            return taskExecutor->getState();
        }

        /* 通过 id 获取指定任务配置*/
        TaskConfig *TaskScheduler::getTaskConfig(std::string &taskId, int &result_code, std::string &result_msg)
        {
            LOG(INFO) << "TaskScheduler api_getTask.";

            std::unique_lock<std::mutex> lock(m_executorMap_mutex);

            bool isAdd = m_executorMap.end() != m_executorMap.find(taskId);
            if (!isAdd)
            {
                result_code = 2;
                result_msg = "task_id is not exist.";
                LOG(ERROR) << result_msg;
                return nullptr;
            }

            TaskExecutor *taskExecutor = m_executorMap[taskId];
            if (taskExecutor == nullptr)
            {
                result_code = 3;
                result_msg = "taskExecutor is nullptr.";
                LOG(ERROR) << result_msg;
                return nullptr;
            }
            result_code = 0;
            return taskExecutor->m_config;
        }

        /* 获取任务列表接口 */
        std::vector<TaskConfig *> TaskScheduler::getTaskList(int &result_code, std::string &result_msg)
        {
            LOG(INFO) << "TaskScheduler api_getTaskList.";

            std::unique_lock<std::mutex> lock(m_executorMap_mutex);

            std::vector<TaskConfig *> taskList;
            for (auto iter = m_executorMap.begin(); iter != m_executorMap.end(); iter++)
            {
                taskList.push_back(iter->second->m_config);
            }
            result_code = 0;
            result_msg = "Successfully obtained task list.";
            return taskList;
        }

        /* 获取任务运行状态 */
        bool TaskScheduler::getState()
        {
            return m_state;
        }

        void TaskScheduler::setState(bool state)
        {
            m_state = state;
        }

        CMAConfig *TaskScheduler::getConfig()
        {
            return m_config;
        }

        void TaskScheduler::deleteTaskThread()
        {
            LOG(INFO) << "TaskScheduler deleteTaskThread start.";
            std::unique_lock<std::mutex> lock(m_deletedExecutorQueue_mutex);
            m_deletedExecutorQueue_condition.wait(lock);

            while (!m_deletedExecutorQueue.empty())
            {
                TaskExecutor *taskExecutor = m_deletedExecutorQueue.front();
                m_deletedExecutorQueue.pop();

                LOG(INFO) << "TaskScheduler deleteTaskThread delete taskExecutor. id(" << taskExecutor->m_config->get_id() << "))";
                if (taskExecutor != nullptr)
                {
                    delete taskExecutor;
                    taskExecutor = nullptr;
                }
            }
            LOG(INFO) << "TaskScheduler deleteTaskThread end.";
        }

    }

}