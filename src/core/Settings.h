#ifndef GOULAG_SETTINGS_H
#define GOULAG_SETTINGS_H

struct Settings {
    int brightness = 100; // from 20 to 100 %
    int volume = 100;     // from 0 to 100 %
    int character = 0;    // from 0 to 3

    void changeBrightness(int delta);
    void changeVolume(int delta);
};

#endif //GOULAG_SETTINGS_H