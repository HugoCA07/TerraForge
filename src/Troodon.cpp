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
    // 1. Damage timer (Aturdimiento)
    if (mDamageTimer > 0.0f) {
        mDamageTimer -= dt.asSeconds();

        // Mientras están aturdidos (volando hacia atrás), aplicamos fricción
        // para que no resbalen por el suelo infinitamente como si fuera hielo.
        mVel.x *= 0.95f;
    }
    else {
        // -----------------------------------------------------
        // 1. AI (TROODON BRAIN) - SENSORS AND PARKOUR
        // -----------------------------------------------------
        float distX = playerPos.x - mPos.x;
        float distY = playerPos.y - mPos.y;
        float distTotal = std::sqrt(distX*distX + distY*distY);

        // --- EL MOVIMIENTO AHORA ES CONSTANTE (Tierra y Aire) ---
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

        // --- LOS SENSORES DE SALTO SOLO FUNCIONAN EN EL SUELO ---
        if (mVel.y == 0.0f && std::abs(mVel.x) > 10.0f) {
            int dirX = mFacingRight ? 1 : -1;

            sf::FloatRect bounds = mSprite.getGlobalBounds();

            float centerX = bounds.left + (bounds.width / 2.0f);
            float bottomY = bounds.top + bounds.height;

            int nextGridX = static_cast<int>(std::floor((centerX + (dirX * tileSize)) / tileSize));
            int footGridY = static_cast<int>(std::floor((bottomY - 5.0f) / tileSize));
            int pitGridY  = static_cast<int>(std::floor((bottomY + 5.0f) / tileSize));

            bool wallAhead = world.isSolid(world.getBlock(nextGridX, footGridY));

            // --- ¡EL FIX! Escáner de profundidad ---
            // Un barranco real requiere al menos 3 bloques seguidos de caída libre
            bool pitAhead = !world.isSolid(world.getBlock(nextGridX, pitGridY)) &&
                            !world.isSolid(world.getBlock(nextGridX, pitGridY + 1)) &&
                            !world.isSolid(world.getBlock(nextGridX, pitGridY + 2));

            if (pitAhead || wallAhead) {
                // Salto de Parkour para superar muros o cruzar barrancos reales
                mVel.y = -350.0f;
            }
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

    // --- CORRECCIÓN HORIZONTAL ---
    if (checkCollision(bounds)) {
        mPos.x -= dx;
        mSprite.setPosition(mPos);
        mVel.x = 0.0f;
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

    // --- CORRECCIÓN VERTICAL ---
    if (checkCollision(bounds)) {
        if (mVel.y > 0.0f) { // Cayendo
            int blockY = static_cast<int>(std::floor((bounds.top + bounds.height) / tileSize));
            float newY = blockY * tileSize - bounds.height;

            if (std::abs(mPos.y - newY) < tileSize) {
                mPos.y = newY;
            } else {
                mPos.y -= dy;
            }
            mVel.y = 0.0f;
        } else if (mVel.y < 0.0f) { // Techo
            mPos.y -= dy;
            mVel.y = 0.0f;
        }
        mSprite.setPosition(mPos);
    }

    // -----------------------------------------------------
    // 4. UPDATE VISUALS
    // -----------------------------------------------------
    mSprite.setPosition(mPos);

    // Corregimos el sprite centrándolo para que al girar no tiemble
    mSprite.setOrigin(mSprite.getLocalBounds().width / 2.0f, 0.0f);

    if (mFacingRight) mSprite.setScale(1.0f, 1.0f);
    else mSprite.setScale(-1.0f, 1.0f);
}