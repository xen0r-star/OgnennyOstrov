#include "Settings.h"

#include <SFML/Audio.hpp>
#include <algorithm>

namespace Settings
{
    int luminosite = 100;
    int volume = 100;
    int perso = 0;

    void changerLuminosite(int ecart)
    {
        luminosite = std::min(100, std::max(20, luminosite + ecart));
    }

    void changerVolume(int ecart)
    {
        volume = std::min(100, std::max(0, volume + ecart));
        sf::Listener::setGlobalVolume((float)volume); // agit sur tous les sons et musiques SFML
    }

    std::string cheminPerso()
    {
        return "assets/characters/" + FICHIERS_PERSOS[perso];
    }

    void appliquerLuminosite(sf::RenderWindow& fenetre)
    {
        // Un voile noir plus ou moins transparent par-dessus tout l'écran
        sf::View vueDuJeu = fenetre.getView();
        fenetre.setView(fenetre.getDefaultView());

        sf::RectangleShape voile(sf::Vector2f(fenetre.getSize()));
        voile.setFillColor(sf::Color(0, 0, 0, 255 * (100 - luminosite) / 100));
        fenetre.draw(voile);

        fenetre.setView(vueDuJeu);
    }
}
