#ifndef GOULAG_MAPTYPES_H
#define GOULAG_MAPTYPES_H

#include <random>
#include <utility>

namespace MapGen {

using Point = std::pair<int, int>;
using Rng   = std::mt19937;

struct Rect {
    int x, y, w, h;
    int right()  const { return x + w - 1; }
    int bottom() const { return y + h - 1; }
};

enum HouseType {
    HOUSE_BARRACKS = 0,
    HOUSE_MESS     = 1,
    HOUSE_STORAGE  = 2
};

struct House : Rect {
    int type = HOUSE_BARRACKS;
};

}

#endif // GOULAG_MAPTYPES_H
