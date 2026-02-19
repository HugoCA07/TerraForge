#pragma once
#include <SFML/Graphics.hpp>
#include "World.h"

class Dodo {
public:
    Dodo(sf::Vector2f startPos, const sf::Texture& texture);

    void update(sf::Time dt, sf::Vector2f playerPos, World& world);
    void render(sf::RenderWindow& window, sf::Color ambientLight);

    sf::Vector2f getPosition() const { return mPos; }
    sf::FloatRect getBounds() const { return mSprite.getGlobalBounds(); }
    bool takeDamage(int amount, float knockbackDir);
    bool isDead() const { return mHp <= 0; }

private:
    sf::Vector2f mPos;
    sf::Vector2f mVel;
    sf::Sprite mSprite;

    bool mFacingRight; // Para saber hacia dónde mirar
    int mHp;             // Puntos de vida
    float mDamageTimer;  // Tiempo de invulnerabilidad tras un golpe
};