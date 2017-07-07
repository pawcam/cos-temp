#ifndef CANCELORDERMESSAGEHANDLER_H
#define CANCELORDERMESSAGEHANDLER_H

#include <twLib/mq/MessageHandler.h>

namespace TW {
class OR2Adapter;
}

class CancelOrderMessageHandler : public TW::MessageHandler {
public:
  CancelOrderMessageHandler(TW::OR2Adapter *pOR2Adapter);

  virtual bool handleMessage(nlohmann::json& jMessage, std::string strTopic) override;

private:
  TW::OR2Adapter *m_pOR2Adapter;
};

#endif //CANCELORDERMESSAGEHANDLER_H
