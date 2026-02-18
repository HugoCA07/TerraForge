#include "Player.h"
#include <iostream>
#include <cmath>

Player::Player()
: mVelocity(0.f, 0.f)
, mIsGrounded(false)
, mAnimTimer(0.f)
, mCurrentFrame(0)
, mNumFrames(4) // <--- ASSUMING YOU HAVE 4 FRAMES IN YOUR IMAGE
{
    // 1. Load Texture
    if (!mTexture.loadFromFile("assets/player.png")) {
        std::cerr << "Error: Could not load player.png" << std::endl;
    }

    // 2. Pixel Art
    mTexture.setSmooth(false);
    mSprite.setTexture(mTexture);

    // 3. CALCULATE FRAME SIZE
    // We divide the total width of the image by the number of frames
    mFrameWidth = mTexture.getSize().x / mNumFrames;
    mFrameHeight = mTexture.getSize().y;

    // 4. CROP THE FIRST FRAME (Idle)
    // IntRect(x, y, width, height) defines which part of the image is visible.
    mSprite.setTextureRect(sf::IntRect(0, 0, mFrameWidth, mFrameHeight));

    // 5. CENTER THE ORIGIN (Pivot)
    // IMPORTANT: We use mFrameWidth, not the total width of the texture
    mSprite.setOrigin(mFrameWidth / 2.f, mFrameHeight / 2.f);

    // 6. Scale and Position
    // Since your sprite is 32px wide, you want it to measure 3 blocks (96px)...
    // 32 * 3 = 96. Scale factor 3.0f?
    // Adjust this to 1.25f or whatever worked for you before.
    mSprite.setScale(1.25f, 1.25f);

    mSprite.setPosition(100.f, 1000.f);
}

void Player::handleInput() {
    // Reset horizontal velocity every frame (no inertia yet)
    mVelocity.x = 0.f;

    // --- LEFT MOVEMENT ---
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::A)) {
        mVelocity.x = -MOVEMENT_SPEED;

        // Flip Sprite: Negative X scale makes it face Left.
        // We use a positive Y scale to keep it upright.
        mSprite.setScale(1.25f, 1.25f);
    }

    // --- RIGHT MOVEMENT ---
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::D)) {
        mVelocity.x = MOVEMENT_SPEED;

        // Normal Sprite: Positive X scale makes it face Right.
        mSprite.setScale(-1.25f, 1.25f);
    }

    // --- JUMPING ---
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Space) && mIsGrounded) {
        mVelocity.y = JUMP_FORCE; // Apply upward force
        mIsGrounded = false;      // We are now airborne
    }
}


