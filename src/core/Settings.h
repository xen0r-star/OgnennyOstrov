#ifndef GOULAG_SETTINGS_H
#define GOULAG_SETTINGS_H

#include <SFML/Graphics.hpp>
#include <string>

// Réglages du jeu, accessibles partout avec Settings::...
namespace Settings
{
    const std::string NOMS_PERSOS[4] = {"Chevalier", "Mage", "Archer", "Barbare"};
    const std::string FICHIERS_PERSOS[4] = {"chevalier.png", "mage.png", "archer.png", "barbare.png"};

    extern int luminosite; // de 20 à 100 %
    extern int volume;     // de 0 à 100 %
    extern int perso;      // de 0 à 3

    void changerLuminosite(int ecart);
    void changerVolume(int ecart);

    // Chemin de l'image du perso choisi
    std::string cheminPerso();

    // A appeler juste avant fenetre.display() pour assombrir l'écran
    void appliquerLuminosite(sf::RenderWindow& fenetre);
}

#endif //GOULAG_SETTINGS_H
