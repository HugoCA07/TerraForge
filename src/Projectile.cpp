#include "Projectile.h"
#include <cmath>

Projectile::Projectile(sf::Vector2f startPos, sf::Vector2f velocity, const sf::Texture& texture)
    : mPos(startPos), mVel(velocity), mIsDead(false), mLifeTime(0.0f)
{
    mSprite.setTexture(texture);
    // El origen de la flecha está en el centro para rotar perfectamente
    mSprite.setOrigin(texture.getSize().x / 2.0f, texture.getSize().y / 2.0f);
    mSprite.setPosition(mPos);
    
    // Si tu imagen de la flecha apunta hacia arriba, pon 90.0f. 
    // Si apunta a la derecha en el PNG original, déjalo así.
    mSprite.setScale(0.8f, 0.8f); 
}

void Projectile::update(sf::Time dt, World& world) {
    if (mIsDead) return;

    // 1. Tiempo de vida máximo (por si la disparas al cielo)
    mLifeTime += dt.asSeconds();
    if (mLifeTime > 4.0f) { 
        mIsDead = true;
        return;
    }

    // 2. Gravedad Balística (400.0f es menos que el jugador, para que vuele recta y rápida)
    mVel.y += 400.0f * dt.asSeconds();

    // 3. Moverse
    mPos += mVel * dt.asSeconds();
    mSprite.setPosition(mPos);

    // 4. Rotación automática: atan2 calcula el ángulo exacto basado en el vector X e Y
    float angle = std::atan2(mVel.y, mVel.x) * 180.0f / 3.14159265f;
    mSprite.setRotation(angle);

    // 5. Colisión con el mundo (Paredes/Suelo)
    int gridX = static_cast<int>(std::floor(mPos.x / world.getTileSize()));
    int gridY = static_cast<int>(std::floor(mPos.y / world.getTileSize()));

    if (World::isSolid(world.getBlock(gridX, gridY))) {
        // Al chocar contra piedra o tierra, soltamos el ítem Flecha (ID 36) para poder recogerla
        world.spawnItem(gridX, gridY, 36); 
        mIsDead = true; // Y destruimos el proyectil físico
    }
}

void Projectile::render(sf::RenderWindow& window, sf::Color lightColor) {
    mSprite.setColor(lightColor);
    window.draw(mSprite);
}