#include "Projectile.h"
#include <cmath>

/**
 * @brief Constructs a Projectile object.
 * Configures the starting velocity, position, and texture origin.
 * @param startPos The exact position where the projectile spawns.
 * @param velocity The directional force (speed) of the projectile.
 * @param texture The sprite texture.
 */
Projectile::Projectile(sf::Vector2f startPos, sf::Vector2f velocity, const sf::Texture& texture)
    : mPos(startPos), mVel(velocity), mIsDead(false), mLifeTime(0.0f)
{
    mSprite.setTexture(texture);
    // Origin centered for perfect rotation in the air
    mSprite.setOrigin(texture.getSize().x / 2.0f, texture.getSize().y / 2.0f);
    mSprite.setPosition(mPos);
    
    // Scale it down to match the world visually
    mSprite.setScale(0.8f, 0.8f); 
}

/**
 * @brief Updates projectile physics, applies gravity, calculates rotation, and checks collisions.
 * @param dt Time elapsed since the last frame.
 * @param world Reference to the game world for collision checks.
 */
void Projectile::update(sf::Time dt, World& world) {
    if (mIsDead) return;

    // 1. Maximum lifespan (Despawns if shot into the sky indefinitely)
    mLifeTime += dt.asSeconds();
    if (mLifeTime > 4.0f) { 
        mIsDead = true;
        return;
    }

    // 2. Ballistic Gravity
    // Uses 400.0f (less than the player's 980.0f) so arrows fly relatively straight and fast
    mVel.y += 400.0f * dt.asSeconds();

    // 3. Move the physical position
    mPos += mVel * dt.asSeconds();
    mSprite.setPosition(mPos);

    // 4. Automatic Rotation
    // atan2 calculates the exact angle in radians based on the X and Y velocity vectors
    float angle = std::atan2(mVel.y, mVel.x) * 180.0f / 3.14159265f; // Convert to degrees
    mSprite.setRotation(angle);

    // 5. Collision with the world (Walls/Ground)
    int gridX = static_cast<int>(std::floor(mPos.x / world.getTileSize()));
    int gridY = static_cast<int>(std::floor(mPos.y / world.getTileSize()));

    // If the arrow hits a solid block
    if (World::isSolid(world.getBlock(gridX, gridY))) {
        // Drop the arrow item (ID 36) back into the world so it can be picked up again
        world.spawnItem(gridX, gridY, 36); 
        mIsDead = true; // Destroy the physical projectile
    }
}

/**
 * @brief Renders the projectile to the window.
 * @param window The render window.
 * @param lightColor The ambient lighting at its current position.
 */
void Projectile::render(sf::RenderWindow& window, sf::Color lightColor) {
    mSprite.setColor(lightColor);
    window.draw(mSprite);
}