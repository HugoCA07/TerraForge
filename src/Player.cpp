#include "Player.h"
#include <iostream>
#include <cmath>
#include <algorithm> // For std::min

/**
 * @brief Constructor for the Player class.
 * Initializes attributes, loads textures, and configures sprites.
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
    , mIsBleeding(false)
    , mBleedTimer(0.0f)
    , mBleedTickTimer(0.0f)
    , mIsRegenerating(false)
    , mRegenTimer(0.0f)
    , mRegenTickTimer(0.0f)
{
    if (!mTexture.loadFromFile("assets/Player.png")) {
        std::cerr << "Error: Could not load player.png" << std::endl;
    }
    mTexture.setSmooth(false);
    mSprite.setTexture(mTexture);

    // Calculate frame dimensions from the spritesheet
    int numRows = 4;
    mFrameWidth = mTexture.getSize().x / mNumFrames;
    mFrameHeight = mTexture.getSize().y / numRows;

    mSprite.setTextureRect(sf::IntRect(0, 0, mFrameWidth, mFrameHeight));
    mSprite.setOrigin(mFrameWidth / 2.f, mFrameHeight / 2.f);
    mSprite.setScale(1.25f, 1.25f);
    mSprite.setPosition(100.f, 1000.f);

    mWeaponTexture.setSmooth(false); // Vital for pixel art
    mWeaponSprite.setTexture(mWeaponTexture);
    // Set pivot to the bottom center of the weapon (where the handle is)
    mWeaponSprite.setOrigin(mWeaponTexture.getSize().x / 2.0f, mWeaponTexture.getSize().y);
    mWeaponSprite.setScale(1.25f, 1.25f);
}

/**
 * @brief Handles player input for movement and combat initiation.
 * @param isInventoryOpen Disables most actions if the inventory is open.
 */
void Player::handleInput(bool isInventoryOpen) {
    mMoveDirection = 0.0f; // Reset movement intention

    // Player can only move, jump, or attack if not already in a combat animation
    if (mCombatState == CombatState::None) {
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::A)) mMoveDirection = -1.0f;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::D)) mMoveDirection = 1.0f;

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Space) && mIsGrounded) {
            float jumpPower = mIsOverweight ? -350.0f : JUMP_FORCE;
            mVelocity.y = jumpPower;
            mIsGrounded = false;
        }

        // Start attack sequence on Left Click if inventory is closed
        if (!isInventoryOpen && sf::Mouse::isButtonPressed(sf::Mouse::Left)) {
            mCombatState = CombatState::Windup;
            mCombatTimer = 0.0f;
            mVelocity.x = 0.0f; // Stop moving to start the attack
            std::cout << "[COMBAT] Preparing strike..." << std::endl;
        }
    }
}

/**
 * @brief Updates player state, including combat, physics, and animation.
 * @param dt Delta time for frame-independent calculations.
 * @param world Reference to the world for collision detection.
 */
