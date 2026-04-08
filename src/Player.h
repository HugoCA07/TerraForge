#pragma once
#include <SFML/Graphics.hpp>
#include "World.h"

/**
 * @class Player
 * @brief Represents the main character controlled by the user.
 *
 * This class handles player input, physics (movement, gravity, collisions),
 * combat mechanics, animation, and interactions with the game world.
 */
class Player {
public:
    Player();

    // --- Main Methods ---
    /**
     * @brief Handles keyboard input for movement and actions.
     * @param isInventoryOpen If true, movement and combat inputs are disabled.
     */
    void handleInput(bool isInventoryOpen = false);

    /**
     * @brief Updates player physics, state, and animations.
     * @param dt Time elapsed since the last frame.
     * @param world Reference to the game world for collision detection.
     */
    void update(sf::Time dt, World& world);

    /**
     * @brief Draws the player and their equipment to the window.
     * @param window The render window.
     * @param lightColor The ambient light color affecting the player's sprite.
     */
    void render(sf::RenderWindow& window, sf::Color lightColor);

    // --- Getters and Setters ---
    sf::Vector2f getPosition() const { return mSprite.getPosition(); }
    sf::Vector2f getCenter() const;
    sf::FloatRect getGlobalBounds() const { return mSprite.getGlobalBounds(); }
    void setPosition(sf::Vector2f pos);
    int getHp() const { return mHp; }
    int getMaxHp() const { return mMaxHp; }
    void setHp(int hp) { mHp = hp; }
    void setVelocity(sf::Vector2f vel) { mVelocity = vel; }
    void heal(int amount);
    void setOverweight(bool isHeavy) { mIsOverweight = isHeavy; }
    void setEquippedWeapon(int itemID) { mEquippedWeaponID = itemID; }

    /**
     * @brief Applies damage to the player and triggers knockback.
     * @param amount The amount of damage to take.
     * @param knockbackDir The direction of the knockback force.
     * @return True if damage was taken, false if the player was invulnerable.
     */
    bool takeDamage(int amount, float knockbackDir);

    // --- Combat System ---
    /**
     * @enum CombatState
     * @brief Defines the phases of the player's attack sequence.
     */
    enum class CombatState {
        None,     // Not attacking
        Windup,   // Preparing the attack
        Active,   // The attack can deal damage
        Recovery  // Cooldown after the attack
    };

    CombatState getCombatState() const { return mCombatState; }
    sf::FloatRect getWeaponHitbox() const;
    bool canDealDamage() const { return mCombatState == CombatState::Active && !mHasHitThisSwing; }
    void registerHit() { mHasHitThisSwing = true; } // Prevents hitting multiple enemies with one swing

    /**
     * @brief Sets the textures for the animated armor layers.
     */
    void setArmorAnimTextures(const sf::Texture* head, const sf::Texture* chest, const sf::Texture* legs, const sf::Texture* boots);

private:
    sf::Texture mTexture;
    sf::Sprite mSprite;

    int mHp;
    int mMaxHp;
    float mDamageTimer; // Invulnerability timer after being hit

    // --- Physics Variables ---
    sf::Vector2f mVelocity;
    bool mIsGrounded;
    bool mIsOverweight = false;

    // --- Animation Variables ---
    float mAnimTimer;
    int mCurrentFrame;
    int mNumFrames;
    int mFrameWidth;
    int mFrameHeight;

    /**
     * @enum AnimState
     * @brief Represents the player's current animation pose.
     */
    enum class AnimState {
        Idle,
        Walk,
        Mine // Future use
    };
    AnimState mCurrentState = AnimState::Idle;
    int mCurrentRow = 0; // The row in the spritesheet for the current animation

    // --- Physics Constants ---
    const float GRAVITY = 980.0f;
    const float JUMP_FORCE = -500.0f;

    // --- Fluid Movement Parameters ---
    float mAcceleration = 2000.0f;
    float mFriction = 1500.0f;
    float mMaxSpeed = 250.0f;
    float mMoveDirection = 0.0f; // -1 (Left), 1 (Right), 0 (Still)

    // --- Combat State ---
    CombatState mCombatState = CombatState::None;
    float mCombatTimer = 0.0f;

    // --- Weapon and Armor Visuals ---
    sf::Texture mWeaponTexture;
    sf::Sprite mWeaponSprite;
    int mEquippedWeaponID = 0;
    bool mHasHitThisSwing = false; // Safety flag for single-hit-per-swing logic
    sf::Sprite mArmorAnimSprites[4]; // Sprites for head, chest, legs, boots
    const sf::Texture* mArmorAnimTextures[4] = {nullptr, nullptr, nullptr, nullptr};
};