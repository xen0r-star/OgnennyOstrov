#include "models/map/generation/Pathfinding.h"

#include <algorithm>
#include <array>
#include <climits>
#include <queue>
#include <tuple>

namespace MapGen {

std::vector<Point> findRoute(const TileMap& map, const Point start, const Point goal,
                             const std::function<bool(int, int)>& isBlocked) {
    static const int dx[4] = {1, -1, 0, 0};
    static const int dy[4] = {0, 0, 1, -1};
    const int W = static_cast<int>(map.size());
    const int H = map.empty() ? 0 : static_cast<int>(map[0].size());
    auto idx = [&](int x, int y) { return y * W + x; };

    std::vector dist(
        (size_t)W * H, std::array<int, 4>{INT_MAX, INT_MAX, INT_MAX, INT_MAX});
    std::vector prevPos(
        (size_t)W * H, std::array<Point, 4>{Point{-1, -1}, Point{-1, -1}, Point{-1, -1}, Point{-1, -1}});
    std::vector prevDir(
        (size_t)W * H, std::array<int, 4>{-1, -1, -1, -1});

    using State = std::tuple<int, int, int, int>; // cost, x, y, dir
    std::priority_queue<State, std::vector<State>, std::greater<>> pq;

    for (int d = 0; d < 4; ++d) {
        dist[idx(start.first, start.second)][d] = 0;
        pq.emplace(0, start.first, start.second, d);
    }

    while (!pq.empty()) {
        auto [cost, x, y, d] = pq.top();
        pq.pop();
        if (cost > dist[idx(x, y)][d]) continue;
        if (x == goal.first && y == goal.second) break;

        for (int nd = 0; nd < 4; ++nd) {
            int nx = x + dx[nd], ny = y + dy[nd];
            if (nx < 0 || ny < 0 || nx >= W || ny >= H) continue;

            bool isGoal = (nx == goal.first && ny == goal.second);
            if (!isGoal && isBlocked(nx, ny)) continue;

            const int turnPenalty = (d == nd) ? 0 : 2;
            int newCost = cost + 1 + turnPenalty;
            if (newCost < dist[idx(nx, ny)][nd]) {
                dist[idx(nx, ny)][nd] = newCost;
                prevPos[idx(nx, ny)][nd] = {x, y};
                prevDir[idx(nx, ny)][nd] = d;
                pq.emplace(newCost, nx, ny, nd);
            }
        }
    }

    int bestDir = -1, bestCost = INT_MAX;
    for (int d = 0; d < 4; ++d) {
        int c = dist[idx(goal.first, goal.second)][d];
        if (c < bestCost) { bestCost = c; bestDir = d; }
    }
    if (bestDir == -1) return {};

    std::vector<Point> path;
    int x = goal.first, y = goal.second, d = bestDir;
    while (!(x == start.first && y == start.second)) {
        path.emplace_back(x, y);
        Point p = prevPos[idx(x, y)][d];
        int pd = prevDir[idx(x, y)][d];
        if (p.first == -1) break;
        x = p.first; y = p.second; d = pd;
    }
    path.push_back(start);
    std::reverse(path.begin(), path.end());
    return path;
}

} // namespace MapGen
