#include <executor.h>

using namespace CMA;

namespace CMA
{
    namespace TaskManage
    {

        TaskExecutor::TaskExecutor(TaskScheduler *scheduler, TaskConfig *config)
            : m_scheduler(scheduler), m_config(config), m_state(false)
        {
            priority = m_config->config_mgr->get<int>("priority")->getValue();
            delay = m_config->config_mgr->get<int64_t>("delay")->getValue();
            m_config->config_mgr->set("executorStartTimestamp", Utils::getCurTimestamp()); // 设置任务开始执行时间
            LOG(INFO) << "Load and initialize task configuration.";
        }

        TaskExecutor::~TaskExecutor()
        {
            pause();

            for (auto th : m_threads)
            {
                if (th != nullptr)
                {
                    delete th;
                    th = nullptr;
                }
            }
            m_threads.clear();

            if (m_config != nullptr)
            {
                delete m_config;
                m_config = nullptr;
            }
            LOG(INFO) << "TaskExecutor destructor.";

            if (m_pullStream)
            {
                delete m_pullStream;
                m_pullStream = nullptr;
            }
            if (m_pushStream)
            {
                delete m_pushStream;
                m_pushStream = nullptr;
            }
        }

        bool TaskExecutor::start(std::string &msg)
        {
            m_state = true;
            std::string id = m_config->config_mgr->get<std::string>("id")->getValue();
            // PullStream
            m_pullStream = new StreamManage::PullStream(m_scheduler->getConfig(), m_config);

            if (!m_pullStream->connect())
            {
                LOG(ERROR) << "Task " << id << " connect failed.";
                msg = "Task " + id + " connect failed.";
                return false;
            }

            std::thread *th = new std::thread(&StreamManage::PullStream::start, m_pullStream, this);
            m_threads.push_back(th);

            th = new std::thread(
                [&]()
                {
                    while (m_state)
                    {
                        AVFrame *frame = nullptr;
                        int index = 0;
                        // LOG(INFO) << "Task " << id << " get frame.";
                        m_pullStream->getFrame(frame, index);
                        if (frame != nullptr)
                        {
                            // LOG(INFO) << "Task " << id << " push frame.";
                            m_pushStream->pushFrame(frame, index);
                        }
                    }
                });

            m_threads.push_back(th);

            // PushStream
            m_pushStream = new StreamManage::PushStream(m_scheduler->getConfig(), m_config, m_pullStream);

            if (!m_pushStream->connect())
            {
                LOG(ERROR) << "Task " << id << " connect failed.";
                msg = "Task " + id + " connect failed.";
                return false;
            }
            th = new std::thread(&StreamManage::PushStream::start, m_pushStream, this);
            m_threads.push_back(th);

            for (auto th : m_threads)
            {

#ifdef _MSC_VER

                HANDLE threadHandle = th->native_handle();
#else
                pthread_t threadHandle = th->native_handle();

#endif
            }

            LOG(INFO) << "Task " << id << " is running.";
            msg = "Task " + id + " is running.";
            return true;
        }

        bool TaskExecutor::getState()
        {
            // LOG(INFO) << "TaskExecutor::getState()";
            return m_state;
        }

        bool TaskExecutor::pause()
        {
            // 改变任务运行状态
            if (m_state)
            {
                m_state = false;

                for (auto th : m_threads)
                {
                    th->join(); // 等待子任务线程执行完毕
                }

                for (auto th : m_threads)
                {
                    if (th != nullptr)
                    {
                        delete th;
                        th = nullptr;
                    }
                }
                m_threads.clear();
                LOG(INFO) << "Task execution state changed, currently " << m_state << ".";
            }

            return true;
        }

        bool TaskComparator::operator()(const TaskExecutor *lhs, const TaskExecutor *rhs) const
        {
            LOG(INFO) << "TaskComparator::operator()";
            return lhs->priority < rhs->priority;
        }
    }
}