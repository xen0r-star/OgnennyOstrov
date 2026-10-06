#ifndef GOULAG_MATCH_H
#define GOULAG_MATCH_H

#include <SFML/Config.hpp>
#include <SFML/System/Time.hpp>
#include <string>
#include <vector>

class Match {
public:
    Match(std::string code, int seed);

    void addPlayer(int playerId);
    void removePlayer(int playerId);
    void update(sf::Time deltaTime);

    bool isEmpty() const;
    bool isFull() const;

    const std::string& getCode() const { return code; }
    int getSeed() const { return seed; }
    sf::Uint32 getTick() const { return tick; }
    const std::vector<int>& getPlayers() const { return players; }

private:
    std::string code;
    int seed;
    sf::Uint32 tick = 0;
    std::vector<int> players;
};

#endif // GOULAG_MATCH_H
