#pragma once
#include <SFML/Graphics.hpp>
#include "World.h"

class Player {

public:
    Player(); // Constructor

    // Main methods
    void handleInput(bool isInventoryOpen = false); // Handles keyboard input for movement

    // Updates player physics and state, checking for collisions with the world
    void update(sf::Time dt, World& world);
    void render(sf::RenderWindow& window, sf::Color lightColor); // Draws the player to the window

    // Position and bounds getters
    sf::Vector2f getPosition() const { return mSprite.getPosition(); }
    sf::Vector2f getCenter() const;
    // --- NEW: TAKE DAMAGE ---
    bool takeDamage(int amount, float knockbackDir);
    sf::FloatRect getGlobalBounds() const { return mSprite.getGlobalBounds(); }
    void setPosition(sf::Vector2f pos);
    int getHp() const { return mHp; }
    int getMaxHp() const { return mMaxHp; }
    void heal(int amount);
    void setOverweight(bool isHeavy) { mIsOverweight = isHeavy; }
    void setEquippedWeapon(int itemID) { mEquippedWeaponID = itemID; }
    // --- COMBAT ---
    sf::FloatRect getWeaponHitbox() const;
    bool canDealDamage() const { return mCombatState == CombatState::Active && !mHasHitThisSwing; }
    void registerHit() { mHasHitThisSwing = true; } // We will call this when we hit an enemy

    // --- SOULS-LIKE COMBAT SYSTEM ---
    enum class CombatState {
        None,
        Windup,
        Active,
        Recovery
    };
    CombatState getCombatState() const { return mCombatState; }

    // --- USEFUL SETTERS
    void setHp(int hp) { mHp = hp; }
    void setVelocity(sf::Vector2f vel) { mVelocity = vel; }

private:
    sf::Texture mTexture;
    sf::Sprite mSprite;

    int mHp;
    int mMaxHp;
    float mDamageTimer; // Invulnerability timer

    // --- PHYSICS VARIABLES ---
    sf::Vector2f mVelocity; // Current speed in X and Y directions
    bool mIsGrounded;       // Is the player standing on a solid block?
    bool mIsOverweight = false;

    float mAnimTimer;
    int mCurrentFrame;
    int mNumFrames;
    int mFrameWidth;
    int mFrameHeight;

    // --- CONSTANTS (Physics Rules) ---
    const float GRAVITY = 980.0f;        // Downward force (pixels/s^2)
    const float JUMP_FORCE = -500.0f;    // Upward force (Negative Y goes UP)

    // --- ANIMATION SYSTEM ---
    enum class AnimState {
        Idle,   // Standing still
        Walk,   // Walking
        Mine    // Mining (for the future)
    };
    AnimState mCurrentState = AnimState::Idle;
    int mCurrentRow = 0; // The row of the image we are reading

    // --- FLUID MOVEMENT ---
    float mAcceleration = 2000.0f; // How fast it picks up speed
    float mFriction = 1500.0f;     // How fast it brakes when releasing the key
    float mMaxSpeed = 250.0f;      // Max speed when running
    float mMoveDirection = 0.0f;   // -1 (Left), 1 (Right), 0 (Still)

    CombatState mCombatState = CombatState::None;
    float mCombatTimer = 0.0f;

    // --- WEAPON / VISUAL COMBAT ---
    sf::Texture mWeaponTexture;
    sf::Sprite mWeaponSprite;
    int mEquippedWeaponID = 0; // Will store the ID of the weapon equipped in hand
    bool mHasHitThisSwing = false; // Safety lock to avoid hitting 60 times per second
};