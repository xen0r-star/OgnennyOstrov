#ifndef GOULAG_CLIENTSESSION_H
#define GOULAG_CLIENTSESSION_H

#include "SFML/Network.hpp"

struct ClientSession {
    sf::TcpSocket tcp;
    int id = 0;
    sf::Uint32 token = 0;

    sf::IpAddress udpAddress;
    unsigned short udpPort = 0;
    bool udpBound = false;

    std::string matchCode;

    sf::Uint32 lastInputSeq = 0;
    float moveX = 0.f;
    float moveY = 0.f;

    sf::Clock lastSeen;
};

#endif // GOULAG_CLIENTSESSION_H
