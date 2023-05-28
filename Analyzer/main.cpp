#define WIN32_LEAN_AND_MEAN
#define ELPP_WINSOCK2

#include <chrono>
#include <random>
#include <thread>

#include "argparse.hpp" // 命令行参数解析
#include "config.h" // 配置参数管理
#include "log.h" // 日志
#include "executor.h"
#include "scheduler.h"
#include "server.h"

std::default_random_engine generator;
using namespace el;
using namespace CMA;
INITIALIZE_EASYLOGGINGPP

int main(int argc, char **argv)
{   
    START_EASYLOGGINGPP(argc, argv);

    auto seed = std::chrono::system_clock::now().time_since_epoch().count();
    generator.seed(static_cast<unsigned int>(seed));

    // 命令行参数解析
    argparse::ArgumentParser parser("StreamMediaAnalysisServer");
    parser.add_description("Stream Media Analysis Server by SCW");
    parser.add_argument("-c","--config").default_value<std::string>(std::string("./config.json")) //.required()
        .help("Config File Path.");
    parser.add_argument("--port").default_value<short>(-1).help("Server Port. The highest priority, higher than the configuration file.").scan<'i', short>();
    ;
    parser.add_argument("--host").default_value<std::string>(std::string("")).help("Server Host. The highest priority, higher than the configuration file.");
    parser.add_epilog("Author: SCW(1911523105@qq.com), 2023, all rights reserved.");

    try
    {
        parser.parse_args(argc, argv);
    }
    catch (const std::runtime_error &err)
    {
        LOG(ERROR) << err.what() << std::endl;
        std::exit(1);
    }
    
    // 程序帮助信息处理
    if (parser["--help"] == true)
    {
        LOG(INFO) << parser << std::endl;
        std::cout << parser << std::endl;
        std::exit(1);
    }


    // 程序配置参数文件路径, 必需参数, 默认值为 ./config.json
    std::string m_file = parser.get<std::string>("--config");

    if (m_file.empty())
    {
        LOG(ERROR) << "Config File Path is empty." << std::endl;
        std::exit(1);
    }

    // 获取和初始化程序全局配置参数管理实例
    auto *cfg = ConfigManage::CMAConfig::instance(m_file);

    if (!cfg->m_state)
    {
        LOG(ERROR) << "Config File Parse Failed." << std::endl;
        std::exit(1);
    }

    // 根据命令行参数, 更正一些配置信息, 
    if (parser.get<std::string>("host") != "")
    {
        cfg->config_mgr->set("server_host", parser.get<std::string>("host"));
    }

    if (parser.get<short>("port")>=1024 && parser.get<int>("port") <= 65535)
    {
        cfg->config_mgr->set("server_port", parser.get<int>("port"));
    }

    LOG(INFO) << "Config File Parse Success." << std::endl;
    cfg->show_help();

    // 构建和运行分析布控实例
	TaskManage::TaskScheduler *taskScheduler = new TaskManage::TaskScheduler(cfg);
	
    /* 运行分析服务 */
	Server::Server server;
    server.start(taskScheduler);
    // 等待程序终止
    taskScheduler->loop();

    // 释放资源
    delete taskScheduler;
    taskScheduler = nullptr;
    return 0;
}