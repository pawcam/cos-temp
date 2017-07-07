#include <iostream>

#include <signal.h>

#include <boost/lexical_cast.hpp>
#include <json/json.hpp>
#include <kr/sx_pl.h>
#include <kr/CmdLine.h>
#include <twLib/mq/MQAdapter.h>
#include <twLib/or/OR2Adapter.h>

#include "CancelOrderMessageHandler.h"

using namespace std;
using namespace boost;
using namespace TW;

int main(int argc, char *argv[]) {
    sx_setArgCArgV(argc, argv);

    CCmdLine cmdLine;
    cmdLine.SplitLine(argc, argv);

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
    string strMqExchangeName = getenv("MQ_EXCHANGE_NAME");
    string strORDefaultRoute = getenv("OR_DEFAULT_ROUTE");
    string strMqQueueName = getenv("MQ_QUEUE_NAME");

    TW::MQAdapter mqAdapter(strMqHost, unMQPort, strMqUsername,
                            strMqPassword, strMqVHost, strMqQueueName,
                            strMqExchangeName);

    TW::OR2Adapter or2Adapter(TW::OR2ClientMode::INPUT, strORDefaultRoute);

    CancelOrderMessageHandler messageHandler = CancelOrderMessageHandler(&or2Adapter);

    mqAdapter.setMessageHandler(&messageHandler);

    mqAdapter.start();
    or2Adapter.start();

    mqAdapter.join();
    or2Adapter.join();
    return 0;
}
