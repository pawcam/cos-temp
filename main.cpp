#include <iostream>

#include <signal.h>

#include <boost/lexical_cast.hpp>
#include <json/json.hpp>
#include <kr/CmdLine.h>
#include <kr/sx_pl.h>
#include <twLib/util.h>
#include <twLib/mq/MQAdapter.h>
#include <twLib/or/OR2Adapter.h>
#include <twLib/SenderLocationReader.h>
#include <twLib/models/Future.h>
#include <twLib/models/FutureOption.h>
#include "CancelOrderMessageHandler.h"

#include <kr/static_assertions.h>

#include "OR2Lib/OR2Constants.h"
#include "kr/SemanticVersion.h"
#include "kr/serialize.h"

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

    //TODO add AppVersion
    SX_WARN("[OR2Version:%s]"
            "[OR2Hash:%zu]"
            "\n"
            , sx::SemanticVersion64BitWrapper{OR2::VERSION}
            , sxORMsgWithType::HASH
           );

    // Required Environment Variables
    string strMqHost = getenv("MQ_HOST");
    uint16_t unMQPort = lexical_cast<uint16_t>(getenv("MQ_PORT"));
    string strMqUsername = getenv("MQ_USERNAME");
    string strMqPassword = getenv("MQ_PASSWORD");
    string strMqVHost = getenv("MQ_VHOST");
    string strMqDirectExchangeName = getenv("MQ_DIRECT_EXCHANGE_NAME");
    string strMqExchangeName = getenv("MQ_EXCHANGE_NAME");
    string strORDefaultRoute = getenv("OR_DEFAULT_ROUTE");
    string strMqQueueName = getenv("MQ_QUEUE_NAME");
    // Optional Environment Variables
    bool bMqSslEnabled = (TW::getEnv("MQ_SSL_ENABLED", "false") == "true" ? true : false);
    const string strMqSslCaCertPath = TW::getEnv("MQ_SSL_CA_CERT_PATH", "");
    const string strMqSslClientCertPath = TW::getEnv("MQ_SSL_CLIENT_CERT_PATH", "");
    const string strMqSslClientKeyPath = TW::getEnv("MQ_SSL_CLIENT_KEY_PATH", "");
    bool bMqSslVerifyHostname = (TW::getEnv("MQ_SSL_VERIFY_HOSTNAME", "false") == "true" ? true : false);

    std::vector <std::string> vFutureSymbolMappings = {
        "cme_db_future.out",
        "smalls_db_future.out",
        "cfe_db_future.out"
    };
    TW::Future::loadMultipleFutureSymbolMappings(vFutureSymbolMappings);

    std::vector <std::string> vFutureOptionSymbolMappings = {
        "cme_db_option.out",
        "smalls_db_option.out"
    };
    TW::FutureOption::loadMultipleFutureOptionSymbolMappings(vFutureOptionSymbolMappings);

    TW::SenderLocationReader locationReader;
    locationReader.readFile();

    sx_ThreadSafeLockUnlock lock;
    TW::OR2Adapter or2Adapter(TW::OR2ClientMode::INPUT, strORDefaultRoute, "COS", 100, false, &lock);

    string strBindingKey = bDirectExchange ? strMqQueueName : "";
    TW::MQAdapter mqAdapter(strMqHost, unMQPort, strMqUsername,
                            strMqPassword, strMqVHost, strMqQueueName,
                            strMqExchangeName, "MQAdapter", strBindingKey, bDirectExchange, strMqDirectExchangeName,
                            bMqSslEnabled, strMqSslCaCertPath, strMqSslClientKeyPath, strMqSslClientCertPath, bMqSslVerifyHostname);

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
