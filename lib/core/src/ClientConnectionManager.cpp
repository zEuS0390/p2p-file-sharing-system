#include "core/network/ClientConnectionManager.hpp"
#include "core/network/IMessageHandler.hpp"

ClientConnectionManager::ClientConnectionManager(
  IMessageHandler& message_handler
):
  ConnectionManager{message_handler}
{
}

