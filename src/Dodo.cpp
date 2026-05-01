#include "Dodo.h"
#include <cmath>
#include <iostream>
#include "Game.h"

/**
 * @brief Constructor for the Dodo.
 * Initializes stats, origin, and starting animation.
 * @param startPos The initial spawning position.
 * @param texture The sprite texture to use.
 */
Dodo::Dodo(sf::Vector2f startPos, const sf::Texture& texture)
    : Mob(startPos, texture, 30) // 30 HP
    , mCurrentAnim(DodoAnim::Idle)
    , mAnimTimer(0.0f)
    , mCurrentFrame(0)
    , mIsAggro(false) // Spawns peaceful
    , mWanderTimer(0.0f)
    , mWanderDir(0)
    , mIsAttacking(false)
    , mAttackCooldown(0.0f)
    , mAttackDuration(0.0f)
{
    mAttackDamage = 25;
    mSprite.setOrigin(32.0f, 64.0f); // Center-bottom origin (Frame size: 64x64)
    mSprite.setTextureRect(sf::IntRect(0, 0, 64, 64));
}

/**
 * @brief Overrides the default damage function.
 * If the Dodo takes damage, it permanently enters an aggressive state.
 * @param amount The damage dealt.
 * @param knockbackDir The direction of the physical knockback.
 * @return True if damage was taken successfully.
 */
bool Dodo::takeDamage(int amount, float knockbackDir) {
    if (!mIsAggro) {
        mIsAggro = true;
        std::cout << "[MOB] You have enraged the Dodo!" << std::endl;
    }
    return Mob::takeDamage(amount, knockbackDir);
}

/**
 * @brief Updates the Dodo's AI (Wandering or Aggro), physics, and animation.
 * @param dt Time elapsed since the last frame.
 * @param playerPos The player's current position (used when aggro).
 * @param world Reference to the game world for collision detection.
 * @param lightLevel The current ambient light level for coloring.
 */
