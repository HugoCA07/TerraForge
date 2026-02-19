#pragma once
#include <SFML/Graphics.hpp>
#include "World.h"

class Player {

public:
    Player(); // Constructor

    // Main methods
    void handleInput(); // Handles keyboard input for movement

    // Updates player physics and state, checking for collisions with the world
    void update(sf::Time dt, World& world);
    void render(sf::RenderWindow& window, sf::Color lightColor); // Draws the player to the window

    // Position and bounds getters
    sf::Vector2f getPosition() const { return mSprite.getPosition(); }
    sf::Vector2f getCenter() const;
    sf::FloatRect getGlobalBounds() const { return mSprite.getGlobalBounds(); }
    void setPosition(sf::Vector2f pos);
    int getHp() const { return mHp; }
    int getMaxHp() const { return mMaxHp; }
    void heal(int amount);

private:
    sf::Texture mTexture;
    sf::Sprite mSprite;

    int mHp;
    int mMaxHp;

    // --- PHYSICS VARIABLES ---
    sf::Vector2f mVelocity; // Current speed in X and Y directions
    bool mIsGrounded;       // Is the player standing on a solid block?

    float mAnimTimer;
    int mCurrentFrame;
    int mNumFrames;
    int mFrameWidth;
    int mFrameHeight;

    // --- CONSTANTS (Physics Rules) ---
    const float MOVEMENT_SPEED = 300.0f; // Pixels per second horizontal
    const float GRAVITY = 980.0f;        // Downward force (pixels/s^2)
    const float JUMP_FORCE = -500.0f;    // Upward force (Negative Y goes UP)
};