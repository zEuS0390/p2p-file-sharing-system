#ifndef CORE_NETWORK_MESSAGE_HANDLER_HPP
#define CORE_NETWORK_MESSAGE_HANDLER_HPP

#include <type_traits>
#include <string_view>
#include <functional>
#include <cstdint>
#include <tuple>

#include "core/types/MessageType.hpp"
#include "core/types/Connection.hpp"

namespace message_handler_detail
{
  template <MessageType T>
  struct MessageTraits;

  template <>
  struct MessageTraits<MessageType::MESSAGE_RESPONSE>
  {
    using Handler = std::function<void(std::uint64_t, std::string_view)>;
  };

  template <>
  struct MessageTraits<MessageType::FILE_DOWNLOAD_RESPONSE>
  {
    using Handler = std::function<void(std::uint64_t, std::uint64_t)>;
  };

  template <>
  struct MessageTraits<MessageType::FILE_UPLOAD_RESPONSE>
  {
    using Handler = std::function<void(std::uint64_t)>;
  };

  template <>
  struct MessageTraits<MessageType::FILE_UPLOAD_PROGRESS>
  {
    using Handler = std::function<void(std::uint64_t, std::uint64_t, std::uint64_t)>;
  };

  template <>
  struct MessageTraits<MessageType::FILE_CHUNK>
  {
    using Handler = std::function<void(std::uint64_t, std::uint64_t)>;
  };

  template <>
  struct MessageTraits<MessageType::FILE_ERROR>
  {
    using Handler = std::function<void(std::uint64_t, std::string_view)>;
  };

  template <MessageType T>
  struct HandlerSlot
  {
    using Handler = typename MessageTraits<T>::Handler;
    Handler function;
  };
}

/*
 * Handles message operations for the ConnectionManager.
 */
class MessageHandler
{
private:
  using Handlers = std::tuple<
    message_handler_detail::HandlerSlot<MessageType::MESSAGE_RESPONSE>,
    message_handler_detail::HandlerSlot<MessageType::FILE_DOWNLOAD_RESPONSE>,
    message_handler_detail::HandlerSlot<MessageType::FILE_UPLOAD_RESPONSE>,
    message_handler_detail::HandlerSlot<MessageType::FILE_UPLOAD_PROGRESS>,
    message_handler_detail::HandlerSlot<MessageType::FILE_CHUNK>,
    message_handler_detail::HandlerSlot<MessageType::FILE_ERROR>
  >;
  Handlers handlers;
public:
  void appendBytes(std::vector<char>& buffer, const void* data, std::size_t size_t);
  void dispatchMessage(Connection& connection, const char* data);
  void queueMessage(Connection& connection, const MessageType& message_type, const char* data, size_t data_size);
  bool queueNextFileChunk(Connection& connection, std::uint64_t transfer_id);
  template <
    MessageType T,
    typename Func,
    typename = std::enable_if_t<
      std::is_constructible_v<
        typename message_handler_detail::MessageTraits<T>::Handler, Func
      >
    >
  >
  void on(Func&& func);
};

template <MessageType T, typename Func, typename>
void MessageHandler::on(Func&& func)
{
  std::get<message_handler_detail::HandlerSlot<T> >(handlers).function = std::forward<Func>(func);
}

#endif // CORE_NETWORK_MESSAGE_HANDLER_HPP

