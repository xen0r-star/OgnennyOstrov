#ifndef GOULAG_PROTOCOL_H
#define GOULAG_PROTOCOL_H

#include <type_traits>
#include <SFML/Config.hpp>
#include <SFML/Network/Packet.hpp>

enum class TcpType : sf::Uint8 {
  Welcome,
  CreateMatch,
  JoinMatch,
  MatchJoined,
  MatchError
};

enum class UdpType : sf::Uint8 {
  Bind,
  BindAck,
  Input,
  State
};

template <typename E, typename = std::enable_if_t<std::is_enum_v<E>>>
sf::Packet& operator<<(sf::Packet& packet, E type) {
  return packet << static_cast<sf::Uint8>(type);
}

template <typename E, typename = std::enable_if_t<std::is_enum_v<E>>>
sf::Packet& operator>>(sf::Packet& packet, E& type) {
  sf::Uint8 raw = 0;
  packet >> raw;
  type = static_cast<E>(raw);
  return packet;
}

#endif // GOULAG_PROTOCOL_H
