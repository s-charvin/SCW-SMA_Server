#ifdef _WIN32
#include <winsock2.h>
#else
#include <sys/socket.h>
#endif

#include <event2/event.h>
#include <event2/http.h>
#include <event2/buffer.h>
#include <event2/http_struct.h>
#include <thread>

#include "scheduler.h"
#include "server.h"
#include "log.h" // 日志
#include "json.hpp"

using namespace CMA;
using json = nlohmann::json;
using namespace el;

#define RECV_BUF_MAX_SIZE 1024 * 8

namespace CMA
{

    namespace Server
    {

        Server::Server()
        {
            
            WORD wdVersion = MAKEWORD(2, 2); /* 定义自己需要的网络库版本, 这里是 2.2 */
            WSADATA wdSockMsg;
            int s = WSAStartup(wdVersion, &wdSockMsg); /* 打开一个套接字(Socket) */

            // 出错处理
            // ref: http://blog.chinaunix.net/uid-21768364-id-3244516.html
            if (0 != s)
            {
                switch (s)
                {

                case WSASYSNOTREADY: /* 网络通信依赖的网络子系统还没有准备好. */
                {
                    LOG(ERROR) << "Please restart the computer, or check the network library.";
                    break;
                }
                case WSAVERNOTSUPPORTED: /* 所需的 Windows Sockets API的版本未由特定的Windows Sockets实现提供 */
                {
                    LOG(ERROR) << "Please update the network library.";
                    break;
                }
                case WSAEINPROGRESS: /* 当前任务（或线程）有未完成的阻塞操作, */
                {
                    LOG(ERROR) << "Please restart the program.";
                    break;
                }
                case WSAEPROCLIM: /* 达到了 Windows Sockets 使用限制的应用程序数量 */
                {
                    LOG(ERROR) << "Please close unnecessary software to ensure sufficient network resources.";
                    break;
                }
                }
            }

            if (2 != HIBYTE(wdSockMsg.wVersion) || 2 != LOBYTE(wdSockMsg.wVersion))
            {
                LOG(ERROR) << "Network library version error.";
                return;
            }
        }

        Server::~Server()
        {
            LOG(INFO) << "Server is closed.";
            WSACleanup(); /* 关闭此套接字 */
        }

        void Server::start(TaskManage::TaskScheduler *scheduler)
        {
            scheduler->setState(true);
            /* 开启一个线程, thread(Fn&& fn, Args&&… args) 以 scheduler 为参数, 执行一个匿名函数.[外部变量访问方式说明符] (参数)  {函数体;};
             */
            std::thread([](TaskManage::TaskScheduler *scheduler)
                        {
                    // 创建 http 事件
                    // ref: https://zhuanlan.zhihu.com/p/76336361
                    // ref: https://blog.csdn.net/qq_28114615/article/details/92847048
                    // ref: https://blog.csdn.net/chuanglan/article/details/55667076
                    // ref: https://www.cnblogs.com/osbreak/p/10146571.html

                    /* 分配一个 event_base 的内部配置结构体，其中指定了需要避免使用的IO模型（entries），所使用的IO复用模型需要满足的特征（event_method_feature）, 创建的 event_base 需要满足的要求（event_base_config_flag）
                     */
                    event_config *evt_config = event_config_new();
                    /* 根据配置 (可自定义) 创建一个 event_base，使用 Reactor 模式，处理并发 I/O IO 事件(event)，如添加事件，激活事件，IO操作等
                     */
                    struct event_base *base = event_base_new_with_config(evt_config);

                    /* 基于 event_base 创建一个新的 http event */
                    struct evhttp *http = evhttp_new(base);
                    /* 设置当前 http 事件协议传输内容标头 */
                    evhttp_set_default_content_type(http, "text/html; charset=utf-8");
                    /* 为 http 请求设置超时时间，以秒为单位 */
                    evhttp_set_timeout(http, 30);

                    /* 设置 http 请求/响应路由，即针对每一个事件(请求)注册一个处理函数(回调函数)(服务器, 请求路径, 回调函数, 回调参数); */
                    evhttp_set_cb(http, "/api", api_help, nullptr);
                    evhttp_set_cb(http, "/api/task/add", api_addTask, scheduler);
                    evhttp_set_cb(http, "/api/task/pause", api_pauseTask, scheduler);
                    evhttp_set_cb(http, "/api/task/start", api_startTask, scheduler);
                    evhttp_set_cb(http, "/api/task/remove", api_removeTask, scheduler);
                    evhttp_set_cb(http, "/api/task/get_state", api_getTaskState, scheduler);

                    /* 在指定的地址和端口上绑定 HTTP 服务 */
                    evhttp_bind_socket(http, scheduler->getConfig()->config_mgr->get<std::string>("server_host")->getValue().c_str(),
                                       scheduler->getConfig()->config_mgr->get<int>("server_port")->getValue());

                    // ref: https://blog.csdn.net/weixin_42169029/article/details/85249840

                    /* 展开事件循环运行, 检测信号并执行回调函数, 超时处理, 事件监听 */
                    event_base_dispatch(base);
                    /* 释放与 event_base 对象关联的所有内存 */
                    event_base_free(base);
                    /* 释放 HTTP 服务 */
                    evhttp_free(http);
                    /* 释放与 event_config 对象关联的所有内存 */
                    event_config_free(evt_config);

                    scheduler->setState(false); /* 当服务线程及其子线程运行结束后, 重置任务调度系统状态 */ },

                        scheduler)
                .detach(); /* 将当前线程对象所代表的执行实例与该线程对象分离 一旦线程执行完毕，它所分配的资源将会被释放。 */
        }

