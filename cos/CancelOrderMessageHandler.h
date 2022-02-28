#ifndef CANCELORDERMESSAGEHANDLER_H
#define CANCELORDERMESSAGEHANDLER_H

#include <twLib/mq/MessageHandler.h>
#include <kr/MismatchedRootsMap.h>

namespace TW {
class SenderLocationReader;
class OR2Adapter;
}

class CancelOrderMessageHandler : public TW::MessageHandler {
public:
  CancelOrderMessageHandler(TW::OR2Adapter *pOR2Adapter, TW::SenderLocationReader *pSenderLocationReader, bool bDefaultRoute = false);

  virtual bool handleMessage(nlohmann::json& jMessage, std::string strTopic) override;

private:
  TW::OR2Adapter *m_pOR2Adapter;
  TW::SenderLocationReader *m_pSenderLocationReader;
  bool m_bDefaultRoute;
  MismatchedRootsMap m_rootMap;
  bool isGTCCancel(const nlohmann::json& jMessage);
  bool handleDayOrderMessage(nlohmann::json& jMessage);
  bool handleGTCOrderMessage(nlohmann::json& jMessage);
};

#endif //CANCELORDERMESSAGEHANDLER_H
