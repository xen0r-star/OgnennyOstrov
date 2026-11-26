#include "models/entities/Bot.h"

#include "core/Config.h"
#include "models/map/Hitbox.h"
#include <cmath>

namespace {
    // Réglages : touche uniquement à ces chiffres pour changer la difficulté
    constexpr float WALK = 0.45f;            // vitesse en patrouille (x vitesse du joueur)
    constexpr float RUN = 0.9f;              // vitesse en poursuite : un peu moins vite que toi
    constexpr float VIEW_RANGE = 650.f;      // distance de vue (px)
    constexpr float VIEW_COS = 0.5f;         // cône de vision : 0.5 = 120° au total
    constexpr float FEEL_RANGE = 140.f;      // tout près, il te sent même dans son dos
    constexpr float ATTACK_RANGE = 100.f;    // distance pour frapper (centre à centre)
    constexpr float REACTION_TIME = 0.5f;    // temps avant de te poursuivre
    constexpr float FORGET_TIME = 1.5f;      // méfiant sans te voir -> il oublie
    constexpr float WINDUP_TIME = 0.4f;      // il arme son coup (le temps d'esquiver)
    constexpr float COOLDOWN_TIME = 1.2f;    // temps entre deux coups
    constexpr float SEARCH_TIME = 5.f;       // durée de la recherche

    float length(const sf::Vector2f v) { return std::sqrt(v.x * v.x + v.y * v.y); }

    float dot(const sf::Vector2f a, const sf::Vector2f b) { return a.x * b.x + a.y * b.y; }

    sf::Vector2f normalize(const sf::Vector2f v) {
        const float l = length(v);
        return l > 0.0001f ? v / l : sf::Vector2f(0.f, 0.f);
    }

    // Vrai si aucune hitbox de la map ne coupe la ligne entre a et b (on teste tous les 10 px)
    bool lineIsClear(const sf::Vector2f a, const sf::Vector2f b, const Map& map) {
        const int steps = static_cast<int>(length(b - a) / 10.f);

        for (int i = 1; i < steps; ++i) {
            const sf::Vector2f p = a + (b - a) * (static_cast<float>(i) / steps);
            const int x = static_cast<int>(std::floor(p.x / TILE_STEP));
            const int y = static_cast<int>(std::floor(p.y / TILE_STEP));
            if (x < 0 || y < 0 || x >= map.width || y >= map.height) return false;

            for (const Hitbox::Polygon& polygon : map.cells[y][x].hitboxes) {
                if (Hitbox::circleTouchesPolygon(p, 2.f, polygon)) return false;
            }
        }
        return true;
    }
}

Bot::Bot(const unsigned int seed) : rng(seed) {}

// ---------- Le chef d'orchestre : percevoir, décider, agir ----------
void Bot::update(const sf::Time dt, const Prisoner& target, const Map& map) {
    const float seconds = dt.asSeconds();
    hasHit = false;
    timeInState += seconds;
    if (cooldown > 0.f) cooldown -= seconds;

    // 1. PERCEVOIR
    const bool seen = sees(target, map);
    if (seen) lastSeen = target.getPosition();
    const float distance = length(target.getPosition() - body.getPosition());

    // 2 + 3. DÉCIDER ET AGIR
    switch (state) {
        case BotState::Patrol:
            if (seen) changeState(BotState::Suspicious);
            else patrol(seconds, map);
            break;
        case BotState::Suspicious: suspicious(seen);          break;
        case BotState::Chase:      chase(seen, distance);     break;
        case BotState::Attack:     attack(seconds, distance); break;
        case BotState::Search:     search(seconds, seen);     break;
    }

    // Le corps fait le reste : collisions avec les murs, comme le joueur
    body.setAimDirection(facing);
    body.update(dt, map);
}

// ---------- Les états ----------
void Bot::patrol(const float dt, const Map& map) {
    if (pause > 0.f) { pause -= dt; stop(); return; }

    if (!hasPatrolPoint) {
        hasPatrolPoint = pickPatrolPoint(map);
        patrolTime = 0.f;
        if (!hasPatrolPoint) { pause = 0.5f; return; }   // rien trouvé, on réessaie plus tard
    }

    patrolTime += dt;
    const bool arrived = length(patrolPoint - body.getPosition()) < 10.f;

    if (arrived || patrolTime > 6.f) {   // 6 s sans arriver = il est bloqué, il change d'idée
        hasPatrolPoint = false;
        pause = randomBetween(0.5f, 2.f); // pause au hasard = "il réfléchit"
        stop();
        return;
    }
    moveTowards(patrolPoint, WALK);
}

