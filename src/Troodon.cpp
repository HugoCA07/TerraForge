#include "Troodon.h"
#include <cmath>

// The Troodon has 40 health points
Troodon::Troodon(sf::Vector2f startPos, const sf::Texture& texture)
    : Mob(startPos, texture, 40)
{
    mAttackDamage = 40; // <--- ADD THIS!
}

/**
 * Updates the Troodon's state.
 * Handles AI behavior (hunting at night, fleeing during the day), movement, gravity, and collisions.
 * @param dt Delta time (time elapsed since last frame).
 * @param playerPos The player's current position.
 * @param world Reference to the game world for collision detection.
 * @param lightLevel The current light level (affects behavior).
 */
void Troodon::update(sf::Time dt, sf::Vector2f playerPos, World& world, float lightLevel) {

    float tileSize = world.getTileSize();

    // 1. Damage timer
    if (mDamageTimer > 0.0f) mDamageTimer -= dt.asSeconds();

    // -----------------------------------------------------
    // 1. AI (TROODON BRAIN) - DAY AND NIGHT!
    // -----------------------------------------------------
    float distX = playerPos.x - mPos.x;
    float distY = playerPos.y - mPos.y;
    float distTotal = std::sqrt(distX*distX + distY*distY);

    if (mVel.y == 0.0f) { // If touching the ground

        // IT'S NIGHT (Low light) -> Relentless hunt!
        if (lightLevel < 0.6f && distTotal < 600.0f) {
            if (distX > 0) {
                mVel.x = 180.0f; // Runs towards you
                mFacingRight = true;
            } else {
                mVel.x = -180.0f;
                mFacingRight = false;
            }
        }
        // IT'S DAY (High light) -> Runs away terrified if you get close!
        else if (lightLevel >= 0.6f && distTotal < 400.0f) {
            if (distX > 0) {
                mVel.x = -150.0f; // Runs in the opposite direction
                mFacingRight = false;
            } else {
                mVel.x = 150.0f;
                mFacingRight = true;
            }
        }
        // If you are far away or nothing happens, it stays still
        else {
            mVel.x *= 0.8f;
        }
    }

    // -----------------------------------------------------
    // 2. HORIZONTAL MOVEMENT AND COLLISIONS
    // -----------------------------------------------------
    float dx = mVel.x * dt.asSeconds();
    mPos.x += dx;
    mSprite.setPosition(mPos);

    auto checkCollision = [&](sf::FloatRect rect) -> bool {
        int left = static_cast<int>(std::floor(rect.left / tileSize));
        int right = static_cast<int>(std::floor((rect.left + rect.width) / tileSize));
        int top = static_cast<int>(std::floor(rect.top / tileSize));
        int bottom = static_cast<int>(std::floor((rect.top + rect.height) / tileSize));

        for (int x = left; x <= right; ++x) {
            for (int y = top; y <= bottom; ++y) {
                if (world.isSolid(world.getBlock(x, y))) return true;
            }
        }
        return false;
    };

    sf::FloatRect bounds = mSprite.getGlobalBounds();
    bounds.left += 2.0f; bounds.width -= 4.0f; bounds.top += 2.0f; bounds.height -= 4.0f;

    if (checkCollision(bounds)) {
        mPos.x -= dx;
        mSprite.setPosition(mPos);

        if (mVel.y == 0.0f) {
            mVel.y = -350.0f; // Jump the wall!
        }
    }

    // -----------------------------------------------------
    // 3. GRAVITY AND VERTICAL MOVEMENT
    // -----------------------------------------------------
    mVel.y += 900.0f * dt.asSeconds();
    float dy = mVel.y * dt.asSeconds();
    mPos.y += dy;
    mSprite.setPosition(mPos);

    bounds = mSprite.getGlobalBounds();
    bounds.left += 2.0f; bounds.width -= 4.0f;

    if (checkCollision(bounds)) {
        if (mVel.y > 0.0f) { // Falling
            int blockY = static_cast<int>(std::floor((bounds.top + bounds.height) / tileSize));
            float newY = blockY * tileSize;

            if (std::abs(mPos.y - newY) < tileSize) {
                mPos.y = newY;
            } else {
                mPos.y -= dy;
            }
            mVel.y = 0.0f;
        }
        else if (mVel.y < 0.0f) { // Headbutt with the ceiling
            mPos.y -= dy;
            mVel.y = 0.0f;
        }
    }

    // -----------------------------------------------------
    // 4. UPDATE VISUALS
    // -----------------------------------------------------
    mSprite.setPosition(mPos);
    if (mFacingRight) mSprite.setScale(1.0f, 1.0f);
    else mSprite.setScale(-1.0f, 1.0f);
}