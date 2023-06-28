#include "CancelOrderMessageHandler.h"

#include <twLib/JsonOrderInterpreter.h>
#include <twLib/or/OR2Adapter.h>
#include <twLib/SenderLocationReader.h>

using namespace ::TW;
CancelOrderMessageHandler::CancelOrderMessageHandler(OR2Adapter *pOR2Adapter, TW::SenderLocationReader* pSenderLocationReader, bool bDefaultRoute)
  : m_pOR2Adapter(pOR2Adapter), m_pSenderLocationReader(pSenderLocationReader), m_bDefaultRoute(bDefaultRoute)
{
   m_rootMap.initFromDBOption("db_option_combined.out", true);
}

//----------------------------------
//----------------------------------
bool CancelOrderMessageHandler::handleMessage(nlohmann::json& jMessage, std::string UNUSED(strTopic))
{
  if (jMessage.empty() || !jMessage.is_object())
  {
    return false;
  }

  return handleCancelOrderMessage(jMessage);
}

//----------------------------------
//----------------------------------
bool CancelOrderMessageHandler::handleCancelOrderMessage(const nlohmann::json& jMessage)
{
  SX_DEBUG("Attempting to handle a cancel message for order %s\n", jMessage.dump());

  TW::JsonOrderInterpreter orderWrapper = TW::JsonOrderInterpreter(jMessage, m_pSenderLocationReader);
  const std::string strDestination = m_bDefaultRoute ? m_pOR2Adapter->getDefaultRoute() : MQUtil::extractDestination(jMessage, m_pOR2Adapter->getDefaultRoute());

  try {
    sxORMsgWithType szMsg = orderWrapper.to_OR2CancelMessageStruct(m_rootMap);

    if (szMsg.h.uchType == sxORMsgWithType::MSG_INVALID) {
      SX_ERROR("Failed to create cancel message: %d\n", orderWrapper.getOrderId());
      return false;
    }

    // message to OR
    if (szMsg.h.uchType == sxORMsgWithType::MSG_CANCEL_WITH_DETAILS ||
       (szMsg.h.uchType == sxORMsgWithType::MSG_COMPLEX_WRAPPER && szMsg.u.comp.nType == sxORMsgWithType::MSG_CANCEL_WITH_DETAILS))
    {
      const bool bSent = m_pOR2Adapter->sendCancel(szMsg, strDestination);
      SX_DEBUG("%s Sent cancel request: "
               "GON:%u "
               "Identifier:%u "
               "with sx message type %d "
               "and szExchangeSymbol %s"
               "\n"
               , (bSent) ? "Successfully" : "Unsuccessfully"
               , szMsg.u.cwd.nGlobalOrderNum
               , szMsg.u.cwd.nowa.nIdentifier
               , szMsg.h.uchType
               , szMsg.u.cwd.nowa.no.szExchangeSymbol
              );
      return bSent;
    }
  } catch (const std::exception &e) {
    SX_ERROR("Failed to process cancel request - JSON Parser Error %s\n", e.what());
    return false;
  }
  return true;
}