void Dodo::update(sf::Time dt, sf::Vector2f playerPos, World& world, float lightLevel) {
    float dtSec = dt.asSeconds();
    if (mDamageTimer > 0.0f) mDamageTimer -= dtSec;
    if (mAttackCooldown > 0.0f) mAttackCooldown -= dtSec;

    DodoAnim nextAnim = DodoAnim::Idle;
    float distX = playerPos.x - mPos.x;
    float distY = playerPos.y - mPos.y;

    // ==========================================
    // 1. AI BRAIN & COMBAT
    // ==========================================
    if (mDamageTimer > 0.0f) {
        // Stunned from taking a hit
        nextAnim = DodoAnim::Idle;
    }
    else if (mIsAttacking) {
        mAttackDuration -= dtSec;
        nextAnim = DodoAnim::Attack;
        mVel.x = mFacingRight ? 200.0f : -200.0f; // Small dash attack

        if (mAttackDuration <= 0.0f) {
            mIsAttacking = false;
            mAttackCooldown = 2.0f;
        }
    }
    else if (mIsAggro) {
        // --- AGGRESSIVE BEHAVIOR (Chase and Attack) ---
        if (std::abs(distX) < 60.0f && std::abs(distY) < 50.0f && mAttackCooldown <= 0.0f) {
            mIsAttacking = true;
            mAttackDuration = 0.5f;
            mFacingRight = (distX > 0);
            if (mVel.y == 0.0f) mVel.y = -200.0f; // Small hop while pecking
        } else if (std::abs(distX) > 10.0f) {
            mVel.x = (distX > 0) ? 100.0f : -100.0f;
            mFacingRight = (distX > 0);
            nextAnim = DodoAnim::Walk;
        } else {
            mVel.x = 0.0f;
        }
    }
    else {
        // --- PEACEFUL BEHAVIOR (Wander) ---
        mWanderTimer -= dtSec;
        if (mWanderTimer <= 0.0f) {
            mWanderTimer = 2.0f + (rand() % 4); // Change mind every 2-5 seconds
            mWanderDir = (rand() % 3) - 1; // -1 (Left), 0 (Stay), 1 (Right)
        }

        mVel.x = mWanderDir * 40.0f; // Walks very slowly
        if (mWanderDir != 0) {
            mFacingRight = (mWanderDir > 0);
            nextAnim = DodoAnim::Walk;
        }
    }

    // Force jump animation if mid-air
    if (mVel.y != 0.0f && !mIsAttacking) nextAnim = DodoAnim::Jump;

    // ==========================================
    // 2. PRO PHYSICS AND ANTICIPATION SENSOR
    // ==========================================
    auto checkCollision = [&](sf::FloatRect rect) {
        // Use std::floor to ensure collisions don't break in negative map coordinates
        int left = static_cast<int>(std::floor(rect.left / world.getTileSize()));
        int right = static_cast<int>(std::floor((rect.left + rect.width) / world.getTileSize()));
        int top = static_cast<int>(std::floor(rect.top / world.getTileSize()));
        int bottom = static_cast<int>(std::floor((rect.top + rect.height) / world.getTileSize()));
        for (int x = left; x <= right; ++x) {
            for (int y = top; y <= bottom; ++y) {
                if (World::isSolid(world.getBlock(x, y))) return true;
            }
        }
        return false;
    };

    // --- REAL GROUND DETECTOR ---
    // Create an invisible 2-pixel sensor right under the feet
    sf::FloatRect groundSensor = getBounds();
    groundSensor.top += groundSensor.height;
    groundSensor.height = 2.0f;
    bool isGrounded = (mVel.y >= 0.0f && checkCollision(groundSensor));

    float dx = mVel.x * dtSec;
    mPos.x += dx;
    mSprite.setPosition(mPos);

    sf::FloatRect boundsX = getBounds();
    boundsX.height -= 15.0f; // Cut ankles to step over small bumps

    // --- VISUAL ANTICIPATION SENSOR (Jump over obstacles) ---
    // We now use isGrounded instead of mVel.y == 0
    if (isGrounded && std::abs(mVel.x) > 0.0f) {
        sf::FloatRect sensor = boundsX;
        sensor.left += (mVel.x > 0) ? 25.0f : -25.0f; // Look ahead

        if (checkCollision(sensor)) {
            if (mIsAggro) {
                mVel.y = -380.0f;
                isGrounded = false; // Take off!
            } else {
                mWanderDir *= -1; // Turn around if peaceful
                mVel.x = mWanderDir * 40.0f;
                dx = mVel.x * dtSec;
            }
        }
    }

    // Real physical collision X
    if (checkCollision(boundsX)) {
        mPos.x -= dx;
        if (mIsAggro && isGrounded) {
            mVel.y = -380.0f;
            isGrounded = false;
        }
        else if (!mIsAggro) mWanderDir *= -1;
        mSprite.setPosition(mPos);
    }

    // Gravity Y
    mVel.y += 1000.0f * dtSec;
    if (mVel.y > 800.0f) mVel.y = 800.0f; // Terminal velocity
    float dy = mVel.y * dtSec;
    mPos.y += dy;
    mSprite.setPosition(mPos);

    sf::FloatRect boundsY = getBounds();
    // Real physical collision Y
    if (checkCollision(boundsY)) {
        if (mVel.y > 0.0f) { // Hitting ground
            int blockY = static_cast<int>(std::floor((boundsY.top + boundsY.height) / world.getTileSize()));
            float newY = blockY * world.getTileSize();
            if (std::abs(mPos.y - newY) < world.getTileSize() * 2.0f) mPos.y = newY;
            else mPos.y -= dy;
            mVel.y = 0.0f;
            isGrounded = true; // Lands
        } else if (mVel.y < 0.0f) { // Hitting ceiling
            mPos.y -= dy; mVel.y = 0.0f;
        }
        mSprite.setPosition(mPos);
    }

    // ==========================================
    // 3. ANIMATION ENGINE
    // ==========================================
    // Force jump animation while mid-air (going up or down)
    if (!isGrounded && !mIsAttacking) nextAnim = DodoAnim::Jump;

    if (nextAnim != mCurrentAnim) {
        mCurrentAnim = nextAnim;
        mCurrentFrame = 0;
        mAnimTimer = 0.0f;
    }

    mAnimTimer += dtSec;
    float speedFactor = (mCurrentAnim == DodoAnim::Walk && !mIsAggro) ? 0.25f : 0.15f;

    if (mAnimTimer >= speedFactor) {
        mAnimTimer = 0.0f;
        mCurrentFrame = (mCurrentFrame + 1) % 4;
    }

    mSprite.setTextureRect(sf::IntRect(mCurrentFrame * 64, static_cast<int>(mCurrentAnim) * 64, 64, 64));

    // Visuals and damage tint
    float currentScale = std::abs(mSprite.getScale().y);
    mSprite.setScale(mFacingRight ? currentScale : -currentScale, currentScale);
    if (mDamageTimer > 0.0f) {
        mSprite.setColor(sf::Color::Red);
    } else {
        // Base color affected by world lighting
        mSprite.setColor(sf::Color(
            std::min(255, (int)(255 * lightLevel) + 20),
            std::min(255, (int)(255 * lightLevel) + 20),
            std::min(255, (int)(255 * lightLevel) + 20)
        ));
    }
}

// ==========================================
// CUSTOM HITBOX
// ==========================================
/**
 * @brief Generates a custom bounding box for physics and combat.
 * The visual sprite has empty space, so the hitbox is tightened.
 */
sf::FloatRect Dodo::getBounds() const {
    sf::FloatRect bounds = mSprite.getGlobalBounds();
    bounds.left += (bounds.width - 24.0f) / 2.0f;
    bounds.width = 24.0f;
    bounds.top += 20.0f;
    bounds.height -= 20.0f;
    return bounds;
}