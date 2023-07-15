
#include <fstream>
#include <iostream>
#include <memory>

#include "json.hpp"
#include "config.h"
#include "log.h"

using json = nlohmann::json;
using namespace el;
using namespace CMA::ConfigManage;

namespace CMA
{

    namespace ConfigManage
    {
        // 析构函数
        ConfigManager::~ConfigManager()
        {
            std::unique_lock lock(m_mutex);
            m_cfgs.clear();
        }

        /**
         * @brief 清空配置管理器
         *
         */
        void ConfigManager::clear()
        {
            std::unique_lock lock(m_mutex);
            m_cfgs.clear();
        }

        // /**
        //  * @brief: 遍历配置模块里面所有配置项
        //  * @param: cb 配置项回调函数
        //  */

        // void visit(std::function<void(ConfigBase::ptr)> cb)
        // {
        //     std::shared_lock lock(m_mutex);
        //     for (auto &it : m_cfgs)
        //         cb(it.second);
        // }

        CMAConfig *CMAConfig::m_instance = nullptr;
        std::mutex CMAConfig::m_mutex;
        CMAConfig::CGabor CMAConfig::m_gabor;

        CMAConfig::CMAConfig(const std::string &file)
        {
            json root = parse_config(file);
            check_config(root);
            config_mgr->set("cfg_path", file, "Parameter file address");
            config_mgr->set("server_host", root["server_host"].get<std::string>(), "Server address");
            config_mgr->set("server_port", root["server_port"].get<int>(), "Server port");
            config_mgr->set("cache_dir", root["cache_dir"].get<std::string>(), "Cache folder address");
            config_mgr->set("control_executor_maxNum", root["control_executor_maxNum"].get<int>(), "Controls the maximum number of task deployments");
            config_mgr->set("support_hardware_videoDecode", root["support_hardware_videoDecode"].get<bool>(), "Whether to support video hardware decoding");
            config_mgr->set("support_hardware_videoEncode", root["support_hardware_videoEncode"].get<bool>(), "Whether to support video hardware encoding");
            config_mgr->set("algorithm_device", root["algorithm_device"].get<std::string>(), "Algorithm running device environment, CPU, GPU, FPGA.");
            config_mgr->set("algorithm_instanceNum", root["algorithm_instanceNum"].get<int>(), "The maximum number of algorithms that can be called, according to its own equipment conditions and model calculation consumption settings");
            m_state = true;
        }

        CMAConfig::~CMAConfig()
        {
            config_mgr->clear();
            delete config_mgr;
            config_mgr = nullptr;
        }

        const json CMAConfig::parse_config(const std::string &file)
        {
            /* 以二进制方式打开本地参数文件 */
            std::ifstream ifs(file, std::ios::binary);
            if (!ifs.is_open())
            {
                LOG(ERROR) << "open " << file << " error." << std::endl;
                std::exit(1);
                return nullptr;
            }
            // 解析 json 数据
            json root;

            try
            {
                ifs >> root;
            }
            catch (...)
            {
                std::cerr << "error parsing file" << std::endl;
                return nullptr;
            }
            ifs.close();
            return root;
        }

        void CMAConfig::check_config(const json &j)
        {
            // 循环遍历参数名称数组，检查是否都有相应的值
            for (const std::string &name : param_names)
            {
                if (j.find(name) == j.end())
                {
                    LOG(ERROR) << "config.json missing parameter: " << name << std::endl;
                    std::exit(1);
                }
            }
        }

        void CMAConfig::show_help()
        {
            std::cout << *config_mgr << std::endl;
        }

        TaskConfig::TaskConfig(std::string id, std::string phsa, std::string plsa, bool phse = false, std::string ac = "", int64_t ami = 30, int priority = 0, int64_t delay = 0)
        {

            config_mgr->set("id", id, "Task id");
            config_mgr->set("priority", priority, "priority");
            config_mgr->set("delay", delay, "delay");

            config_mgr->set("pullStream_address", plsa, "pullStream_address");
            config_mgr->set("pushStream_enable", phse, "pushStream_enable");
            config_mgr->set("pushStream_address", phsa, "pushStream_address");

            config_mgr->set("algorithmName", ac, "algorithmName");
            config_mgr->set("process_minInterval", ami, "process_minInterval");

            //  以下参数运行过程中自行计算
            config_mgr->set("processFps", 0, "processFps");
            config_mgr->set("executorStartTimestamp", int64_t(0), "executorStartTimestamp");
            config_mgr->set("videoWidth", 0, "videoWidth");
            config_mgr->set("videoHeight", 0, "videoHeight");
            config_mgr->set("videoChannel", 0, "videoChannel");
            config_mgr->set("videoIndex", 0, "videoIndex");
            config_mgr->set("videoFps", -1, "videoFps");
            config_mgr->set("audioIndex", 0, "audioIndex");
            config_mgr->set("audioSampleRate", 0, "audioSampleRate");
            config_mgr->set("audioChannel", 0, "audioChannel");
            config_mgr->set("audioFps", -1, "audioFps");
        }

