#include "Settings.h"

#include <SFML/Audio.hpp>
#include <algorithm>

void Settings::changeBrightness(const int delta) {
    brightness = std::min(100, std::max(20, brightness + delta));
}

void Settings::changeVolume(const int delta) {
    volume = std::min(100, std::max(0, volume + delta));
    sf::Listener::setGlobalVolume(static_cast<float>(volume)); // affects every SFML sound and music
}