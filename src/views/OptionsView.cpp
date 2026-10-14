#include "OptionsView.h"

#include "Dessin.h"
#include "core/Settings.h"

// Position des boutons : (x, y, largeur, hauteur)
static const sf::FloatRect MOINS_LUM(480, 195, 56, 56);
static const sf::FloatRect PLUS_LUM(744, 195, 56, 56);
static const sf::FloatRect MOINS_SON(480, 305, 56, 56);
static const sf::FloatRect PLUS_SON(744, 305, 56, 56);
static const sf::FloatRect PERSO_AVANT(480, 430, 56, 56);
static const sf::FloatRect PERSO_APRES(744, 430, 56, 56);
static const sf::FloatRect RETOUR(540, 570, 200, 56);

OptionsView::OptionsView(const sf::Font& police) : police(police)
{
    for (int i = 0; i < 4; i++)
    {
        textures[i].loadFromFile("assets/characters/" + Settings::FICHIERS_PERSOS[i]);
        textures[i].setSmooth(true);
    }
}

void OptionsView::gererEvenement(const sf::Event& evenement)
{
    if (evenement.type == sf::Event::KeyPressed && evenement.key.code == sf::Keyboard::Escape)
        retour = true;

    if (evenement.type != sf::Event::MouseButtonPressed)
        return;

    float x = (float)evenement.mouseButton.x;
    float y = (float)evenement.mouseButton.y;

    if (MOINS_LUM.contains(x, y))   Settings::changerLuminosite(-10);
    if (PLUS_LUM.contains(x, y))    Settings::changerLuminosite(+10);
    if (MOINS_SON.contains(x, y))   Settings::changerVolume(-10);
    if (PLUS_SON.contains(x, y))    Settings::changerVolume(+10);
    if (PERSO_AVANT.contains(x, y)) Settings::perso = (Settings::perso + 3) % 4;
    if (PERSO_APRES.contains(x, y)) Settings::perso = (Settings::perso + 1) % 4;
    if (RETOUR.contains(x, y))      retour = true;
}

void OptionsView::dessiner(sf::RenderWindow& fenetre)
{
    fond(fenetre);
    texte(fenetre, police, "Options", 46, 640, 105, MUR_CLAIR, 3);

    // Luminosité
    texte(fenetre, police, "Luminosité", 24, 640, 175);
    bouton(fenetre, police, MOINS_LUM, "-");
    bouton(fenetre, police, PLUS_LUM, "+");
    texte(fenetre, police, std::to_string(Settings::luminosite) + " %", 30, 640, 223);

    // Son
    texte(fenetre, police, "Son", 24, 640, 285);
    bouton(fenetre, police, MOINS_SON, "-");
    bouton(fenetre, police, PLUS_SON, "+");
    texte(fenetre, police, std::to_string(Settings::volume) + " %", 30, 640, 333);

    // Personnage
    texte(fenetre, police, "Personnage", 24, 640, 395);
    bouton(fenetre, police, PERSO_AVANT, "<");
    bouton(fenetre, police, PERSO_APRES, ">");

    sf::Texture& image = textures[Settings::perso];
    if (image.getSize().x > 0) // l'image a bien été chargée
    {
        sf::Sprite sprite(image);
        sprite.setOrigin(image.getSize().x / 2.f, image.getSize().y / 2.f);
        sprite.setScale(72.f / image.getSize().x, 72.f / image.getSize().y);
        sprite.setPosition(640, 458);
        fenetre.draw(sprite);
    }

    bouton(fenetre, police, RETOUR, "Retour");
}
