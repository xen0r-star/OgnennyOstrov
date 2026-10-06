#include "Match.h"
#include <algorithm>
#include <utility>

#include "core/Config.h"

Match::Match(std::string code, int seed) : code(std::move(code)), seed(seed) {}

void Match::addPlayer(int playerId) {
    players.push_back(playerId);
}

void Match::removePlayer(int playerId) {
    players.erase(std::remove(players.begin(), players.end(), playerId), players.end());
}

void Match::update(sf::Time) {
    ++tick;
}

bool Match::isEmpty() const {
    return players.empty();
}

bool Match::isFull() const {
    return static_cast<int>(players.size()) >= MAX_PLAYERS_PER_MATCH;
}