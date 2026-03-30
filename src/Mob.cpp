#include "Mob.h"

/**
 * Constructor for the Mob class.
 * Initializes the mob's position, texture, health, and other base attributes.
 * @param startPos The initial position of the mob.
 * @param texture The texture to be used for the mob's sprite.
 * @param maxHp The maximum health points of the mob.
 */
Mob::Mob(sf::Vector2f startPos, const sf::Texture& texture, int maxHp)
    : mPos(startPos)
    , mVel(0.f, 0.f)
    , mFacingRight(true)
    , mHp(maxHp)
    , mDamageTimer(0.f)
{
    mSprite.setTexture(texture);
    mSprite.setOrigin(texture.getSize().x / 2.0f, texture.getSize().y);
}

/**
 * Renders the mob to the window.
 * Applies a red tint if the mob has recently taken damage.
 * @param window The render window.
 * @param ambientLight The current ambient light color (for day/night cycle).
 */
void Mob::render(sf::RenderWindow& window, sf::Color ambientLight) {
    if (mDamageTimer > 0.0f) {
        mSprite.setColor(sf::Color::Red);
    } else {
        mSprite.setColor(ambientLight);
    }
    window.draw(mSprite);
}

/**
 * Applies damage to the mob.
 * Handles invulnerability frames and knockback.
 * @param amount The amount of damage to take.
 * @param knockbackDir The direction of the knockback (-1.0f for left, 1.0f for right).
 * @return True if damage was taken, false if the mob was invulnerable.
 */
bool Mob::takeDamage(int amount, float knockbackDir) {
    if (mDamageTimer > 0.0f) return false; // Ya está invulnerable/aturdido

    mHp -= amount;
    mDamageTimer = 0.4f; // Estará aturdido durante 0.4 segundos

    // --- ¡EL EMPUJÓN (KNOCKBACK)! ---
    mVel.y = -200.0f; // Pequeño salto hacia arriba
    mVel.x = knockbackDir * 350.0f; // Fuerza bruta hacia atrás

    return true;
}