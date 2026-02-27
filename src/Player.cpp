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
    // --- CONFIGURACIÓN DEL ARMA ---
    if (!mWeaponTexture.loadFromFile("assets/PickaxeWoodHands.png")) {
        std::cerr << "Error: No se encontro PickaxeWoodHands.png" << std::endl;
    }
    mWeaponTexture.setSmooth(false); // <--- Vital para pixel art
    mWeaponSprite.setTexture(mWeaponTexture);

    // Ponemos el pivote abajo del todo y en el centro (justo por donde se agarra el palo)
    mWeaponSprite.setOrigin(mWeaponTexture.getSize().x / 2.0f, mWeaponTexture.getSize().y);

    // Le damos la misma escala que tiene tu personaje (1.25f)
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
            // Fase 1: Levantando el arma (0.2 segundos)
            if (mCombatTimer >= 0.2f) {
                mCombatState = CombatState::Active;
                mCombatTimer = 0.0f;
                mHasHitThisSwing = false; // <--- ¡NUEVO! Quitamos el seguro al lanzar el golpe
                std::cout << "[COMBATE] !ZAS! Golpe activo." << std::endl;
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
    // 1.5 VISUALES DEL ARMA (COREOGRAFÍA Y REPOSO)
    // ==========================================
    bool isHoldingPickaxe = (mEquippedWeaponID >= 21 && mEquippedWeaponID <= 24);
    if (isHoldingPickaxe) {
        float facingDir = (mSprite.getScale().x > 0) ? 1.0f : -1.0f;

        // Los mismos ajustes que pusimos antes para que salga del puño
        float handOffsetX = 25.0f * facingDir;
        float handOffsetY = 15.0f;

        // 1. SIEMPRE lo pegamos a la mano y lo volteamos
        mWeaponSprite.setPosition(mSprite.getPosition().x + handOffsetX, mSprite.getPosition().y + handOffsetY);
        mWeaponSprite.setScale(1.25f * facingDir, 1.25f);

        // 2. DECIDIMOS LA ROTACIÓN
        if (mCombatState != CombatState::None) {
            // --- ESTAMOS ATACANDO (Coreografía) ---
            if (mCombatState == CombatState::Windup) {
                float progress = mCombatTimer / 0.2f;
                mWeaponSprite.setRotation(-60.0f * progress * facingDir);
            }
            else if (mCombatState == CombatState::Active) {
                float progress = mCombatTimer / 0.1f;
                mWeaponSprite.setRotation((-60.0f + (150.0f * progress)) * facingDir);
            }
            else if (mCombatState == CombatState::Recovery) {
                mWeaponSprite.setRotation(90.0f * facingDir);
            }
        }
        else {
            // --- ESTAMOS EN REPOSO / CAMINANDO ---
            // Le damos una pequeña inclinación hacia atrás/arriba para que parezca que
            // lo lleva apoyado al hombro o listo para usarse.
            // ¡Prueba a cambiar este -20.0f si lo quieres más recto o más caído!
            mWeaponSprite.setRotation(1.0f * facingDir);
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
    // DECIDIR ANIMACIÓN (Depende del movimiento y del arma)
    // ==========================================
    isHoldingPickaxe = (mEquippedWeaponID >= 21 && mEquippedWeaponID <= 24);

    if (std::abs(mVelocity.x) > 10.0f) {
        mCurrentState = AnimState::Walk;
        // Si tiene el pico, usa la fila 3 (Caminar con arma). Si no, la fila 1 (Caminar normal)
        mCurrentRow = isHoldingPickaxe ? 3 : 1;
    } else {
        mCurrentState = AnimState::Idle;
        // Si tiene el pico, usa la fila 2 (Quieto con arma). Si no, la fila 0 (Quieto normal)
        mCurrentRow = isHoldingPickaxe ? 2 : 0;
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
    // FLIP (TURN) - ¡Corregido!
    // ---------------------------------------------------------
    float scale = 1.25f;

    // Usamos mMoveDirection (la intención del teclado) en lugar de mVelocity (las físicas)
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
    // Dibujamos al jugador
    mSprite.setColor(lightColor);
    window.draw(mSprite);

    // --- NUEVO: Dibujamos el arma SIEMPRE que la tengamos equipada ---
    bool isHoldingPickaxe = (mEquippedWeaponID >= 21 && mEquippedWeaponID <= 24);

    if (isHoldingPickaxe) {
        // Le aplicamos el color de la luz ambiental (+ un toque de brillo)
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
    // NUEVA CONFIGURACIÓN DE HITBOX (Como tu imagen)
    // ==========================================

    // ANCHO (Alcance hacia adelante):
    // He puesto 75px. ¡Es bastante largo para que se parezca al alcance de minería!
    // Tu personajeScaledWidth es ~20px, así que esto llega 3 veces más lejos.
    float hitboxWidth = 75.0f;

    // ALTO: 55px. Cubre desde el pecho hasta las rodillas.
    float hitboxHeight = 55.0f;

    sf::Vector2f playerPos = mSprite.getPosition(); // Recordamos: El origin es el CENTRO.

    // --- CÁLCULO DE POSICIÓN X (Horizontal) ---
    // Queremos que la caja empiece un poco *detrás* del puño extendido,
    // en la zona del hombro/pecho, para que no falles a enemigos pegados a ti.
    // Usamos un offset de 5px desde el centro.
    float horizontalOffsetFromCenter = 5.0f * facingDir;

    float xPos = 0.0f;
    if (facingDir > 0) { // Mirando Derecha
        xPos = playerPos.x + horizontalOffsetFromCenter;
    } else { // Mirando Izquierda (Restamos el ancho para calcular la esquina izquierda)
        xPos = playerPos.x + horizontalOffsetFromCenter - hitboxWidth;
    }

    // --- CÁLCULO DE POSICIÓN Y (Vertical) ---
    // Recordamos: El origin es el WAIST (Cintura/Ombligo). En SFML la Y sube restando.
    // Subimos la caja 25px para alinear la parte superior con el hombro/pecho.
    float yPos = playerPos.y - 25.0f;

    return sf::FloatRect(xPos, yPos, hitboxWidth, hitboxHeight);
}