void Player::update(sf::Time dt, World& world) {
    handleStatusEffects(dt);
    float tileSize = world.getTileSize();
    sf::Vector2f originalPos = mSprite.getPosition();
    sf::FloatRect bounds = mSprite.getGlobalBounds();
    float skinW = 4.0f, skinH = 0.5f; // Hitbox skin to prevent getting stuck

    // ==========================================
    // 1. COMBAT STATE MACHINE
    // ==========================================
    if (mCombatState != CombatState::None) {
        mCombatTimer += dt.asSeconds();

        if (mCombatState == CombatState::Windup) { // Phase 1: Raising weapon
            if (mCombatTimer >= 0.2f) {
                mCombatState = CombatState::Active;
                mCombatTimer = 0.0f;
                mHasHitThisSwing = false; // Reset hit flag for the new swing
                std::cout << "[COMBAT] WHAM! Active hit." << std::endl;
            }
        }
        else if (mCombatState == CombatState::Active) { // Phase 2: Damage window
            if (mCombatTimer >= 0.1f) {
                mCombatState = CombatState::Recovery;
                mCombatTimer = 0.0f;
                std::cout << "[COMBAT] Recovering balance..." << std::endl;
            }
        }
        else if (mCombatState == CombatState::Recovery) { // Phase 3: Cooldown
            if (mCombatTimer >= 0.3f) {
                mCombatState = CombatState::None;
                mCombatTimer = 0.0f;
                std::cout << "[COMBAT] Ready for another action." << std::endl;
            }
        }
    }

    // ==========================================
    // 1.5 WEAPON VISUALS (CHOREOGRAPHY & IDLE)
    // ==========================================
    bool isHoldingMelee = (mEquippedWeaponID >= 21 && mEquippedWeaponID <= 24) || (mEquippedWeaponID >= 31 && mEquippedWeaponID <= 34);
    bool isHoldingBow = (mEquippedWeaponID == 35);

    if (isHoldingMelee || isHoldingBow) {
        const sf::Texture* tex = world.getHeldTexture(mEquippedWeaponID);
        if (tex && mWeaponSprite.getTexture() != tex) {
            mWeaponSprite.setTexture(*tex, true);
            // Set origin based on weapon type
            if (isHoldingBow) {
                mWeaponSprite.setOrigin(tex->getSize().x / 2.0f, tex->getSize().y / 2.0f); // Center grip
            } else {
                mWeaponSprite.setOrigin(tex->getSize().x * 0.15f, tex->getSize().y * 0.75f); // Handle grip
            }
        }

        float facingDir = (mSprite.getScale().x > 0) ? 1.0f : -1.0f;
        float handOffsetX = 22.0f * facingDir;
        float handOffsetY = 5.0f;
        mWeaponSprite.setPosition(mSprite.getPosition().x + handOffsetX, mSprite.getPosition().y + handOffsetY);
        mWeaponSprite.setScale(0.85f * facingDir, 0.85f);

        // --- WEAPON SWING/DRAW CHOREOGRAPHY (USING EASING MATH) ---
        if (mCombatState != CombatState::None) {
            if (mCombatState == CombatState::Windup) { // Phase 1: Windup / Draw
                float progress = mCombatTimer / 0.2f;
                float ease = 1.0f - std::pow(1.0f - progress, 3.0f); // EaseOutCubic
                if (isHoldingMelee) mWeaponSprite.setRotation(-70.0f * ease * facingDir);
                else if (isHoldingBow) mWeaponSprite.setRotation((90.0f - (90.0f * ease)) * facingDir);
            }
            else if (mCombatState == CombatState::Active) { // Phase 2: Swing / Shoot
                float progress = mCombatTimer / 0.1f;
                if (isHoldingMelee) {
                    float ease = std::pow(progress, 3.0f); // EaseInCubic
                    float currentRot = -70.0f + (170.0f * ease);
                    float spin = (progress > 0.8f) ? std::pow((progress - 0.8f) / 0.2f, 2.0f) * 15.0f : 0.0f;
                    mWeaponSprite.setRotation((currentRot + spin) * facingDir);
                } else if (isHoldingBow) {
                    float vibration = std::sin(progress * 30.0f) * 8.0f; // Bowstring vibration
                    mWeaponSprite.setRotation(vibration * facingDir);
                }
            }
            else if (mCombatState == CombatState::Recovery) { // Phase 3: Recover
                float progress = mCombatTimer / 0.3f;
                float ease = 1.0f - std::pow(1.0f - progress, 2.0f); // EaseOutQuad
                if (isHoldingMelee) mWeaponSprite.setRotation((100.0f - (100.0f * ease)) * facingDir);
                else if (isHoldingBow) mWeaponSprite.setRotation((90.0f * ease) * facingDir);
            }
        } else { // Idle Pose
            if (isHoldingMelee) mWeaponSprite.setRotation(5.0f * facingDir);
            else if (isHoldingBow) mWeaponSprite.setRotation(90.0f * facingDir); // Bow held vertically
        }
    }

    // ==========================================
    // 2. FLUID MOVEMENT (INERTIA AND FRICTION)
    // ==========================================
    float maxSpd = mIsOverweight ? 60.0f : mMaxSpeed;
    if (mMoveDirection != 0.0f) {
        mVelocity.x += mMoveDirection * mAcceleration * dt.asSeconds();
        mVelocity.x = std::clamp(mVelocity.x, -maxSpd, maxSpd);
    } else {
        if (mVelocity.x > 0.0f) mVelocity.x = std::max(0.0f, mVelocity.x - mFriction * dt.asSeconds());
        else if (mVelocity.x < 0.0f) mVelocity.x = std::min(0.0f, mVelocity.x + mFriction * dt.asSeconds());
    }
    mVelocity.y += GRAVITY * dt.asSeconds();

    // ==========================================
    // 3. COLLISION DETECTION & RESPONSE
    // ==========================================
    auto checkCollision = [&](sf::FloatRect rect) -> bool {
        int left   = static_cast<int>(std::floor(rect.left / tileSize));
        int right  = static_cast<int>(std::floor((rect.left + rect.width) / tileSize));
        int top    = static_cast<int>(std::floor(rect.top / tileSize));
        int bottom = static_cast<int>(std::floor((rect.top + rect.height) / tileSize));
        for (int x = left; x <= right; ++x) {
            for (int y = top; y <= bottom; ++y) {
                if (world.isSolid(world.getBlock(x, y))) return true;
            }
        }
        return false;
    };

    // --- X-AXIS COLLISION ---
    float dx = mVelocity.x * dt.asSeconds();
    mSprite.move(dx, 0.f);
    sf::FloatRect hitboxX = mSprite.getGlobalBounds();
    hitboxX.left += skinW; hitboxX.width -= (skinW * 2);
    hitboxX.top += skinH; hitboxX.height -= (skinH * 2);

    if (checkCollision(hitboxX)) {
        // Auto-step logic
        float stepHeight = tileSize + 0.1f;
        sf::FloatRect stepHitbox = hitboxX;
        stepHitbox.top -= stepHeight;
        if (!checkCollision(stepHitbox)) {
            mSprite.move(0.f, -stepHeight);
        } else {
            mSprite.setPosition(originalPos.x, mSprite.getPosition().y);
            mVelocity.x = 0.0f; // Stop on wall impact
        }
    }

    // --- Y-AXIS COLLISION ---
    float dy = mVelocity.y * dt.asSeconds();
    mSprite.move(0.f, dy);
    sf::FloatRect hitboxY = mSprite.getGlobalBounds();
    hitboxY.left += skinW; hitboxY.width -= (skinW * 2);

    if (checkCollision(hitboxY)) {
        if (mVelocity.y > 0) { // Hitting ground
            float playerBottom = hitboxY.top + hitboxY.height;
            int blockY = static_cast<int>(std::floor(playerBottom / tileSize));
            float newY = (blockY * tileSize) - bounds.height;
            if (std::abs(mSprite.getPosition().y - newY) < tileSize) mSprite.setPosition(mSprite.getPosition().x, newY);
            else mSprite.move(0.f, -dy);
            mVelocity.y = 0;
            mIsGrounded = true;
        }
        else if (mVelocity.y < 0) { // Hitting ceiling
            mSprite.move(0.f, -dy);
            mVelocity.y = 0;
        }
    } else {
        mIsGrounded = false;
    }

    // ==========================================
    // 4. ANIMATION & VISUALS
    // ==========================================
    if (mDamageTimer > 0.0f) {
        mDamageTimer -= dt.asSeconds();
        mSprite.setColor(sf::Color(255, 100, 100)); // Damage flash
    } else {
        mSprite.setColor(sf::Color::White);
    }

    // Decide animation based on movement and equipped weapon
    bool hasWeaponAnim = (mEquippedWeaponID >= 21 && mEquippedWeaponID <= 35);
    mCurrentState = (std::abs(mVelocity.x) > 10.0f) ? AnimState::Walk : AnimState::Idle;
    mCurrentRow = (mCurrentState == AnimState::Idle) ? (hasWeaponAnim ? 2 : 0) : (hasWeaponAnim ? 3 : 1);

    mAnimTimer += dt.asSeconds();
    float timePerFrame = (mCurrentState == AnimState::Walk) ? 0.1f : 0.25f;
    if (mAnimTimer >= timePerFrame) {
        mAnimTimer = 0.0f;
        mCurrentFrame = (mCurrentFrame + 1) % mNumFrames;
    }

    sf::IntRect animRect(mCurrentFrame * mFrameWidth, mCurrentRow * mFrameHeight, mFrameWidth, mFrameHeight);
    mSprite.setTextureRect(animRect);

    // Sync armor layers with player animation
    for (int i = 0; i < 4; ++i) {
        if (mArmorAnimTextures[i] != nullptr) {
            mArmorAnimSprites[i].setTextureRect(animRect);
        }
    }

    // Flip sprite based on movement direction
    float scale = 1.25f;
    if (mMoveDirection > 0) mSprite.setScale(scale, scale);
    else if (mMoveDirection < 0) mSprite.setScale(-scale, scale);
}

