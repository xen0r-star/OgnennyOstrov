#ifndef GOULAG_OPTIONSVIEW_H
#define GOULAG_OPTIONSVIEW_H

#include <SFML/Graphics.hpp>

class OptionsView {
public:
    explicit OptionsView(const sf::Font& police);

    void gererEvenement(const sf::Event& evenement);
    void dessiner(sf::RenderWindow& fenetre);

    bool retour = false; // passe à true quand on clique sur "Retour"

private:
    const sf::Font& police;
    sf::Texture textures[4];
};

#endif //GOULAG_OPTIONSVIEW_H