        void api_help(struct evhttp_request *req, void *arg)
        {
            json result_urls;
            result_urls["/api"] = " API version: 1.0 ";
            result_urls["/api/task/add"] = " add task ";
            result_urls["/api/task/pause"] = " pause task ";
            result_urls["/api/task/start"] = " start task";
            result_urls["/api/task/remove"] = " remove task ";
            result_urls["/api/task/get_state"] = " get task state ";

            json result;
            result["urls"] = result_urls;
            result["msg"] = "success";
            result["code"] = 0;
            api_response(req, result);
        }

        /* 添加任务 */
        void api_addTask(struct evhttp_request *req, void *arg)
        {
            TaskManage::TaskScheduler *scheduler = (TaskManage::TaskScheduler *)arg;
            int result_code = -1;
            std::string result_msg = "";

            char buf[RECV_BUF_MAX_SIZE];
            /* 解析 HTTP 请求中传输的 JSON 数据到 buf 中 */
            parse_post(req, buf);

            // 解析 json 数据
            json root = json::parse(buf);
            ConfigManage::TaskConfig *taskConfig = new ConfigManage::TaskConfig(root);

            if (!taskConfig->checkAdd(result_msg))
            {
                    result_code = 1;
                    result_msg = "task config error. " + result_msg;
            }

            if (result_code!=1 && scheduler->addTask(taskConfig, result_code, result_msg))
            {
                result_code = 0;
                result_msg = "success"+ result_msg;
            }
            else{

                result_code = 2;
                result_msg = " add task failed. " + result_msg;
            }

            json result;
            result["msg"] = result_msg;
            result["code"] = result_code;

            LOG(INFO) << "\n \t Request:" << root.dump() << "\n \t Response:" << result.dump();

            api_response(req, result);
        }

        /* 暂停指定任务 */
        void api_pauseTask(struct evhttp_request *req, void *arg)
        {
            TaskManage::TaskScheduler *scheduler = (TaskManage::TaskScheduler *)arg;
            int result_code = -1;
            std::string result_msg = "";

            char buf[RECV_BUF_MAX_SIZE];
            /* 解析 HTTP 请求中传输的 JSON 数据到 buf 中 */
            parse_post(req, buf);

            // 解析 json 数据
            json root = json::parse(buf);
            std::string taskId = root["id"].get<std::string>();

            if (scheduler->pauseTask(taskId, result_code, result_msg))
            {
                result_code = 0;
                result_msg = "success";
            }
                        else{

                result_code = 2;
                result_msg = " pause task failed";
            }

            json result;
            result["msg"] = result_msg;
            result["code"] = result_code;

            LOG(INFO) << "\n \t Request:" << root.dump() << "\n \t Response:" << result.dump();

            api_response(req, result);
        }

        /* 重新开始指定任务 */
        void api_startTask(struct evhttp_request *req, void *arg)
        {
            TaskManage::TaskScheduler *scheduler = (TaskManage::TaskScheduler *)arg;
            int result_code = -1;
            std::string result_msg = "";

            char buf[RECV_BUF_MAX_SIZE];
            /* 解析 HTTP 请求中传输的 JSON 数据到 buf 中 */
            parse_post(req, buf);

            // 解析 json 数据
            json root = json::parse(buf);
            std::string taskId = root["id"].get<std::string>();

            if (scheduler->startTask(taskId, result_code, result_msg))
            {
                result_code = 0;
                result_msg = "success";
            }
            else{

                result_code = 2;
                result_msg = " start task failed";
            }
            json result;
            result["msg"] = result_msg;
            result["code"] = result_code;

            LOG(INFO) << "\n \t Request:" << root.dump() << "\n \t Response:" << result.dump();

            api_response(req, result);
        }

