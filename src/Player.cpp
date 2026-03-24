#include "Player.h"
#include <iostream>
#include <cmath>

/**
 * Constructor for the Player class.
 * Initializes the player's attributes, loads the texture, and sets up the sprite.
 */
Player::Player()
    : mVelocity(0.f, 0.f)
    , mIsGrounded(false)
    , mAnimTimer(0.f)
    , mCurrentFrame(0)
    , mNumFrames(4)
    , mMaxHp(100)
    , mHp(50)
    , mDamageTimer(0.0f)
{
    if (!mTexture.loadFromFile("assets/player.png")) {
        std::cerr << "Error: Could not load player.png" << std::endl;
    }
    mTexture.setSmooth(false);
    mSprite.setTexture(mTexture);

    // Calculate exact size by dividing by rows and columns
    int numRows = 4;
    mFrameWidth = mTexture.getSize().x / mNumFrames;
    mFrameHeight = mTexture.getSize().y / numRows;

    mSprite.setTextureRect(sf::IntRect(0, 0, mFrameWidth, mFrameHeight));
    mSprite.setOrigin(mFrameWidth / 2.f, mFrameHeight / 2.f);
    mSprite.setScale(1.25f, 1.25f);
    mSprite.setPosition(100.f, 1000.f);
    mWeaponTexture.setSmooth(false); // <--- Vital for pixel art
    mWeaponSprite.setTexture(mWeaponTexture);

    // We set the pivot at the very bottom and centered (exactly where the stick is held)
    mWeaponSprite.setOrigin(mWeaponTexture.getSize().x / 2.0f, mWeaponTexture.getSize().y);

    // We give it the same scale as your character (1.25f)
    mWeaponSprite.setScale(1.25f, 1.25f);
}

/**
 * Handles player input (keyboard).
 * Updates velocity based on key presses (A, D, Space).
 */
void Player::handleInput(bool isInventoryOpen) { // <--- ADD PARAMETER HERE
    mMoveDirection = 0.0f; // By default we don't move

    // We can only move, jump or attack if we are NOT in the middle of a hit
    if (mCombatState == CombatState::None) {

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::A)) {
            mMoveDirection = -1.0f;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::D)) {
            mMoveDirection = 1.0f;
        }

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Space) && mIsGrounded) {
            float jumpPower = mIsOverweight ? -350.0f : -500.0f;
            mVelocity.y = jumpPower;
            mIsGrounded = false;
        }

        // START ATTACK (Left Click)
        // --- NEW! We only attack if the inventory is CLOSED ---
        if (!isInventoryOpen && sf::Mouse::isButtonPressed(sf::Mouse::Left)) {
            mCombatState = CombatState::Windup;
            mCombatTimer = 0.0f;
            mVelocity.x = 0.0f; // Stop dead when starting to attack
            std::cout << "[COMBAT] Preparing strike..." << std::endl;
        }
    }
}

/**
 * Updates the player's state.
 * Handles movement, physics (gravity), collisions, and animation.
 * @param dt Delta time (time elapsed since last frame).
 * @param world Reference to the game world for collision detection.
 */
