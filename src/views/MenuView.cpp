#include "MenuView.h"

#include <SFML/Graphics.hpp>
#include <iostream>
#include <string>

#include "core/Game.h"
#include "Dessin.h"
#include "OptionsView.h"
#include "core/Settings.h"

static const std::string BOUTONS[4] = {"Jouer", "Multijoueur", "Options", "Quitter"};

// Zone cliquable du bouton numéro i
static sf::FloatRect zoneBouton(int i)
{
    return sf::FloatRect(470, 250 + i * 76, 340, 56);
}

void creerWindow()
{
    sf::RenderWindow fenetre(sf::VideoMode(1280, 720), "Ognonny Ostrov");
    fenetre.setFramerateLimit(60);

    sf::Font police;
    if (!police.loadFromFile("assets/fonts/LilitaOne-Regular.ttf") &&
        !police.loadFromFile("C:/Windows/Fonts/arialbd.ttf"))
    {
        std::cout << "aucune police trouvee" << std::endl;
        return;
    }

    int selection = 0;
    OptionsView options(police);
    bool optionsOuvert = false;

    while (fenetre.isOpen())
    {
        std::string clic = ""; // nom du bouton cliqué

        sf::Event evenement;
        while (fenetre.pollEvent(evenement))
        {
            if (evenement.type == sf::Event::Closed)
                fenetre.close();

            if (optionsOuvert)
            {
                options.gererEvenement(evenement);
                continue;
            }

            if (evenement.type == sf::Event::MouseMoved)
            {
                for (int i = 0; i < 4; i++)
                    if (zoneBouton(i).contains(evenement.mouseMove.x, evenement.mouseMove.y))
                        selection = i;
            }

            if (evenement.type == sf::Event::MouseButtonPressed &&
                zoneBouton(selection).contains(evenement.mouseButton.x, evenement.mouseButton.y))
                clic = BOUTONS[selection];

            if (evenement.type == sf::Event::KeyPressed)
            {
                if (evenement.key.code == sf::Keyboard::Down)  selection = (selection + 1) % 4;
                if (evenement.key.code == sf::Keyboard::Up)    selection = (selection + 3) % 4;
                if (evenement.key.code == sf::Keyboard::Enter) clic = BOUTONS[selection];
            }
        }

        if (clic == "Jouer")
        {
            fenetre.close();
            Game game;
            game.run();
            return;
        }
        if (clic == "Options") optionsOuvert = true;
        if (clic == "Quitter") fenetre.close();

        if (optionsOuvert)
        {
            if (options.retour)
            {
                options.retour = false;
                optionsOuvert = false;
            }
            options.dessiner(fenetre);
        }
        else
        {
            fond(fenetre);
            texte(fenetre, police, "OGNONNY OSTROV", 90, 640, 150, MUR_CLAIR, 5);

            for (int i = 0; i < 4; i++)
                bouton(fenetre, police, zoneBouton(i), BOUTONS[i], i == selection);

            // Petit pion vert devant le bouton sélectionné
            sf::CircleShape pion(10);
            pion.setFillColor(sf::Color(53, 194, 122));
            pion.setOutlineThickness(4);
            pion.setOutlineColor(sf::Color::White);
            pion.setPosition(432, zoneBouton(selection).top + 18);
            fenetre.draw(pion);
        }

        Settings::appliquerLuminosite(fenetre);
        fenetre.display();
    }
}
