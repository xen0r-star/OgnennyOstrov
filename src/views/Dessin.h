#ifndef GOULAG_DESSIN_H
#define GOULAG_DESSIN_H

#include <SFML/Graphics.hpp>
#include <string>

// Couleurs du jeu
const sf::Color HERBE(200, 213, 203);
const sf::Color LIGNE_HERBE(180, 195, 184);
const sf::Color PIERRE(90, 103, 111);
const sf::Color LIGNE_PIERRE(75, 87, 94);
const sf::Color MUR(163, 139, 62);
const sf::Color MUR_CLAIR(194, 168, 88);
const sf::Color ENCRE(42, 38, 32);
const sf::Color BOIS(122, 82, 66);
const sf::Color BOIS_CLAIR(148, 102, 84);
const sf::Color BOIS_FONCE(61, 42, 34);
const sf::Color PARCHEMIN(243, 236, 217);

inline void rectangle(sf::RenderWindow& fenetre, float x, float y, float largeur, float hauteur, sf::Color couleur)
{
    sf::RectangleShape forme(sf::Vector2f(largeur, hauteur));
    forme.setPosition(x, y);
    forme.setFillColor(couleur);
    fenetre.draw(forme);
}

// Texte centré sur (x, y)
inline void texte(sf::RenderWindow& fenetre, const sf::Font& police, const std::string& message,
                  unsigned int taille, float x, float y, sf::Color couleur = PARCHEMIN, float contour = 0)
{
    sf::Text t(sf::String::fromUtf8(message.begin(), message.end()), police, taille);
    t.setFillColor(couleur);
    t.setOutlineColor(ENCRE);
    t.setOutlineThickness(contour);

    sf::FloatRect bords = t.getLocalBounds();
    t.setOrigin(bords.left + bords.width / 2, bords.top + bords.height / 2);
    t.setPosition(x, y);
    fenetre.draw(t);
}

// Bouton en bois avec une ombre et un texte
inline void bouton(sf::RenderWindow& fenetre, const sf::Font& police, sf::FloatRect zone,
                   const std::string& message, bool survole = false)
{
    rectangle(fenetre, zone.left, zone.top + 5, zone.width, zone.height, BOIS_FONCE);
    rectangle(fenetre, zone.left, zone.top, zone.width, zone.height, ENCRE);
    rectangle(fenetre, zone.left + 3, zone.top + 3, zone.width - 6, zone.height - 6, survole ? BOIS_CLAIR : BOIS);
    texte(fenetre, police, message, 30, zone.left + zone.width / 2, zone.top + zone.height / 2);
}

inline void grille(sf::RenderWindow& fenetre, float x, float y, float largeur, float hauteur, sf::Color couleur)
{
    for (float i = 0; i < largeur; i += 44)
        rectangle(fenetre, x + i, y, 2, hauteur, couleur);
    for (float j = 0; j < hauteur; j += 44)
        rectangle(fenetre, x, y + j, largeur, 2, couleur);
}

// Fond commun aux menus : herbe + salle en pierre entourée d'un mur
inline void fond(sf::RenderWindow& fenetre)
{
    fenetre.clear(HERBE);
    grille(fenetre, 0, 0, 1280, 720, LIGNE_HERBE);

    rectangle(fenetre, 388, 38, 504, 644, ENCRE);
    rectangle(fenetre, 391, 41, 498, 638, MUR);
    rectangle(fenetre, 407, 57, 466, 606, ENCRE);
    rectangle(fenetre, 410, 60, 460, 600, PIERRE);
    grille(fenetre, 410, 60, 460, 600, LIGNE_PIERRE);
}

#endif //GOULAG_DESSIN_H
