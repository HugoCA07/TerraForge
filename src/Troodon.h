#pragma once
#include "Mob.h"
#include "World.h"

/**
 * @class Troodon
 * @brief An aggressive, nocturnal dinosaur mob.
 *
 * The Troodon is a fast, hostile enemy that hunts the player.
 * It features sprinting, jumping, and a quick dash-attack.
 */
class Troodon : public Mob {
public:
    /**
     * @enum TroodonAnim
     * @brief Represents the animation states of the Troodon.
     */
    enum class TroodonAnim { Idle = 0, Walk = 1, Jump = 2, Attack = 3 };

    /**
     * @brief Constructs a new Troodon object.
     * @param startPos The initial position in the world.
     * @param texture The texture used for rendering the sprite.
     */
    Troodon(sf::Vector2f startPos, const sf::Texture& texture);

    /**
     * @brief Updates the Troodon's logic, AI, physics, and animation.
     * @param dt Time elapsed since the last frame.
     * @param playerPos The current position of the player (for tracking).
     * @param world Reference to the game world for collision detection.
     * @param lightLevel The current ambient light level.
     */
    void update(sf::Time dt, sf::Vector2f playerPos, World& world, float lightLevel) override;

    /**
     * @brief Gets the collision bounding box for the Troodon.
     * @return A customized FloatRect representing the physical hitbox.
     */
    sf::FloatRect getBounds() const override;

private:
    TroodonAnim mCurrentAnim;   // Current animation state
    float mAnimTimer;           // Timer to control frame switching
    int mCurrentFrame;          // Current frame index of the animation

    // Combat attributes
    bool mIsAttacking;          // Is the Troodon currently performing an attack?
    float mAttackCooldown;      // Time remaining before it can attack again
    float mAttackDuration;      // How long the current attack lasts
};