void Bot::suspicious(const bool sees) {
    stop();
    lookAt(lastSeen);   // il se tourne vers toi

    if (sees && timeInState > REACTION_TIME)       changeState(BotState::Chase);   // "c'est bien lui !"
    else if (!sees && timeInState > FORGET_TIME)   changeState(BotState::Patrol);  // "j'ai rêvé"
}

void Bot::chase(const bool sees, const float distance) {
    moveTowards(lastSeen, RUN);

    if (distance < ATTACK_RANGE)   changeState(BotState::Attack);
    else if (!sees)                changeState(BotState::Search);
}

void Bot::attack(const float dt, const float distance) {
    stop();
    lookAt(lastSeen);   // il te fait face

    // Trop loin et pas en train d'armer : il te court après
    if (windup < 0.f && distance > ATTACK_RANGE * 1.5f) { changeState(BotState::Chase); return; }

    if (windup < 0.f && cooldown <= 0.f) windup = WINDUP_TIME;   // il lève le bras

    if (windup > 0.f) {
        windup -= dt;
        if (windup <= 0.f) {                                     // le coup part
            if (distance < ATTACK_RANGE) hasHit = true;          // touché seulement si tu es encore là
            cooldown = COOLDOWN_TIME;
            windup = -1.f;
        }
    }
}

void Bot::search(const float dt, const bool sees) {
    if (sees) { changeState(BotState::Chase); return; }

    if (length(lastSeen - body.getPosition()) > 10.f && timeInState < SEARCH_TIME / 2.f) {
        moveTowards(lastSeen, WALK);
    } else {
        // Sur place : il tourne sur lui-même pour regarder autour
        stop();
        const float angle = 2.f * dt;
        const sf::Vector2f d = facing;
        facing = {d.x * std::cos(angle) - d.y * std::sin(angle),
                  d.x * std::sin(angle) + d.y * std::cos(angle)};
    }

    if (timeInState > SEARCH_TIME) changeState(BotState::Patrol);
}

// ---------- Les outils ----------
void Bot::moveTowards(const sf::Vector2f point, const float speedFactor) {
    const sf::Vector2f toPoint = point - body.getPosition();
    if (length(toPoint) < 5.f) { stop(); return; }

    lookAt(point);
    body.setDirection(facing * speedFactor);   // direction plus courte = il va moins vite
}

void Bot::lookAt(const sf::Vector2f point) {
    const sf::Vector2f d = normalize(point - body.getPosition());
    if (d.x != 0.f || d.y != 0.f) facing = d;   // s'il est pile dessus, il garde sa direction
}

void Bot::stop() {
    body.setDirection({0.f, 0.f});
}

bool Bot::sees(const Prisoner& target, const Map& map) const {
    const sf::Vector2f toTarget = target.getPosition() - body.getPosition();
    const float distance = length(toTarget);

    if (distance > VIEW_RANGE) return false;                                // Test 1 : trop loin

    if (distance > FEEL_RANGE) {
        if (dot(facing, toTarget / distance) < VIEW_COS) return false;     // Test 2 : pas devant lui

        const int x = static_cast<int>(std::floor(target.getPosition().x / TILE_STEP));
        const int y = static_cast<int>(std::floor(target.getPosition().y / TILE_STEP));
        if (map.cells[y][x].hides) return false;                           // Test 3 : caché dans un buisson
    }

    return lineIsClear(body.getPosition(), target.getPosition(), map);     // Test 4 : un mur entre nous
}

// Choisit un point au hasard, pas trop loin, qu'il peut atteindre en ligne droite
bool Bot::pickPatrolPoint(const Map& map) {
    const sf::Vector2f start = body.getPosition();

    for (int attempt = 0; attempt < 20; ++attempt) {
        const float angle = randomBetween(0.f, 6.2832f);
        const float distance = randomBetween(2.f, 6.f) * TILE_STEP;
        const sf::Vector2f point = start + sf::Vector2f(std::cos(angle), std::sin(angle)) * distance;

        // On vérifie que son corps entier passe tout le long du trajet (tous les 20 px)
        bool free = true;
        const int steps = static_cast<int>(distance / 20.f);
        for (int i = 1; i <= steps && free; ++i) {
            free = Prisoner::canStandAt(start + (point - start) * (static_cast<float>(i) / steps), map);
        }

        if (free) {
            patrolPoint = point;
            return true;
        }
    }
    return false;
}

float Bot::randomBetween(const float min, const float max) {
    return std::uniform_real_distribution(min, max)(rng);
}

void Bot::changeState(const BotState newState) {
    state = newState;
    timeInState = 0.f;
    windup = -1.f;
    hasPatrolPoint = false;
}