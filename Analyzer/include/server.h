#ifndef __CMA_Server_H__
#define __CMA_Server_H__

#include <event2/http.h>
#include "scheduler.h"
#include "json.hpp"

using json = nlohmann::json;

namespace CMA
{

namespace Server{


    class Server
    {
    public:
        explicit Server();
        ~Server();

    public:
        void start(TaskManage::TaskScheduler *scheduler);
    };

    void api_help(struct evhttp_request *req, void *arg);
    void api_addTask(struct evhttp_request *req, void *arg);
    void api_pauseTask(struct evhttp_request *req, void *arg);
    void api_startTask(struct evhttp_request *req, void *arg);
    void api_removeTask(struct evhttp_request *req, void *arg);
    void api_getTaskState(struct evhttp_request *req, void *arg);

    void api_response(struct evhttp_request *req, json &result);
    void parse_get(struct evhttp_request *req, struct evkeyvalq *params);
    void parse_post(struct evhttp_request *req, char *buff);
    
}
}

#endif // __CMA_Server_H__
