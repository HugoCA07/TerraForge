#pragma once
#include "Mob.h"
#include "World.h"

/**
 * @class Dodo
 * @brief A neutral mob that wanders around but attacks if provoked.
 *
 * The Dodo is peaceful by default. If it takes damage, it enters an aggressive state ("aggro")
 * and will chase and attack the player.
 */
class Dodo : public Mob {
public:
    /**
     * @enum DodoAnim
     * @brief Represents the animation states of the Dodo.
     */
    enum class DodoAnim { Idle = 0, Walk = 1, Jump = 2, Attack = 3 };

    /**
     * @brief Constructs a new Dodo object.
     * @param startPos The initial spawning position.
     * @param texture The sprite texture to use.
     */
    Dodo(sf::Vector2f startPos, const sf::Texture& texture);

    /**
     * @brief Updates the Dodo's AI (Wandering or Aggro), physics, and animation.
     * @param dt Time elapsed since the last frame.
     * @param playerPos The player's current position (used when aggro).
     * @param world Reference to the game world for collision detection.
     * @param lightLevel The current ambient light level for coloring.
     */
    void update(sf::Time dt, sf::Vector2f playerPos, World& world, float lightLevel) override;

    /**
     * @brief Overrides the default damage response to trigger the aggressive state.
     * @param amount The damage dealt to the Dodo.
     * @param knockbackDir The direction of the force applied.
     * @return True if damage was successfully taken, false otherwise.
     */
    bool takeDamage(int amount, float knockbackDir) override;

    /**
     * @brief Gets a custom, tighter collision bounding box for the Dodo.
     * @return The custom FloatRect hitbox.
     */
    sf::FloatRect getBounds() const override;

private:
    DodoAnim mCurrentAnim;  // Current animation state
    float mAnimTimer;       // Timer to control frame switching
    int mCurrentFrame;      // Current frame index of the animation

    // Peaceful / Aggressive AI
    bool mIsAggro;          // Has the Dodo been provoked?
    float mWanderTimer;     // Time before changing wander direction
    int mWanderDir;         // -1 (Left), 0 (Still), 1 (Right)

    // Combat
    bool mIsAttacking;      // Is currently in the middle of an attack animation
    float mAttackCooldown;  // Time before it can attack again
    float mAttackDuration;  // How long the current attack lasts
};