/**
 * @brief Renders the player, armor layers, and held weapon.
 * @param window The SFML RenderWindow to draw on.
 * @param lightColor The calculated ambient light color.
 */
void Player::render(sf::RenderWindow& window, sf::Color lightColor) {
    // 1. Draw player body
    mSprite.setColor(lightColor);
    window.draw(mSprite);

    // 2. Draw armor layers on top
    for (int i = 0; i < 4; ++i) {
        if (mArmorAnimTextures[i] != nullptr) {
            mArmorAnimSprites[i].setPosition(mSprite.getPosition());
            mArmorAnimSprites[i].setScale(mSprite.getScale());
            mArmorAnimSprites[i].setColor(lightColor);
            window.draw(mArmorAnimSprites[i]);
        }
    }

    // 3. Draw weapon on top of everything
    bool isHoldingWeapon = (mEquippedWeaponID >= 21 && mEquippedWeaponID <= 35);
    if (isHoldingWeapon) {
        sf::Color weaponColor = lightColor;
        weaponColor.r = std::min(255, weaponColor.r + 30);
        weaponColor.g = std::min(255, weaponColor.g + 30);
        weaponColor.b = std::min(255, weaponColor.b + 30);
        mWeaponSprite.setColor(weaponColor);
        window.draw(mWeaponSprite);
    }
}