void Player::update(sf::Time dt, World& world) {
    float tileSize = world.getTileSize();
    sf::Vector2f originalPos = mSprite.getPosition();
    sf::FloatRect bounds = mSprite.getGlobalBounds();

    float skinW = 4.0f;
    float skinH = 0.5f;

    // ==========================================
    // 1. THINK: COMBAT STATE MACHINE
    // ==========================================
    if (mCombatState != CombatState::None) {
        mCombatTimer += dt.asSeconds();

        if (mCombatState == CombatState::Windup) {
            // Phase 1: Raising the weapon (0.2 seconds)
            if (mCombatTimer >= 0.2f) {
                mCombatState = CombatState::Active;
                mCombatTimer = 0.0f;
                mHasHitThisSwing = false; // <--- NEW! We remove the safety lock when swinging
                std::cout << "[COMBAT] WHAM! Active hit." << std::endl;
            }
        }
        else if (mCombatState == CombatState::Active) {
            if (mCombatTimer >= 0.1f) {
                mCombatState = CombatState::Recovery;
                mCombatTimer = 0.0f;
                std::cout << "[COMBAT] Recovering balance..." << std::endl;
            }
        }
        else if (mCombatState == CombatState::Recovery) {
            if (mCombatTimer >= 0.3f) {
                mCombatState = CombatState::None;
                mCombatTimer = 0.0f;
                std::cout << "[COMBAT] Ready for another action." << std::endl;
            }
        }
    }

    // ==========================================
    // 1.5 WEAPON VISUALS (CHOREOGRAPHY AND REST)
    // ==========================================
    // We check if holding a Pickaxe (21-24) or a Sword (31-34)
    bool isHoldingWeapon = (mEquippedWeaponID >= 21 && mEquippedWeaponID <= 24) ||
                           (mEquippedWeaponID >= 31 && mEquippedWeaponID <= 34);

    if (isHoldingWeapon) {
        // 1. WE REQUEST THE HAND TEXTURE FROM THE WORLD (We no longer use the inventory one!)
        const sf::Texture* tex = world.getHeldTexture(mEquippedWeaponID);

        if (tex) {
            if (mWeaponSprite.getTexture() != tex) {
                mWeaponSprite.setTexture(*tex, true);

                // NEW ORIGIN! We shift the rotation point to the hilt.
                mWeaponSprite.setOrigin(tex->getSize().x * 0.15f, tex->getSize().y * 0.75f);
            }
        }

        float facingDir = (mSprite.getScale().x > 0) ? 1.0f : -1.0f;

        // --- NEW OFFSETS AND SCALE (SMALLER AND CLOSER TO HAND) ---
        float handOffsetX = 22.0f * facingDir; // Fine adjustment (X) to place it in the fist
        float handOffsetY = 5.0f;             // Fine adjustment (Y) to place it in the fist

        mWeaponSprite.setPosition(mSprite.getPosition().x + handOffsetX, mSprite.getPosition().y + handOffsetY);

        // WE REDUCE THE SCALE (1.25f -> 0.85f) so it doesn't look giant
        mWeaponSprite.setScale(0.85f * facingDir, 0.85f);

        // ==========================================
        // 3. PROFESSIONAL CHOREOGRAPHY (EASING MATHS)
        // ==========================================
        if (mCombatState != CombatState::None) {

            if (mCombatState == CombatState::Windup) {
                // PHASE 1: WINDUP (Builds up power slowing at the top)
                float progress = mCombatTimer / 0.2f;
                float ease = 1.0f - std::pow(1.0f - progress, 3.0f); // Ease Out Cubic
                mWeaponSprite.setRotation(-70.0f * ease * facingDir);
            }
            else if (mCombatState == CombatState::Active) {
                // PHASE 2: IMPACT (Accelerates brutally downwards)
                float progress = mCombatTimer / 0.1f;
                float ease = std::pow(progress, 3.0f); // Ease In Cubic

                // Sweeps from -70º to +100º
                float currentRot = -70.0f + (170.0f * ease);

                // IMPACT SPIN (Spin Recoil): In the last milliseconds, the weapon bounces
                float spin = 0.0f;
                if (progress > 0.8f) {
                    float impactProg = (progress - 0.8f) / 0.2f;
                    spin = std::pow(impactProg, 2.0f) * 15.0f; // Small extra 15-degree vibration
                }

                mWeaponSprite.setRotation((currentRot + spin) * facingDir);
            }
            else if (mCombatState == CombatState::Recovery) {
                // PHASE 3: RECOVER BALANCE (Smoothly returns to idle)
                float progress = mCombatTimer / 0.3f;
                float ease = 1.0f - std::pow(1.0f - progress, 2.0f); // Ease Out Quad
                mWeaponSprite.setRotation((100.0f - (100.0f * ease)) * facingDir);
            }
        }
        else {
            // IDLE: Natural relaxed tilt
            mWeaponSprite.setRotation(5.0f * facingDir);
        }
    }

    // ==========================================
    // 2. THINK: FLUID MOVEMENT (INERTIA AND FRICTION)
    // ==========================================
    float maxSpd = mIsOverweight ? 60.0f : mMaxSpeed;

    if (mMoveDirection != 0.0f) {
        // Accelerate towards the pressed direction
        mVelocity.x += mMoveDirection * mAcceleration * dt.asSeconds();
        // Limit to max speed
        if (mVelocity.x > maxSpd) mVelocity.x = maxSpd;
        if (mVelocity.x < -maxSpd) mVelocity.x = -maxSpd;
    } else {
        // Friction: Slow down gradually when keys are released
        if (mVelocity.x > 0.0f) {
            mVelocity.x -= mFriction * dt.asSeconds();
            if (mVelocity.x < 0.0f) mVelocity.x = 0.0f;
        } else if (mVelocity.x < 0.0f) {
            mVelocity.x += mFriction * dt.asSeconds();
            if (mVelocity.x > 0.0f) mVelocity.x = 0.0f;
        }
    }

    // Gravity
    mVelocity.y += GRAVITY * dt.asSeconds();

    // ==========================================
    // 3. MOVE AND COLLIDE: COLLISION HELPER FUNCTION
    // ==========================================
    auto checkCollision = [&](sf::FloatRect rect) -> bool {
        int left   = static_cast<int>(std::floor(rect.left / tileSize));
        int right  = static_cast<int>(std::floor((rect.left + rect.width) / tileSize));
        int top    = static_cast<int>(std::floor(rect.top / tileSize));
        int bottom = static_cast<int>(std::floor((rect.top + rect.height) / tileSize));

        for (int x = left; x <= right; ++x) {
            for (int y = top; y <= bottom; ++y) {
                int blockID = world.getBlock(x, y);
                if (world.isSolid(blockID)) { return true; }
            }
        }
        return false;
    };

    // --- X AXIS (Horizontal) ---
    float dx = mVelocity.x * dt.asSeconds();
    mSprite.move(dx, 0.f);

    sf::FloatRect hitboxX = mSprite.getGlobalBounds();
    hitboxX.left += skinW; hitboxX.width -= (skinW * 2);
    hitboxX.top += skinH; hitboxX.height -= (skinH * 2);

    if (checkCollision(hitboxX)) {
        float stepHeight = tileSize + 2.0f;
        sf::FloatRect stepHitbox = hitboxX;
        stepHitbox.top -= stepHeight;

        if (!checkCollision(stepHitbox)) {
            mSprite.move(0.f, -stepHeight); // Auto-climb
        } else {
            mSprite.setPosition(originalPos.x, mSprite.getPosition().y); // Hit wall
            mVelocity.x = 0.0f; // <--- IMPORTANT: Stop dead on impact
        }
    }

    // --- Y AXIS (Vertical) ---
    float dy = mVelocity.y * dt.asSeconds();
    mSprite.move(0.f, dy);

    sf::FloatRect hitboxY = mSprite.getGlobalBounds();
    hitboxY.left += skinW; hitboxY.width -= (skinW * 2);

    if (checkCollision(hitboxY)) {
        if (mVelocity.y > 0) { // Touching ground
            float playerBottom = hitboxY.top + hitboxY.height;
            int blockY = static_cast<int>(std::floor(playerBottom / tileSize));
            float newY = (blockY * tileSize) - bounds.height;

            if (std::abs(mSprite.getPosition().y - newY) < tileSize) {
                mSprite.setPosition(mSprite.getPosition().x, newY);
            } else {
                mSprite.move(0.f, -dy);
            }
            mVelocity.y = 0;
            mIsGrounded = true;
        }
        else if (mVelocity.y < 0) { // Hitting ceiling (Headbutt)
            mSprite.move(0.f, -dy);
            mVelocity.y = 0;
        }
    } else {
        mIsGrounded = false; // We are in the air
    }

    // ==========================================
    // 4. ANIMATE AND DRAW
    // ==========================================
    // Damage
    if (mDamageTimer > 0.0f) {
        mDamageTimer -= dt.asSeconds();
        mSprite.setColor(sf::Color(255, 100, 100));
    } else {
        mSprite.setColor(sf::Color::White);
    }

    // ==========================================
    // DECIDE ANIMATION (Depends on movement and weapon)
    // ==========================================
    // ==========================================
    // DECIDE ANIMATION (Depends on movement and weapon)
    // ==========================================
    isHoldingWeapon = (mEquippedWeaponID >= 21 && mEquippedWeaponID <= 24) ||
                      (mEquippedWeaponID >= 31 && mEquippedWeaponID <= 34);

    if (std::abs(mVelocity.x) > 10.0f) {
        mCurrentState = AnimState::Walk;
        // If holding a pickaxe/sword, use row 3 (Walk with weapon). Otherwise, row 1 (Normal walk)
        mCurrentRow = isHoldingWeapon ? 3 : 1;
    } else {
        mCurrentState = AnimState::Idle;
        // If holding a pickaxe/sword, use row 2 (Idle with weapon). Otherwise, row 0 (Normal idle)
        mCurrentRow = isHoldingWeapon ? 2 : 0;
    }

    // Player
    mAnimTimer += dt.asSeconds();
    float timePerFrame = (mCurrentState == AnimState::Walk) ? 0.1f : 0.25f;

    if (mAnimTimer >= timePerFrame) {
        mAnimTimer = 0.0f;
        mCurrentFrame++;
        if (mCurrentFrame >= mNumFrames) mCurrentFrame = 0;
    }

    mSprite.setTextureRect(sf::IntRect(
        mCurrentFrame * mFrameWidth,
        mCurrentRow * mFrameHeight,
        mFrameWidth,
        mFrameHeight
    ));

    // ---------------------------------------------------------
    // FLIP (TURN) - Corrected!
    // ---------------------------------------------------------
    float scale = 1.25f;

    // We use mMoveDirection (keyboard intent) instead of mVelocity (physics)
    if (mMoveDirection > 0) {
        mSprite.setScale(scale, scale);
    }
    else if (mMoveDirection < 0) {
        mSprite.setScale(-scale, scale);
    }
}

