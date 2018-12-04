#include "CancelOrderMessageHandler.h"

#include <twLib/or/OR2Adapter.h>
#include <twLib/SenderLocationReader.h>

using namespace ::TW;
CancelOrderMessageHandler::CancelOrderMessageHandler(OR2Adapter *pOR2Adapter, TW::SenderLocationReader* pSenderLocationReader)
  : m_pOR2Adapter(pOR2Adapter), m_pSenderLocationReader(pSenderLocationReader)
{
}


bool CancelOrderMessageHandler::handleMessage(nlohmann::json& jMessage, std::string UNUSED(strTopic))
{
  if (jMessage.empty())
  {
    return false;
  }
  if (!jMessage.is_object())
  {
    return false;
  }
  if (!jMessage["ext-global-order-number"].is_number_integer())
  {
    return false;
  }

  uint32_t uCancelUserIdentifier = 0;
  if (jMessage["cancel-user-id"].is_number_integer())
  {
	  uCancelUserIdentifier = jMessage["cancel-user-id"];
  }

  const uint32_t nGlobalOrderNum = jMessage["ext-global-order-number"];
  const string   strDestination  = MQUtil::extractDestination(jMessage, m_pOR2Adapter->getDefaultRoute());

  if(m_pSenderLocationReader) {

    return m_pOR2Adapter->sendCancel(numeric_limits<uint32_t>::max(), nGlobalOrderNum, strDestination.c_str(),
                                     uCancelUserIdentifier, m_pSenderLocationReader->getSenderLocation(uCancelUserIdentifier));
  }
  return m_pOR2Adapter->sendCancel(numeric_limits<uint32_t>::max(), nGlobalOrderNum, strDestination.c_str(), uCancelUserIdentifier, "US");
}
