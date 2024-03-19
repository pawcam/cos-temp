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
bool CancelOrderMessageHandler::handleMessage(nlohmann::json& jMessage, std::string strTopic)
{
  if (jMessage.empty() || !jMessage.is_object())
  {
    return false;
  }

  if(sx_logGetBit(sx_log::SX_LOG_DEBUG))
  {
    SX_DEBUG("[%s]Attempting to handle a cancel message for order %s\n", strTopic, jMessage.dump());
  }

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
      if(sx_logGetBit(sx_log::SX_LOG_DEBUG))
      {
        uint32_t nGlobalOrderNum{std::numeric_limits<uint32_t>::max()};
        uint32_t nIdentifier{std::numeric_limits<uint32_t>::max()};
        const char* szOrderID{""};
        const char* szExchangeSymbol{""};
        auto pCancelWithDetailsMsg{sxORMsgWithType::getCancelWithDetailsMsgPtr(szMsg)};
        if(pCancelWithDetailsMsg)
        {
          nGlobalOrderNum   = pCancelWithDetailsMsg->nGlobalOrderNum;
          nIdentifier       = pCancelWithDetailsMsg->nowa.nIdentifier;
          szOrderID         = pCancelWithDetailsMsg->szOrderID;
          szExchangeSymbol  = pCancelWithDetailsMsg->nowa.no.szExchangeSymbol;
        }
        SX_DEBUG("%s Sent cancel request: "
                 "[GON:%u]"
                 "[Identifier:%u]"
                 "[OrderID:%s]"
                 "[message type:%d]"
                 "[ExchangeSymbol:%s]"
                 "\n"
                 , (bSent) ? "Successfully" : "Unsuccessfully"
                 , nGlobalOrderNum
                 , nIdentifier
                 , szOrderID
                 , szMsg.h.uchType
                 , szExchangeSymbol
                );
      }
      return bSent;
    }
  }
  catch (const std::exception &e)
  {
    SX_ERROR("Failed to process cancel request - JSON Parser Error %s\n", e.what());
    return false;
  }
  return true;
}