        /* 取消指定任务 */
        void api_removeTask(struct evhttp_request *req, void *arg)
        {
            TaskManage::TaskScheduler *scheduler = (TaskManage::TaskScheduler *)arg;
            int result_code = -1;
            std::string result_msg = "";

            char buf[RECV_BUF_MAX_SIZE];
            /* 解析 HTTP 请求中传输的 JSON 数据到 buf 中 */
            parse_post(req, buf);

            // 解析 json 数据
            json root = json::parse(buf);
            std::string taskId = root["id"].get<std::string>();

            if (scheduler->removeTask(taskId, result_code, result_msg))
            {
                result_code = 0;
                result_msg = "success";
            }
            else{

                result_code = 2;
                result_msg = " remove task failed";
            };
            json result;
            result["msg"] = result_msg;
            result["code"] = result_code;

            LOG(INFO) << "\n \t Request:" << root.dump() << "\n \t Response:" << result.dump();

            api_response(req, result);
        }

        /* 获取指定任务的状态 */
        void api_getTaskState(struct evhttp_request *req, void *arg)
        {
            TaskManage::TaskScheduler *scheduler = (TaskManage::TaskScheduler *)arg;
            int result_code = -1;
            std::string result_msg = "";

            char buf[RECV_BUF_MAX_SIZE];
            /* 解析 HTTP 请求中传输的 JSON 数据到 buf 中 */
            parse_post(req, buf);

            // 解析 json 数据
            json root = json::parse(buf);
            std::string taskId = root["id"].get<std::string>();

            if (scheduler->getTaskState(taskId, result_code, result_msg))
            {
                result_code = 0;
                result_msg = "success";
            }
            else{

                result_code = 2;
                result_msg = " get task state failed";
            };
            json result;
            result["msg"] = result_msg;
            result["code"] = result_code;

            LOG(INFO) << "\n \t Request:" << root.dump() << "\n \t Response:" << result_msg;

            api_response(req, result);
        }

        /* api 响应, 用来发送数据 */
        void api_response(struct evhttp_request *req, json &result)
        {
            // ref: https://blog.csdn.net/u010710458/article/details/80055667
            struct evbuffer *buff = evbuffer_new();

            evbuffer_add_printf(buff, "%s", result.dump().c_str());
            evhttp_send_reply(req, HTTP_OK, nullptr, buff);
            evbuffer_free(buff);
        }

        void parse_get(struct evhttp_request *req, struct evkeyvalq *params)
        {
            if (req == nullptr)
            {
                return;
            }
            const char *url = evhttp_request_get_uri(req);
            if (url == nullptr)
            {
                return;
            }
            struct evhttp_uri *decoded = evhttp_uri_parse(url);
            if (!decoded)
            {
                return;
            }
            const char *path = evhttp_uri_get_path(decoded);
            if (path == nullptr)
            {
                path = "/";
            }
            char *query = (char *)evhttp_uri_get_query(decoded);
            if (query == nullptr)
            {
                return;
            }
            evhttp_parse_query_str(query, params);
        }

        /* 获取 HTTP 请求传输过来的数据, 最长 1024 * 8 字节数 */
        void parse_post(struct evhttp_request *req, char *buf)
        {
            size_t post_size = 0;
            /* 获取 HTTP 请求传输过来的数据长度 */
            post_size = evbuffer_get_length(req->input_buffer);
            if (post_size <= 0)
            {
                //        printf("====line:%d,post msg is empty!\n",__LINE__);
                return;
            }
            else
            {
                /* 根据传输来的数据内容长度和指定长度限制确定要读取的内容长度 */
                size_t copy_len = post_size > RECV_BUF_MAX_SIZE ? RECV_BUF_MAX_SIZE : post_size;
                //        printf("====line:%d,post len:%d, copy_len:%d\n",__LINE__,post_size,copy_len);
                /* 将传输来的数据中截取到的指定长度的内容存储到 buf 中 */
                memcpy(buf, evbuffer_pullup(req->input_buffer, -1), copy_len);
                buf[post_size] = '\0';
                //        printf("====line:%d,post msg:%s\n",__LINE__,buf);
            }
        }

    }
} // namespace CMA::Server