#pragma once
#include "Mob.h"
#include <string>

/**
 * @class Boss
 * @brief Base class for major boss encounters.
 *
 * Bosses inherit from Mob but feature custom traits, such as immunity
 * to knockback and the inclusion of a display name and maximum health
 * for rendering a boss health bar.
 */
class Boss : public Mob {
public:
    /**
     * @brief Constructs a new Boss object.
     * @param startPos The initial spawning position.
     * @param texture The texture used for the boss's sprite.
     * @param maxHp The maximum health points of the boss.
     * @param name The display name of the boss.
     */
    Boss(sf::Vector2f startPos, const sf::Texture& texture, int maxHp, std::string name);

    /**
     * @brief Overrides the default damage response to make the boss immune to knockback.
     * @param amount The damage dealt to the boss.
     * @param knockbackDir The direction of the applied force (ignored by Bosses).
     * @return True if damage was taken, false if currently invulnerable.
     */
    bool takeDamage(int amount, float knockbackDir) override;

    /**
     * @brief Gets the boss's display name.
     * @return The name as a string.
     */
    std::string getName() const { return mName; }

    /**
     * @brief Gets the maximum health of the boss.
     * @return The max HP (used to calculate health bar percentage).
     */
    int getMaxHp() const { return mMaxHp; }

protected:
    std::string mName; // The display name for the boss interface
    int mMaxHp;        // Stored to calculate the percentage for the boss health bar
};