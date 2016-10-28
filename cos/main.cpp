#include <iostream>

#include <signal.h>

#include <boost/lexical_cast.hpp>
#include <json/json.hpp>
#include <kr/sx_pl.h>
#include <kr/CmdLine.h>
#include <twMagic/service/MessageRelayService.h>
#include <twMagic/cancelorders/CancelOrderService.h>

using namespace std;
using namespace boost;
using namespace TW;

typedef MessageRelayService<CancelOrderService> service_t;
typedef service_t::OELAdapterType OELAdapterType;

service_t *service;

void exit_cleanly(int sig) {
    cout << "Received signal " << sig << "; interrupting process" << endl;
    service->stop();
}

int main(int argc, char *argv[]) {
    signal(SIGINT, exit_cleanly);

    sx_setArgCArgV(argc, argv);

    CCmdLine cmdLine;
    cmdLine.SplitLine(argc, argv);

#ifdef _DEBUG
    cout << "Enabling debug logging" << endl;
    sx_log::Instance().setBit(sx_log::SX_LOG_DEBUG, true);
#endif

    string strInstanceId = getenv("SERVICE_INSTANCE_ID");
    string strMqHost = getenv("MQ_HOST");
    uint16_t strMqPort = lexical_cast<uint16_t>(getenv("MQ_PORT"));
    string strMqUsername = getenv("MQ_USERNAME");
    string strMqPassword = getenv("MQ_PASSWORD");
    string strMqVHost = getenv("MQ_VHOST");
    string strMqExchangeName = getenv("MQ_EXCHANGE_NAME");
    string strORDefaultRoute = getenv("OR_DEFAULT_ROUTE");
    string strMqQueueName = getenv("MQ_QUEUE_NAME");

    OELAdapterType *pOelAdapter = new OELAdapterType(INPUT, strORDefaultRoute);
    service = new service_t(strInstanceId, strMqHost, strMqPort, strMqUsername, strMqPassword, strMqVHost, strMqQueueName, strMqExchangeName,
                            pOelAdapter);
    service->start();
    service->join();
    delete service;
    service = nullptr;
    exit(0);
}
