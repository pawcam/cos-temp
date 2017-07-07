#include "CancelOrderMessageHandler.h"
#include <twLib/or/OR2Adapter.h>

using namespace ::TW;
CancelOrderMessageHandler::CancelOrderMessageHandler(OR2Adapter *pOR2Adapter)
  : m_pOR2Adapter(pOR2Adapter)
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

  const uint32_t nGlobalOrderNum = jMessage["ext-global-order-number"];
  const string   strDestination  = MQUtil::extractDestination(jMessage, m_pOR2Adapter->getDefaultRoute());
  return m_pOR2Adapter->sendCancel(numeric_limits<uint32_t>::max(), nGlobalOrderNum, strDestination.c_str());
}
