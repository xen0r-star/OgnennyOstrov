#include "core/GameServer.h"

#include <algorithm>
#include <iostream>

#include "core/Config.h"
#include "network/Protocol.h"

GameServer::GameServer() : rng(std::random_device{}()) {}

void GameServer::run() {
    if (listener.listen(SERVER_PORT) != sf::Socket::Done) {
        std::cerr << "[SERVEUR] Impossible d'ouvrir le port TCP " << SERVER_PORT << std::endl;
        return;
    }
    if (udp.bind(SERVER_PORT) != sf::Socket::Done) {
        std::cerr << "[SERVEUR] Impossible d'ouvrir le port UDP " << SERVER_PORT << std::endl;
        return;
    }

    udp.setBlocking(false);
    selector.add(listener);
    selector.add(udp);
    running = true;

    std::cout << "[SERVEUR] Lance sur le port " << SERVER_PORT << " (TCP + UDP)" << std::endl;

    sf::Clock clock;
    const sf::Time tick = sf::seconds(1.f / TICK_RATE);
    sf::Time acc = sf::Time::Zero;

    while (running) {
        const sf::Time timeout = std::max(tick - acc, sf::milliseconds(1));

        if (selector.wait(timeout)) {
            if (selector.isReady(listener)) acceptClient();
            if (selector.isReady(udp)) receiveUdp();
            receiveTcp();
        }

        acc += clock.restart();
        if (acc > tick * 5.f) acc = tick * 5.f;

        while (acc >= tick) {
            acc -= tick;
            update(tick);
        }
    }
}

void GameServer::acceptClient() {
    auto client = std::make_unique<ClientSession>();
    if (listener.accept(client->tcp) != sf::Socket::Done) return;

    client->id = nextId++;
    client->token = static_cast<sf::Uint32>(rng());

    sf::Packet welcome;
    welcome << TcpType::Welcome << client->id << client->token;
    client->tcp.send(welcome);
    client->tcp.setBlocking(false);

    std::cout << "[SERVEUR] Joueur " << client->id << " connecte ("
              << client->tcp.getRemoteAddress() << ")" << std::endl;

    selector.add(client->tcp);
    clients.push_back(std::move(client));
}

void GameServer::receiveTcp() {
    for (auto it = clients.begin(); it != clients.end();) {
        ClientSession& client = **it;

        if (!selector.isReady(client.tcp)) {
            ++it;
            continue;
        }

        sf::Packet packet;
        const sf::Socket::Status status = client.tcp.receive(packet);

        if (status == sf::Socket::Disconnected || status == sf::Socket::Error) {
            std::cout << "[SERVEUR] Joueur " << client.id << " deconnecte" << std::endl;
            it = removeClient(it);
            continue;
        }

        if (status == sf::Socket::Done) {
            handleTcpPacket(client, packet);
        }
        ++it;
    }
}

void GameServer::handleTcpPacket(ClientSession& client, sf::Packet& packet) {
    TcpType type;
    if (!(packet >> type)) return;

    switch (type) {
        case TcpType::CreateMatch:
            createMatch(client);
            break;
        case TcpType::JoinMatch: {
            std::string code;
            if (!(packet >> code)) return;
            joinMatch(client, code);
            break;
        }
        default:
            break;
    }
}

void GameServer::createMatch(ClientSession& client) {
    if (!client.matchCode.empty()) {
        sendMatchError(client, "Deja dans une partie");
        return;
    }

    const std::string code = generateMatchCode();
    matches.emplace(code, std::make_unique<Match>(code, static_cast<int>(rng())));

    std::cout << "[SERVEUR] Partie " << code << " creee par le joueur " << client.id << std::endl;
    joinMatch(client, code);
}

void GameServer::joinMatch(ClientSession& client, const std::string& code) {
    if (!client.matchCode.empty()) {
        sendMatchError(client, "Deja dans une partie");
        return;
    }

    auto it = matches.find(code);
    if (it == matches.end()) {
        sendMatchError(client, "Partie introuvable");
        return;
    }

    Match& match = *it->second;
    if (match.isFull()) {
        sendMatchError(client, "Partie pleine");
        return;
    }

    match.addPlayer(client.id);
    client.matchCode = code;

    sf::Packet joined;
    joined << TcpType::MatchJoined << code << match.getSeed();
    client.tcp.send(joined);

    std::cout << "[SERVEUR] Joueur " << client.id << " rejoint la partie " << code
              << " (" << match.getPlayers().size() << "/" << MAX_PLAYERS_PER_MATCH << ")" << std::endl;
}

