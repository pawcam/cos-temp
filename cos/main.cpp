#include <iostream>

#include <signal.h>

#include <boost/lexical_cast.hpp>
#include <json/json.hpp>
#include <kr/CmdLine.h>
#include <kr/sx_pl.h>
#include <twLib/mq/MQAdapter.h>
#include <twLib/or/OR2Adapter.h>
#include <twLib/SenderLocationReader.h>

#include "CancelOrderMessageHandler.h"

using namespace std;
using namespace boost;
using namespace TW;

int main(int argc, char *argv[]) {
    sx_setArgCArgV(argc, argv);

    CCmdLine cmdLine;
    cmdLine.SplitLine(argc, argv);
    bool bDirectExchange = cmdLine.HasSwitch("--direct_exchange");

#ifdef _DEBUG
    cout << "Enabling debug logging" << endl;
    sx_log::Instance().setBit(sx_log::SX_LOG_DEBUG, true);
#endif

    string strInstanceId = getenv("SERVICE_INSTANCE_ID");
    string strMqHost = getenv("MQ_HOST");
    uint16_t unMQPort = lexical_cast<uint16_t>(getenv("MQ_PORT"));
    string strMqUsername = getenv("MQ_USERNAME");
    string strMqPassword = getenv("MQ_PASSWORD");
    string strMqVHost = getenv("MQ_VHOST");
    string strMqDirectExchangeName = getenv("MQ_DIRECT_EXCHANGE_NAME");
    string strMqExchangeName = getenv("MQ_EXCHANGE_NAME");
    string strORDefaultRoute = getenv("OR_DEFAULT_ROUTE");
    string strMqQueueName = getenv("MQ_QUEUE_NAME");

    TW::SenderLocationReader locationReader;
    locationReader.readFile();

    sx_ThreadSafeLockUnlock lock;
    TW::OR2Adapter or2Adapter(TW::OR2ClientMode::INPUT, strORDefaultRoute, "OR2Adapter", 100, false, &lock);

    string strBindingKey = bDirectExchange ? strMqQueueName : "";
    TW::MQAdapter mqAdapter(strMqHost, unMQPort, strMqUsername,
                            strMqPassword, strMqVHost, strMqQueueName,
                            strMqExchangeName, "MQAdapter", strBindingKey, bDirectExchange, strMqDirectExchangeName);
    CancelOrderMessageHandler messageHandler = CancelOrderMessageHandler(&or2Adapter, &locationReader, bDirectExchange);
    mqAdapter.setMessageHandler(&messageHandler);

    or2Adapter.start();
    lock.Lock(__FILE__, __LINE__);
    lock.Unlock();
    mqAdapter.start();

    or2Adapter.join();
    mqAdapter.join();
    return 0;
}
