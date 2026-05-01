#pragma once
#include <SFML/Graphics.hpp>

#include "World.h"


/**
 * @class Mob
 * @brief Base class for all mobile entities (enemies) in the game.
 *
 * This class provides the fundamental attributes and behaviors for any character
 * that can move, take damage, and interact with the game world. It is designed
 * to be inherited by specific mob types like Dodo, Troodon, etc.
 */
class Mob {
public:
    /**
     * @brief Constructs a new Mob object.
     * @param startPos The initial position of the mob in the world.
     * @param texture The texture used to render the mob.
     * @param maxHp The maximum health points for the mob.
     */
    Mob(sf::Vector2f startPos, const sf::Texture& texture, int maxHp);

    /**
     * @brief Virtual destructor. Ensures proper cleanup for derived classes.
     */
    virtual ~Mob() = default;

    /**
     * @brief Pure virtual update method.
     *
     * This method must be implemented by all derived classes. It defines the
     * mob's behavior, AI, and physics for each frame.
     * @param dt The time elapsed since the last frame.
     * @param playerPos The current position of the player.
     * @param world The game world for collision detection.
     * @param lightLevel The current environmental light level (0.0 to 1.0).
     */
    virtual void update(sf::Time dt, sf::Vector2f playerPos, World& world, float lightLevel) = 0;

    /**
     * @brief Renders the mob to the specified window.
     * @param window The SFML RenderWindow to draw on.
     * @param ambientLight The ambient light color affecting the mob's sprite.
     */
    void render(sf::RenderWindow& window, sf::Color ambientLight);

    /**
     * @brief Applies damage to the mob and triggers knockback.
     *
     * This can be overridden by derived classes to provide custom damage responses
     * (e.g., becoming aggressive, ignoring knockback).
     * @param amount The amount of damage to inflict.
     * @param knockbackDir The horizontal direction of the knockback force.
     * @return True if the damage was applied, false if the mob is currently invulnerable.
     */
    virtual bool takeDamage(int amount, float knockbackDir);

    /**
     * @brief Checks if the mob's health has reached zero or below.
     * @return True if the mob is dead, false otherwise.
     */
    bool isDead() const { return mHp <= 0; }

    /**
     * @brief Gets the mob's collision bounding box.
     *
     * Can be overridden by derived classes to provide a more accurate hitbox
     * that differs from the sprite's default bounding box.
     * @return The FloatRect representing the mob's hitbox.
     */
    virtual sf::FloatRect getBounds() const { return mSprite.getGlobalBounds(); }

    /**
     * @brief Gets the current position of the mob.
     * @return The mob's position as a 2D vector.
     */
    sf::Vector2f getPosition() const { return mPos; }

    /**
     * @brief Gets the attack damage of the mob.
     * @return The amount of damage the mob inflicts on attack.
     */
    int getDamage() const { return mAttackDamage; }

protected: // Accessible by derived classes (Dodo, Troodon, etc.)
    sf::Vector2f mPos;      // Current position in the world
    sf::Vector2f mVel;      // Current velocity (pixels/second)
    sf::Sprite mSprite;     // The visual representation of the mob

    bool mFacingRight;      // Direction the mob is facing
    int mHp;                // Current health points
    float mDamageTimer;     // Timer for invulnerability frames after being hit

    int mAttackDamage;      // Damage dealt by this mob's attacks
};