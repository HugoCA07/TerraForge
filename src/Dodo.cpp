#include "Dodo.h"
#include <cmath>

// Call the parent constructor (Mob) passing the Dodo's health (20)
Dodo::Dodo(sf::Vector2f startPos, const sf::Texture& texture)
    : Mob(startPos, texture, 20)
{
    mAttackDamage = 25; // <--- ADD THIS!
    // No need to configure the sprite here anymore, the parent does it
}

/**
 * Updates the Dodo's state.
 * Handles AI behavior (chasing the player), movement, gravity, and collisions.
 * @param dt Delta time (time elapsed since last frame).
 * @param playerPos The player's current position.
 * @param world Reference to the game world for collision detection.
 * @param lightLevel The current light level (affects behavior).
 */
void Dodo::update(sf::Time dt, sf::Vector2f playerPos, World& world, float lightLevel) {
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
        // 1. AI (DODO BRAIN) - SENSORS AND AVOIDANCE
        // -----------------------------------------------------
        float distX = playerPos.x - mPos.x;
        float distY = playerPos.y - mPos.y;
        float distTotal = std::sqrt(distX*distX + distY*distY);

        // --- MOVIMIENTO CONSTANTE ---
        if (distTotal < 400.0f) {
            // Go for the player!
            if (distX > 0) {
                mVel.x = 100.0f;
                mFacingRight = true;
            } else {
                mVel.x = -100.0f;
                mFacingRight = false;
            }
        } else {
            // Stay still
            mVel.x *= 0.8f;
        }

        // --- SENSORES SOLO EN EL SUELO ---
        if (mVel.y == 0.0f && std::abs(mVel.x) > 10.0f) {
            int dirX = mFacingRight ? 1 : -1;

            sf::FloatRect bounds = mSprite.getGlobalBounds();
            float centerX = bounds.left + (bounds.width / 2.0f);
            float bottomY = bounds.top + bounds.height;

            // Miramos la cuadrícula que tenemos justo delante
            int nextGridX = static_cast<int>(std::floor((centerX + (dirX * tileSize)) / tileSize));
            int footGridY = static_cast<int>(std::floor((bottomY - 5.0f) / tileSize));
            int pitGridY  = static_cast<int>(std::floor((bottomY + 5.0f) / tileSize));

            bool wallAhead = world.isSolid(world.getBlock(nextGridX, footGridY));
            bool pitAhead = !world.isSolid(world.getBlock(nextGridX, pitGridY)) &&
                            !world.isSolid(world.getBlock(nextGridX, pitGridY + 1));

            if (pitAhead) {
                // Instinto de supervivencia: frena antes del barranco
                mVel.x = 0.0f;
            } else if (wallAhead) {
                // Pared delante: intenta saltarla
                mVel.y = -280.0f;
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

    // --- ¡AQUÍ ESTÁ LA CORRECCIÓN HORIZONTAL! ---
    if (checkCollision(bounds)) {
        mPos.x -= dx; // Si chocamos, retrocedemos
        mSprite.setPosition(mPos);
        mVel.x = 0.0f; // Y nos detenemos en seco (la IA decidirá qué hacer en el próximo frame)
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

    // --- ¡AQUÍ ESTÁ LA CORRECCIÓN VERTICAL! ---
    if (checkCollision(bounds)) {
        if (mVel.y > 0.0f) { // Cayendo y tocando el suelo
            int blockY = static_cast<int>(std::floor((bounds.top + bounds.height) / tileSize));
            float newY = blockY * tileSize - bounds.height; // Nos posamos encima

            if (std::abs(mPos.y - newY) < tileSize) {
                mPos.y = newY;
            } else {
                mPos.y -= dy; // Fallback
            }
            mVel.y = 0.0f; // Detenemos la gravedad
        } else if (mVel.y < 0.0f) { // Chocando la cabeza contra el techo
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