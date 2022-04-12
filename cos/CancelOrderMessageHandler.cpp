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

  if (isGTCCancel(jMessage) == true) {
    return handleGTCOrderMessage(jMessage);
  }

  return handleDayOrderMessage(jMessage);
}

//----------------------------------
//----------------------------------
bool CancelOrderMessageHandler::handleDayOrderMessage(nlohmann::json& jMessage)
{
  if (!jMessage["ext-global-order-number"].is_number_integer())
  {
    return false;
  }

  uint32_t uCancelUserIdentifier = 0;
  if (jMessage["cancel-user-id"].is_number_integer())
  {
	  uCancelUserIdentifier = jMessage["cancel-user-id"];
  }

  int32_t nCmtaId = JsonOrderInterpreter::DEFAULT_CMTA_ID;
  if (jMessage["cmta-id"].is_number_integer())
  {
    nCmtaId = jMessage["cmta-id"];
  }

  const uint32_t nGlobalOrderNum = jMessage["ext-global-order-number"];
  const string   strDestination  = m_bDefaultRoute ? m_pOR2Adapter->getDefaultRoute() : MQUtil::extractDestination(jMessage, m_pOR2Adapter->getDefaultRoute());

  if(m_pSenderLocationReader) {

    return m_pOR2Adapter->sendCancel(numeric_limits<uint32_t>::max(), nGlobalOrderNum, strDestination.c_str(),
                                     uCancelUserIdentifier, m_pSenderLocationReader->getSenderLocation(uCancelUserIdentifier), nCmtaId);
  }
  return m_pOR2Adapter->sendCancel(numeric_limits<uint32_t>::max(), nGlobalOrderNum, strDestination.c_str(), uCancelUserIdentifier, "US", nCmtaId);
}

//----------------------------------
//----------------------------------
bool CancelOrderMessageHandler::handleGTCOrderMessage(nlohmann::json& jMessage)
{
  SX_DEBUG("Attempting to handle a GTC cancel message for order %s\n", jMessage.dump());

  TW::JsonOrderInterpreter orderWrapper = TW::JsonOrderInterpreter(jMessage, m_pSenderLocationReader);
  const std::string strDestination = m_bDefaultRoute ? m_pOR2Adapter->getDefaultRoute() : MQUtil::extractDestination(jMessage, m_pOR2Adapter->getDefaultRoute());

  try {
    sxORMsgWithType szMsg = orderWrapper.to_OR2CancelMessageStruct(m_rootMap);

    if (szMsg.h.uchType == sxORMsgWithType::MSG_INVALID) {
      SX_ERROR("Failed to create GTC cancel message: %d\n", orderWrapper.getOrderId());
      return false;
    }

    // message to OR
    if (szMsg.h.uchType == sxORMsgWithType::MSG_CANCEL_WITH_DETAILS ||
       (szMsg.h.uchType == sxORMsgWithType::MSG_COMPLEX_WRAPPER && szMsg.u.comp.nType == sxORMsgWithType::MSG_CANCEL_WITH_DETAILS))
    {
      SX_DEBUG("Sent complex GTC cancel request: %d with sx message type %d\n", szMsg.u.cwd.nGlobalOrderNum, szMsg.h.uchType);
      return m_pOR2Adapter->sendCancel(szMsg, strDestination);
    }
  } catch (const std::exception &e) {
    SX_ERROR("Failed to process GTC cancel request - JSON Parser Error %s\n", e.what());
    return false;
  }
  return true;
}

//----------------------------------
//----------------------------------
bool CancelOrderMessageHandler::isGTCCancel(const nlohmann::json& jMessage)
{
  std::string tif;
  if (jMessage.count("time-in-force") > 0 && jMessage.at("time-in-force").is_string())
  {
    tif = jMessage.at("time-in-force");
  }
  const std::string tifGTC = ORStrings::toString(sxORMsgWithType::GTC);
  const std::string tifGTD = ORStrings::toString(sxORMsgWithType::GTD);
  if (tif == tifGTC || tif == tifGTD) return true;
  return false;
}
