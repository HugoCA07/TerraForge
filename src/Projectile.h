#pragma once
#include <SFML/Graphics.hpp>
#include "World.h"

class Projectile {
public:
    // Al nacer, le damos su posición, hacia dónde va (velocidad) y su textura
    Projectile(sf::Vector2f startPos, sf::Vector2f velocity, const sf::Texture& texture);
    
    // Actualiza su física cada frame
    void update(sf::Time dt, World& world);
    
    // Se dibuja en pantalla con la luz correcta
    void render(sf::RenderWindow& window, sf::Color lightColor);

    bool isDead() const { return mIsDead; }
    void kill() { mIsDead = true; } // Para destruirla si toca a un monstruo

    sf::FloatRect getBounds() const { return mSprite.getGlobalBounds(); }
    int getDamage() const { return 30; } // ¡Daño de la flecha!
    sf::Vector2f getVelocity() const { return mVel; }

private:
    sf::Vector2f mPos;
    sf::Vector2f mVel;
    sf::Sprite mSprite;
    
    bool mIsDead;
    float mLifeTime; // Para que desaparezca si vuela hacia el infinito
};