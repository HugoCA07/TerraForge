#pragma once
#include <SFML/Graphics.hpp>
#include "World.h"

/**
 * @class Projectile
 * @brief Represents an entity shot from a weapon (e.g., an arrow from a bow).
 *
 * Handles projectile physics (gravity over time), rotation based on velocity,
 * collision with the world, and lifespan to avoid infinite travel.
 */
class Projectile {
public:
    /**
     * @brief Constructs a new Projectile object.
     * @param startPos The initial spawning position (e.g., player's location).
     * @param velocity The initial directional velocity.
     * @param texture The texture used for rendering the projectile.
     */
    Projectile(sf::Vector2f startPos, sf::Vector2f velocity, const sf::Texture& texture);
    
    /**
     * @brief Updates the projectile's physics and orientation.
     * @param dt Time elapsed since the last frame.
     * @param world Reference to the game world for collision detection against blocks.
     */
    void update(sf::Time dt, World& world);
    
    /**
     * @brief Renders the projectile to the window.
     * @param window The render window.
     * @param lightColor The current ambient light color.
     */
    void render(sf::RenderWindow& window, sf::Color lightColor);

    /**
     * @brief Checks if the projectile is dead (hit something or expired).
     * @return True if dead, false otherwise.
     */
    bool isDead() const { return mIsDead; }

    /**
     * @brief Forces the projectile to die (e.g., upon hitting a monster).
     */
    void kill() { mIsDead = true; }

    /**
     * @brief Gets the collision bounding box.
     * @return A FloatRect representing the projectile's physical space.
     */
    sf::FloatRect getBounds() const { return mSprite.getGlobalBounds(); }

    /**
     * @brief Gets the damage dealt by the projectile.
     * @return The damage value (e.g., 30 for an arrow).
     */
    int getDamage() const { return 30; }

    /**
     * @brief Gets the current velocity of the projectile.
     * @return The 2D velocity vector.
     */
    sf::Vector2f getVelocity() const { return mVel; }

private:
    sf::Vector2f mPos;  // Current position in the world
    sf::Vector2f mVel;  // Current velocity (changes with gravity)
    sf::Sprite mSprite; // Visual representation
    
    bool mIsDead;       // Marks for deletion
    float mLifeTime;    // Timer to destroy if it flies out of bounds
};