/**
 * Renders the player to the window.
 * Applies the current light color to the player sprite.
 * @param window The render window.
 * @param lightColor The current light color (for day/night cycle).
 */
void Player::render(sf::RenderWindow& window, sf::Color lightColor) {
    // Draw the player
    mSprite.setColor(lightColor);
    window.draw(mSprite);

    // --- NEW: ALWAYS draw the weapon if equipped ---
    bool isHoldingWeapon = (mEquippedWeaponID >= 21 && mEquippedWeaponID <= 24) ||
                           (mEquippedWeaponID >= 31 && mEquippedWeaponID <= 34);

    if (isHoldingWeapon) {
        // We apply the ambient light color (+ a touch of brightness)
        sf::Color weaponColor = lightColor;
        weaponColor.r = std::min(255, weaponColor.r + 30);
        weaponColor.g = std::min(255, weaponColor.g + 30);
        weaponColor.b = std::min(255, weaponColor.b + 30);

        mWeaponSprite.setColor(weaponColor);
        window.draw(mWeaponSprite);
    }
}

/**
 * Gets the center position of the player.
 * @return The center coordinates of the player sprite.
 */
sf::Vector2f Player::getCenter() const {
    // Calculate the absolute center of the sprite in the game world.
    sf::FloatRect bounds = mSprite.getGlobalBounds();
    return sf::Vector2f(
        bounds.left + bounds.width / 2.0f,
        bounds.top + bounds.height / 2.0f
    );
}

