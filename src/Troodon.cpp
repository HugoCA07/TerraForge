#include "Troodon.h"
#include <cmath>
#include "Game.h"

/**
 * @brief Constructor for the Troodon.
 * Initializes stats, origin, and starting animation.
 * @param startPos The initial spawning position.
 * @param texture The sprite texture to use.
 */
Troodon::Troodon(sf::Vector2f startPos, const sf::Texture& texture)
    : Mob(startPos, texture, 50) // 50 HP (Stronger than the Dodo)
    , mCurrentAnim(TroodonAnim::Idle)
    , mAnimTimer(0.0f)
    , mCurrentFrame(0)
    , mIsAttacking(false)
    , mAttackCooldown(0.0f)
    , mAttackDuration(0.0f)
{
    mAttackDamage = 45;
    mSprite.setOrigin(32.0f, 48.0f); // Center-bottom origin (Frame size: 64x48)
    mSprite.setTextureRect(sf::IntRect(0, 0, 64, 48));
}

/**
 * @brief Updates the Troodon's logic, AI, physics, and animation.
 * @param dt Time elapsed since the last frame.
 * @param playerPos The player's current position to track and attack.
 * @param world Reference to the game world for collision detection.
 * @param lightLevel The current ambient light level for coloring.
 */
void Troodon::update(sf::Time dt, sf::Vector2f playerPos, World& world, float lightLevel) {
    float dtSec = dt.asSeconds();
    if (mDamageTimer > 0.0f) mDamageTimer -= dtSec;
    if (mAttackCooldown > 0.0f) mAttackCooldown -= dtSec;

    TroodonAnim nextAnim = TroodonAnim::Idle;
    float distX = playerPos.x - mPos.x;
    float distY = playerPos.y - mPos.y;

    // ==========================================
    // 1. HUNTER AI BRAIN
    // ==========================================
    if (mDamageTimer > 0.0f) {
        nextAnim = TroodonAnim::Idle; // Stunned while taking damage
    }
    else if (mIsAttacking) {
        mAttackDuration -= dtSec;
        nextAnim = TroodonAnim::Attack;
        mVel.x = mFacingRight ? 280.0f : -280.0f; // Super fast dash attack!

        if (mAttackDuration <= 0.0f) {
            mIsAttacking = false;
            mAttackCooldown = 1.5f; // Fast attack recovery
        }
    }
    else {
        // Attack range check
        if (std::abs(distX) < 70.0f && std::abs(distY) < 50.0f && mAttackCooldown <= 0.0f) {
            mIsAttacking = true;
            mAttackDuration = 0.4f; // Quick lethal bite
            mFacingRight = (distX > 0);
            if (mVel.y == 0.0f) mVel.y = -250.0f; // Leap at the jugular
        }
        // Chase range check (Huge sight radius)
        else if (std::abs(distX) < 800.0f) {
            mVel.x = (distX > 0) ? 140.0f : -140.0f; // Runs quite fast
            mFacingRight = (distX > 0);
            nextAnim = TroodonAnim::Walk;
        }
        // Idle (Stays still stalking if you are far away)
        else {
            mVel.x = 0.0f;
        }
    }

    if (mVel.y != 0.0f && !mIsAttacking) nextAnim = TroodonAnim::Jump;

    // ==========================================
    // 2. PRO PHYSICS AND ANTICIPATION SENSOR
    // ==========================================
    auto checkCollision = [&](sf::FloatRect rect) {
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
    sf::FloatRect groundSensor = getBounds();
    groundSensor.top += groundSensor.height;
    groundSensor.height = 2.0f;
    bool isGrounded = (mVel.y >= 0.0f && checkCollision(groundSensor));

    float dx = mVel.x * dtSec;
    mPos.x += dx;
    mSprite.setPosition(mPos);

    sf::FloatRect boundsX = getBounds();
    boundsX.height -= 15.0f; // Cut the ankles for easier stepping

    // --- VISUAL ANTICIPATION SENSOR (Jump over obstacles) ---
    if (isGrounded && std::abs(mVel.x) > 0.0f) {
        sf::FloatRect sensor = boundsX;
        sensor.left += (mVel.x > 0) ? 25.0f : -25.0f; // Look ahead

        if (checkCollision(sensor)) {
            mVel.y = -380.0f;
            isGrounded = false; // Take off!
        }
    }

    // Real physical collision X
    if (checkCollision(boundsX)) {
        mPos.x -= dx;
        if (isGrounded) {
            mVel.y = -380.0f;
            isGrounded = false;
        }
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
            mPos.y -= dy;
            mVel.y = 0.0f;
        }
        mSprite.setPosition(mPos);
    }

    // ==========================================
    // 3. ANIMATION ENGINE
    // ==========================================
    // Force jump animation while mid-air
    if (!isGrounded && !mIsAttacking) nextAnim = TroodonAnim::Jump;

    if (nextAnim != mCurrentAnim) {
        mCurrentAnim = nextAnim;
        mCurrentFrame = 0;
        mAnimTimer = 0.0f;
    }

    mAnimTimer += dtSec;
    if (mAnimTimer >= 0.12f) { // Fast animation speed
        mAnimTimer = 0.0f;
        mCurrentFrame = (mCurrentFrame + 1) % 4;
    }

    mSprite.setTextureRect(sf::IntRect(mCurrentFrame * 64, static_cast<int>(mCurrentAnim) * 48, 64, 48));

    // Visuals and damage tint
    float currentScale = std::abs(mSprite.getScale().y);
    mSprite.setScale(mFacingRight ? currentScale : -currentScale, currentScale);
    if (mDamageTimer > 0.0f) {
        mSprite.setColor(sf::Color::Red);
    } else {
        // Base color affected by world lighting, but a bit brighter so it's not invisible at night
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
 * The visual sprite has a lot of empty space, so the hitbox is tightened.
 */
sf::FloatRect Troodon::getBounds() const {
    sf::FloatRect bounds = mSprite.getGlobalBounds();
    bounds.left += (bounds.width - 24.0f) / 2.0f;
    bounds.width = 24.0f;
    bounds.top += 10.0f;
    bounds.height -= 10.0f;
    return bounds;
}