void GameServer::leaveMatch(ClientSession& client) {
    if (client.matchCode.empty()) return;

    auto it = matches.find(client.matchCode);
    if (it != matches.end()) {
        it->second->removePlayer(client.id);

        if (it->second->isEmpty()) {
            std::cout << "[SERVEUR] Partie " << client.matchCode << " supprimee (vide)" << std::endl;
            matches.erase(it);
        }
    }
    client.matchCode.clear();
}

void GameServer::sendMatchError(ClientSession& client, const std::string& message) {
    sf::Packet error;
    error << TcpType::MatchError << message;
    client.tcp.send(error);
}

std::string GameServer::generateMatchCode() {
    static const std::string alphabet = "ABCDEFGHJKLMNPQRSTUVWXYZ23456789";

    std::string code;
    do {
        code.clear();
        for (int i = 0; i < MATCH_CODE_LENGTH; ++i) {
            code += alphabet[rng() % alphabet.size()];
        }
    } while (matches.count(code) > 0);

    return code;
}

void GameServer::receiveUdp() {
    sf::Packet packet;
    sf::IpAddress address;
    unsigned short port;

    while (udp.receive(packet, address, port) == sf::Socket::Done) {
        UdpType type;
        if (!(packet >> type)) continue;

        switch (type) {
            case UdpType::Bind: {
                int id;
                sf::Uint32 token;
                if (!(packet >> id >> token)) break;

                ClientSession* client = findById(id);
                if (!client || client->token != token) break;

                if (!client->udpBound) {
                    std::cout << "[SERVEUR] Joueur " << client->id << " UDP lie ("
                              << address << ":" << port << ")" << std::endl;
                }

                client->udpAddress = address;
                client->udpPort = port;
                client->udpBound = true;
                client->lastSeen.restart();

                sf::Packet ack;
                ack << UdpType::BindAck;
                udp.send(ack, address, port);
                break;
            }
            case UdpType::Input: {
                ClientSession* client = findByEndpoint(address, port);
                if (!client || client->matchCode.empty()) break;

                sf::Uint32 seq;
                float dx, dy;
                if (!(packet >> seq >> dx >> dy)) break;

                client->lastSeen.restart();
                if (seq <= client->lastInputSeq) break;

                client->lastInputSeq = seq;
                client->moveX = dx;
                client->moveY = dy;
                break;
            }
            default:
                break;
        }
    }
}

void GameServer::removeTimedOutClients() {
    for (auto it = clients.begin(); it != clients.end();) {
        ClientSession& client = **it;

        if (client.udpBound && client.lastSeen.getElapsedTime().asSeconds() > CLIENT_TIMEOUT_SECONDS) {
            std::cout << "[SERVEUR] Joueur " << client.id << " timeout" << std::endl;
            it = removeClient(it);
        } else {
            ++it;
        }
    }
}

void GameServer::update(sf::Time deltaTime) {
    removeTimedOutClients();

    for (auto& entry : matches) {
        Match& match = *entry.second;
        match.update(deltaTime);

        sf::Packet state;
        state << UdpType::State << match.getTick();

        for (int playerId : match.getPlayers()) {
            ClientSession* client = findById(playerId);
            if (client && client->udpBound) {
                udp.send(state, client->udpAddress, client->udpPort);
            }
        }
    }
}

GameServer::ClientList::iterator GameServer::removeClient(ClientList::iterator it) {
    leaveMatch(**it);
    selector.remove((*it)->tcp);
    return clients.erase(it);
}

ClientSession* GameServer::findById(int id) {
    for (auto& client : clients) {
        if (client->id == id) return client.get();
    }
    return nullptr;
}

ClientSession* GameServer::findByEndpoint(const sf::IpAddress& address, unsigned short port) {
    for (auto& client : clients) {
        if (client->udpBound && client->udpAddress == address && client->udpPort == port) {
            return client.get();
        }
    }
    return nullptr;
}