/**
 * @brief Gets the absolute center coordinates of the player sprite.
 */
sf::Vector2f Player::getCenter() const {
    sf::FloatRect bounds = mSprite.getGlobalBounds();
    return sf::Vector2f(bounds.left + bounds.width / 2.0f, bounds.top + bounds.height / 2.0f);
}

void Player::setPosition(sf::Vector2f pos) {
    mSprite.setPosition(pos);
}

/**
 * @brief Restores player health, capped at max HP.
 */
void Player::heal(int amount) {
    mHp = std::min(mMaxHp, mHp + amount);
}

/**
 * @brief Applies damage and knockback to the player.
 * @return True if damage was taken, false if invulnerable.
 */
bool Player::takeDamage(int amount, float knockbackDir) {
    if (mDamageTimer > 0.0f) return false; // Invulnerable
    mHp -= amount;
    mDamageTimer = 1.0f; // 1 second of invulnerability
    mVelocity.y = -250.0f;
    mVelocity.x = knockbackDir * 300.0f;
    if (mHp < 0) mHp = 0;
    return true;
}

/**
 * @brief Calculates the hitbox for the player's melee attack.
 */
sf::FloatRect Player::getWeaponHitbox() const {
    float facingDir = (mSprite.getScale().x > 0) ? 1.0f : -1.0f;
    float hitboxWidth = 75.0f;
    float hitboxHeight = 55.0f;
    sf::Vector2f playerPos = mSprite.getPosition();
    float horizontalOffsetFromCenter = 5.0f * facingDir;
    float xPos = (facingDir > 0) ? (playerPos.x + horizontalOffsetFromCenter) : (playerPos.x + horizontalOffsetFromCenter - hitboxWidth);
    float yPos = playerPos.y - 25.0f;
    return sf::FloatRect(xPos, yPos, hitboxWidth, hitboxHeight);
}

/**
 * @brief Sets the textures for the animated armor layers.
 * @param head Helmet texture.
 * @param chest Chestplate texture.
 * @param legs Leggings texture.
 * @param boots Boots texture.
 */
void Player::setArmorAnimTextures(const sf::Texture* head, const sf::Texture* chest, const sf::Texture* legs, const sf::Texture* boots) {
    mArmorAnimTextures[0] = head;
    mArmorAnimTextures[1] = chest;
    mArmorAnimTextures[2] = legs;
    mArmorAnimTextures[3] = boots;

    for (int i = 0; i < 4; ++i) {
        if (mArmorAnimTextures[i] != nullptr) {
            mArmorAnimSprites[i].setTexture(*mArmorAnimTextures[i]);
            // Origin must match the player's sprite for correct layering
            mArmorAnimSprites[i].setOrigin(mFrameWidth / 2.f, mFrameHeight / 2.f);
        }
    }
}

// ==========================================
// SISTEMA DE ESTADOS ALTERADOS (BUFFOS/DEBUFFOS)
// ==========================================
void Player::applyBleeding(float duration) {
    mIsBleeding = true;
    // Si ya estábamos sangrando, sumamos el tiempo (o lo reiniciamos, tú eliges)
    mBleedTimer = std::max(mBleedTimer, duration);
}

void Player::applyRegeneration(float duration) {
    mIsRegenerating = true;
    mRegenTimer = std::max(mRegenTimer, duration);
}

void Player::handleStatusEffects(sf::Time dt) {
    float dtSec = dt.asSeconds();

    // 1. Lógica del Sangrado
    if (mIsBleeding) {
        mBleedTimer -= dtSec;
        mBleedTickTimer += dtSec;

        // Cada 1.5 segundos, pierdes 2 de vida
        if (mBleedTickTimer >= 1.5f) {
            mBleedTickTimer = 0.0f;
            mHp -= 2; // Daño directo que ignora la armadura
            // Hacemos que brilles en rojo un instante
            mDamageTimer = 0.2f;
        }

        if (mBleedTimer <= 0.0f) {
            mIsBleeding = false;
        }
    }

    // 2. Lógica de la Regeneración
    if (mIsRegenerating) {
        mRegenTimer -= dtSec;
        mRegenTickTimer += dtSec;

        // Cada 1.0 segundo, recuperas 1 de vida
        if (mRegenTickTimer >= 1.0f) {
            mRegenTickTimer = 0.0f;
            heal(1); // Usamos tu función heal() para no pasar del máximo
        }

        if (mRegenTimer <= 0.0f) {
            mIsRegenerating = false;
        }
    }
}