        TaskConfig::TaskConfig(json &root)
        {

            config_mgr->set("id", root["id"].get<std::string>(), "Task id");
            config_mgr->set("priority", root["priority"].get<int>(), "priority");
            config_mgr->set("delay", root["delay"].get<int64_t>(), "delay");

            config_mgr->set("pullStream_address", root["pullStream_address"].get<std::string>(), "pullStream_address");
            config_mgr->set("pushStream_enable", root["pushStream_enable"].get<bool>(), "pushStream_enable");
            config_mgr->set("pushStream_address", root["pushStream_address"].get<std::string>(), "pushStream_address");

            config_mgr->set("algorithmName", root["algorithmName"].get<std::string>(), "algorithmName");
            config_mgr->set("process_minInterval", root["process_minInterval"].get<int64_t>(), "process_minInterval");

            //  以下参数运行过程中自行计算
            config_mgr->set("processFps", 0, "processFps");
            config_mgr->set("executorStartTimestamp", int64_t(0), "executorStartTimestamp");
            config_mgr->set("videoWidth", 0, "videoWidth");
            config_mgr->set("videoHeight", 0, "videoHeight");
            config_mgr->set("videoChannel", 0, "videoChannel");
            config_mgr->set("videoIndex", 0, "videoIndex");
            config_mgr->set("videoFps", -1, "videoFps");
            config_mgr->set("audioIndex", 0, "audioIndex");
            config_mgr->set("audioSampleRate", 0, "audioSampleRate");
            config_mgr->set("audioChannel", 0, "audioChannel");
            config_mgr->set("audioFps", -1, "audioFps");
            check_config();
            m_state = true;
        }

        bool TaskConfig::check_config()
        {
            // 循环遍历参数名称数组，检查是否都有相应的值
            for (const std::string &name : param_names)
            {
                std::unordered_map<std::string, CMA::ConfigManage::ConfigBase::ptr> const m_cfgs = config_mgr->get();
                if (m_cfgs.find(name) == m_cfgs.end())
                {
                    LOG(ERROR) << "TaskConfig missing parameter: " << name << std::endl;
                    m_state = false;
                    config_mgr->clear();
                    return false;
                }
            }
            return true;
        }

        bool TaskConfig::checkAdd(std::string &result_msg)
        {
            if (!m_state)
            {
                result_msg = "TaskConfig parameter error";
                return false;
            }
            m_state = true;
            result_msg = "success";
            if (config_mgr->get<std::string>("id")->getValue().empty() || config_mgr->get<std::string>("pullStream_address")->getValue().empty())
            {
                LOG(ERROR) << "TaskConfig parameter error" << std::endl;
                result_msg = "TaskConfig parameter error";
                m_state = false;
                config_mgr->clear();
                return false;
            }

            if (config_mgr->get<bool>("pushStream_enable")->getValue() && config_mgr->get<std::string>("pushStream_address")->getValue().empty())
            {
                LOG(ERROR) << "TaskConfig parameter error, missing pushStreamUrl." << std::endl;
                result_msg = "TaskConfig parameter error, missing pushStreamUrl.";
                m_state = false;
                config_mgr->clear();
                return false;
            }
            return true;
        }

        bool TaskConfig::checkCancel(std::string &result_msg)
        {
            m_state = true;
            result_msg = "success";
            if (config_mgr->get<std::string>("id")->getValue().empty())
            {
                LOG(ERROR) << "TaskConfig parameter error" << std::endl;
                result_msg = "TaskConfig parameter error";
                m_state = false;
                config_mgr->clear();
                return false;
            }
            return true;
        }

        TaskConfig::~TaskConfig()
        {
            if (config_mgr != nullptr)
            {
                config_mgr->clear();
                delete config_mgr;
                config_mgr = nullptr;
            }
        };
    }

}