/**
 * Sets the player's position.
 * @param pos The new position.
 */
void Player::setPosition(sf::Vector2f pos) {
    mSprite.setPosition(pos);
}

/**
 * Heals the player by a specified amount.
 * Ensures health does not exceed the maximum.
 * @param amount The amount of health to restore.
 */
void Player::heal(int amount) {
    mHp += amount;
    if (mHp > mMaxHp) {
        mHp = mMaxHp; // Never have more than the maximum
    }
}

/**
 * Applies damage to the player.
 * Handles invulnerability frames and knockback.
 * @param amount The amount of damage to take.
 * @param knockbackDir The direction of the knockback (-1.0f for left, 1.0f for right).
 * @return True if damage was taken, false if the player was invulnerable.
 */
bool Player::takeDamage(int amount, float knockbackDir) {
    // If we are still invulnerable from the previous hit, ignore this one
    if (mDamageTimer > 0.0f) return false;

    mHp -= amount;
    mDamageTimer = 1.0f; // 1 second of invulnerability after a hit

    // Knockback to make the attack feel impactful
    mVelocity.y = -250.0f;
    mVelocity.x = knockbackDir * 300.0f;

    // Limit minimum health to 0
    if (mHp < 0) mHp = 0;

    return true; // Hit received successfully
}

sf::FloatRect Player::getWeaponHitbox() const {
    float facingDir = (mSprite.getScale().x > 0) ? 1.0f : -1.0f;

    // ==========================================
    // NEW HITBOX CONFIGURATION (Like your image)
    // ==========================================

    // WIDTH (Forward reach):
    // I set it to 75px. It's quite long to match the mining reach!
    // Your characterScaledWidth is ~20px, so this reaches 3 times further.
    float hitboxWidth = 75.0f;

    // HEIGHT: 55px. Covers from the chest down to the knees.
    float hitboxHeight = 55.0f;

    sf::Vector2f playerPos = mSprite.getPosition(); // Remember: The origin is the CENTER.

    // --- X POSITION CALCULATION (Horizontal) ---
    // We want the box to start slightly *behind* the extended fist,
    // in the shoulder/chest area, so you don't miss enemies right in front of you.
    // We use a 5px offset from the center.
    float horizontalOffsetFromCenter = 5.0f * facingDir;

    float xPos = 0.0f;
    if (facingDir > 0) { // Facing Right
        xPos = playerPos.x + horizontalOffsetFromCenter;
    } else { // Facing Left (We subtract the width to calculate the left corner)
        xPos = playerPos.x + horizontalOffsetFromCenter - hitboxWidth;
    }

    // --- Y POSITION CALCULATION (Vertical) ---
    // Remember: The origin is the WAIST (Belly button). In SFML, Y goes up by subtracting.
    // We raise the box 25px to align the top with the shoulder/chest.
    float yPos = playerPos.y - 25.0f;

    return sf::FloatRect(xPos, yPos, hitboxWidth, hitboxHeight);
}