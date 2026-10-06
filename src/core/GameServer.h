#ifndef GOULAG_GAME_SERVER_H
#define GOULAG_GAME_SERVER_H

#include <SFML/Network.hpp>
#include <map>
#include <memory>
#include <random>
#include <string>
#include <vector>

#include "controllers/server/ClientSession.h"
#include "controllers/server/Match.h"

class GameServer {
public:
    GameServer();
    void run();

private:
    using ClientList = std::vector<std::unique_ptr<ClientSession>>;

    void acceptClient();
    void receiveTcp();
    void receiveUdp();
    void handleTcpPacket(ClientSession& client, sf::Packet& packet);
    void removeTimedOutClients();
    void update(sf::Time deltaTime);

    void createMatch(ClientSession& client);
    void joinMatch(ClientSession& client, const std::string& code);
    void leaveMatch(ClientSession& client);
    void sendMatchError(ClientSession& client, const std::string& message);
    std::string generateMatchCode();

    ClientList::iterator removeClient(ClientList::iterator it);
    ClientSession* findById(int id);
    ClientSession* findByEndpoint(const sf::IpAddress& address, unsigned short port);

    sf::TcpListener listener;
    sf::UdpSocket udp;
    sf::SocketSelector selector;
    ClientList clients;
    std::map<std::string, std::unique_ptr<Match>> matches;

    std::mt19937 rng;
    int nextId = 1;
    bool running = false;
};

#endif // GOULAG_GAME_SERVER_H