void Player::update(sf::Time dt, World& world) {
    float tileSize = world.getTileSize();
    // Note: Input is now handled in handleInput() and called from Game::processEvents()
    // This keeps input polling separate from physics updates.

    // Store original position for collision response
    sf::Vector2f originalPos = mSprite.getPosition();
    sf::FloatRect bounds = mSprite.getGlobalBounds();

    // Create a slightly smaller hitbox ("skin") to avoid getting stuck on tile edges.
    float skinW = 4.0f;
    float skinH = 0.5f;
    // ---------------------------------------------------------
    // ANIMATION SYSTEM
    // ---------------------------------------------------------

    // 1. Are we moving horizontally?
    if (std::abs(mVelocity.x) > 10.0f) { // We use a small threshold

        // Add time
        mAnimTimer += dt.asSeconds();

        // Every 0.15 seconds, we change the frame
        float timePerFrame = 0.15f;

        if (mAnimTimer >= timePerFrame) {
            mAnimTimer = 0;
            mCurrentFrame++;

            // If we reach the end, go back to the beginning (Loop)
            if (mCurrentFrame >= mNumFrames) {
                mCurrentFrame = 0;
            }
        }
    } else {
        // If standing still, reset to frame 0 (Idle)
        mCurrentFrame = 0;
        mAnimTimer = 0;
    }

    // 2. APPLY THE CROP TO THE TEXTURE
    // We move the "window" X pixels to the right according to the current frame
    int textureX = mCurrentFrame * mFrameWidth;
    mSprite.setTextureRect(sf::IntRect(textureX, 0, mFrameWidth, mFrameHeight));


    // ---------------------------------------------------------
    // FLIP (TURN)
    // ---------------------------------------------------------
    // Make sure to keep your correct scale (1.25f in your case)
    float scale = 1.25f;

    if (mVelocity.x > 0) {
        mSprite.setScale(scale, scale);
    }
    else if (mVelocity.x < 0) {
        mSprite.setScale(-scale, scale);
    }
    
    sf::FloatRect hitbox = bounds;
    // Reduce hitbox size by the skin amount
    hitbox.left += skinW;
    hitbox.width -= (skinW * 2);
    hitbox.top += skinH;
    hitbox.height -= (skinH * 2);

    auto checkCollision = [&](sf::FloatRect rect) -> bool {
        // Coordinate mathematics (std::floor for negatives)
        int left   = static_cast<int>(std::floor(rect.left / tileSize));
        int right  = static_cast<int>(std::floor((rect.left + rect.width) / tileSize));
        int top    = static_cast<int>(std::floor(rect.top / tileSize));
        int bottom = static_cast<int>(std::floor((rect.top + rect.height) / tileSize));

        for (int x = left; x <= right; ++x) {
            for (int y = top; y <= bottom; ++y) {

                int blockID = world.getBlock(x, y);

                // --- MODIFIED COLLISION LOGIC ---
                // Before: if (blockID != 0) return true;

                // NOW: We only collide if it is NOT air (0) AND NOT vegetation (4, 5)
                // That is: We only collide with Dirt (1), Grass (2), and Stone (3).

                if (blockID != 0 && blockID != 4 && blockID != 5) {
                    return true; // It's a solid block!
                }

                // If it is 4 (Wood) or 5 (Leaves), the loop continues and the function returns false.
                // The player walks through it!
            }
        }
        return false;
    };

    // ---------------------------------------------------------
    // PHASE 1: HORIZONTAL MOVEMENT (X) + AUTO-CLIMB
    // We process X and Y movement separately to handle collisions correctly.
    // ---------------------------------------------------------
    float dx = mVelocity.x * dt.asSeconds();
    mSprite.move(dx, 0.f);

    // Check for horizontal collision at the new position
    sf::FloatRect nextHitbox = hitbox;
    nextHitbox.left += dx;
    if (checkCollision(nextHitbox)) {

        // --- AUTO-CLIMB LOGIC ---
        // If we hit a wall, check if we can step up it.
        float stepHeight = tileSize + 2.0f;

        sf::FloatRect stepHitbox = nextHitbox;
        stepHitbox.top -= stepHeight; // Check one block higher

        // If the space one block up is clear, move the player up.
        if (!checkCollision(stepHitbox)) {
            mSprite.move(0.f, -stepHeight);
        }
        else {
            mSprite.setPosition(originalPos.x, mSprite.getPosition().y);
        }
    }

    // ---------------------------------------------------------
    // PHASE 2: VERTICAL MOVEMENT (Y)
    // ---------------------------------------------------------
    // Apply gravity to vertical velocity
    mVelocity.y += GRAVITY * dt.asSeconds();
    float dy = mVelocity.y * dt.asSeconds();
    mSprite.move(0.f, dy);

    // Update hitbox for Y-axis collision check
    hitbox = mSprite.getGlobalBounds();
    hitbox.left += skinW;
    hitbox.width -= (skinW * 2);

    // Check for vertical collision
    if (checkCollision(hitbox)) {
        if (mVelocity.y > 0) { // Moving down (falling or walking down a slope)
            float playerBottom = hitbox.top + hitbox.height;

            // Find the Y coordinate of the block the player is colliding with.
            // Using std::floor is crucial for correct collision detection.
            int blockY = static_cast<int>(std::floor(playerBottom / tileSize));

            // Calculate the new Y position to place the player exactly on top of the block.
            float newY = (blockY * tileSize) - bounds.height;

            // A small check to prevent snapping to a block far away
            if (std::abs(mSprite.getPosition().y - newY) < tileSize) {
                mSprite.setPosition(mSprite.getPosition().x, newY);
            } else {
                mSprite.move(0.f, -dy);
            }
            mVelocity.y = 0;
            mIsGrounded = true;
        }
    }
    else {
        // If there's no block below us, we are in the air.
        mIsGrounded = false;
    }
}

void Player::render(sf::RenderWindow& window, sf::Color lightColor) {
    // Aplicamos el tinte de luz al personaje
    mSprite.setColor(lightColor);

    window.draw(mSprite);
}

sf::Vector2f Player::getCenter() const {
    // Calculate the absolute center of the sprite in the game world.
    sf::FloatRect bounds = mSprite.getGlobalBounds();
    return sf::Vector2f(
        bounds.left + bounds.width / 2.0f,
        bounds.top + bounds.height / 2.0f
    );
}

void Player::setPosition(sf::Vector2f pos) {
    mSprite.setPosition(pos);
}