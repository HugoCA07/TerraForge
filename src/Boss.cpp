#include "Boss.h"

/**
 * @brief Constructs a Boss object by extending the base Mob class.
 * @param startPos The initial spawning position.
 * @param texture The texture used for the boss's sprite.
 * @param maxHp The maximum health points.
 * @param name The display name of the boss.
 */
Boss::Boss(sf::Vector2f startPos, const sf::Texture& texture, int maxHp, std::string name)
    : Mob(startPos, texture, maxHp) // Initialize base class attributes
    , mName(name)
    , mMaxHp(maxHp)
{
}

/**
 * @brief Handles incoming damage for bosses.
 * Overrides the default Mob behavior to make bosses completely immune to knockback physics.
 * @param amount The damage inflicted on the boss.
 * @param knockbackDir The direction of the applied force (Ignored).
 * @return True if damage was taken, false if currently invulnerable.
 */
bool Boss::takeDamage(int amount, float knockbackDir) {
    // If the boss is already flashing red from a previous hit, ignore the attack
    if (mDamageTimer > 0.0f) return false;

    mHp -= amount;
    mDamageTimer = 0.4f; // Boss will visually flash red for 0.4 seconds

    // --- THE MAGIC OF THE BOSS CLASS ---
    // Unlike a normal Mob, we DO NOT modify mVel.x or mVel.y here.
    // The boss is too heavy to be knocked back, so it will continue its path
    // towards the player unstoppably even while taking damage.

    return true;
}