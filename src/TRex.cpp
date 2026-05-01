#include "TRex.h"
#include <cmath>
#include <iostream>
#include "Game.h"

TRex::TRex(sf::Vector2f startPos, const sf::Texture& texture)
    : Boss(startPos, texture, 1000, "T-REX ALFA")
    , mRoarTimer(0.0f)
    , mIsRoaring(false)
    , mRoarDuration(0.0f)
    , mCurrentAnim(RexAnimState::Idle)
    , mAnimTimer(0.0f)
    , mCurrentFrame(0)
    , mFramesPerAnimation(4)
    , mIsFleeing(false)       // <--- NUEVO
    , mFleeTimer(0.0f)        // <--- NUEVO
    , mStuckTimer(0.0f)       // <--- NUEVO
    , mFleeDirection(1)       // <--- NUEVO
    , mIsAttacking(false)
    , mAttackCooldown(3.0f) // Lanza un ataque cada 3 segundos
    , mAttackDuration(0.0f)
{
    mAttackDamage = 50;

    // --- CONFIGURACIÓN DE SPRITE GRANDE (148x118) ---
    // Clavamos el origen en el centro-inferior del FRAME, no de la textura completa
    mSprite.setOrigin(FRAME_WIDTH / 2.0f, FRAME_HEIGHT);

    // Escala ajustada (como el dibujo es grande, 1.5x suele bastar. Súbelo si quieres)
    mSprite.setScale(1.5f, 1.5f);

    // Seleccionamos el primer frame (Idle, Fila 0, Columna 0)
    mSprite.setTextureRect(sf::IntRect(0, 0, FRAME_WIDTH, FRAME_HEIGHT));
}

void TRex::update(sf::Time dt, sf::Vector2f playerPos, World& world, float lightLevel) {
    float tileSize = world.getTileSize();
    float dtSec = dt.asSeconds();

    if (mDamageTimer > 0.0f) mDamageTimer -= dtSec;

    // =====================================================
    // 2. IA Y ESTADOS (Cerebro del Boss)
    // =====================================================
    RexAnimState nextAnim = RexAnimState::Idle;

    float distX = playerPos.x - mPos.x;
    float distY = playerPos.y - mPos.y; // Negativo significa que el jugador está por encima

    // 1. ¿ESTÁ HUYENDO? (Prioridad Máxima)
    if (mIsFleeing) {
        mFleeTimer -= dtSec;
        mVel.x = mFleeDirection * 170.0f; // ¡Corre al DOBLE de velocidad!
        mFacingRight = (mVel.x > 0);
        nextAnim = RexAnimState::Walk;

        if (mFleeTimer <= 0.0f) {
            mIsFleeing = false; // Se le pasa el susto y vuelve a cazar
            mStuckTimer = 0.0f; // Reseteamos el cronómetro al terminar de huir
        }
    }
    // 2. ¿ESTÁ RUGIENDO?
    else if (mIsRoaring) {
        mRoarDuration -= dtSec;
        mVel.x = 0.0f;
        nextAnim = RexAnimState::Roar;
        if (mRoarDuration <= 0.0f) mIsRoaring = false;
    }
    // 3. CAZA NORMAL Y LÓGICA ANTI-CAMPERO
   // 3. CAZA NORMAL, ANTI-CAMPERO Y COMBATE (Estilo Terraria)
    else {
        // Reducimos el enfriamiento del ataque
        if (mAttackCooldown > 0.0f) mAttackCooldown -= dtSec;

        if (mRoarTimer >= 15.0f) {
            mRoarTimer = 0.0f;
            mIsRoaring = true;
            mRoarDuration = 2.0f;
            nextAnim = RexAnimState::Roar;
            mStuckTimer = 0.0f;
        }
        // --- ¡NUEVO: LÓGICA DE ATAQUE (EMBESTIDA)! ---
        else if (mIsAttacking) {
            mAttackDuration -= dtSec;
            nextAnim = RexAnimState::Attack; // Usa la 4º fila de tu sprite

            // Durante la embestida, avanza súper rápido (Dash)
            mVel.x = mFacingRight ? 350.0f : -350.0f;

            if (mAttackDuration <= 0.0f) {
                mIsAttacking = false;
                mAttackCooldown = 4.0f; // Tarda 4 segundos en poder volver a morder
            }
        }
        else {
            bool isCamper = (distY < -100.0f && std::abs(distX) < 150.0f);

            if (isCamper) {
                mVel.x = 0.0f;
                nextAnim = RexAnimState::Idle;
                mFacingRight = (distX > 0);

                mStuckTimer += dtSec;
                if (mStuckTimer > 1.2f) {
                    mIsFleeing = true;
                    mFleeTimer = 8.0f;
                    mFleeDirection = (mPos.x > playerPos.x) ? 1 : -1;
                    mStuckTimer = 0.0f;
                    std::cout << "[BOSS] ¡Anti-campero! El T-REX se aleja." << std::endl;
                }
            } else {
                // --- DECISIÓN DE COMBATE ---
                // Si está cerca del jugador (ej: a menos de 180 píxeles) y tiene el ataque listo
                if (std::abs(distX) < 180.0f && std::abs(distY) < 100.0f && mAttackCooldown <= 0.0f) {
                    mIsAttacking = true;
                    mAttackDuration = 0.6f; // El mordisco/embestida dura 0.6 segundos
                    mFacingRight = (distX > 0);
                    // Damos un pequeño salto para hacer la embestida más letal
                    if (mVel.y == 0.0f) mVel.y = -300.0f;
                }
                // Si no está atacando, camina normal
                else if (std::abs(distX) > 20.0f) {
                    mVel.x = (distX > 0) ? 85.0f : -85.0f;
                    mFacingRight = (distX > 0);
                    nextAnim = RexAnimState::Walk;
                } else {
                    mVel.x = 0.0f;
                    nextAnim = RexAnimState::Idle;
                    mStuckTimer = 0.0f;
                }
            }
        }
    }

    // =====================================================
    // 3. MOTOR DE ANIMACIÓN
    // =====================================================
    if (nextAnim != mCurrentAnim) {
        mCurrentAnim = nextAnim;
        mCurrentFrame = 0;
        mAnimTimer = 0.0f;

        switch (mCurrentAnim) {
            case RexAnimState::Idle: mFramesPerAnimation = 4; break;
            case RexAnimState::Walk: mFramesPerAnimation = 4; break;
            case RexAnimState::Roar: mFramesPerAnimation = 4; break;
            case RexAnimState::Attack: mFramesPerAnimation = 4; break; // <--- ¡NUEVO!
        }
    }

    mAnimTimer += dtSec;
    // Si huye, mueve las patas mucho más rápido
    float speedFactor = (mCurrentAnim == RexAnimState::Walk) ?
                        (mIsFleeing ? 0.08f : 0.12f) : 0.20f;

    if (mAnimTimer >= speedFactor) {
        mAnimTimer = 0.0f;
        mCurrentFrame = (mCurrentFrame + 1) % mFramesPerAnimation;
    }

    int texX = mCurrentFrame * FRAME_WIDTH;
    int texY = static_cast<int>(mCurrentAnim) * FRAME_HEIGHT;
    mSprite.setTextureRect(sf::IntRect(texX, texY, FRAME_WIDTH, FRAME_HEIGHT));

    // =====================================================
    // 4. FÍSICAS, GRAVEDAD Y COLISIONES (NIVEL PRO)
    // =====================================================
    auto checkCollision = [&](sf::FloatRect rect) -> bool {
        int left = static_cast<int>(std::floor(rect.left / tileSize));
        int right = static_cast<int>(std::floor((rect.left + rect.width) / tileSize));
        int top = static_cast<int>(std::floor(rect.top / tileSize));
        int bottom = static_cast<int>(std::floor((rect.top + rect.height) / tileSize));

        for (int x = left; x <= right; ++x) {
            for (int y = top; y <= bottom; ++y) {
                if (World::isSolid(world.getBlock(x, y))) return true;
            }
        }
        return false;
    };

    // --- 4.1 MOVIMIENTO HORIZONTAL (EJE X) ---
    float dx = mVel.x * dtSec;
    mPos.x += dx;
    mSprite.setPosition(mPos);

    sf::FloatRect boundsX = mSprite.getGlobalBounds();
    boundsX.left += (boundsX.width - 60.0f) / 2.0f;
    boundsX.width = 60.0f;
    boundsX.top += 10.0f;
    boundsX.height -= 35.0f;

    bool hitWallX = false;
    if (checkCollision(boundsX)) {

        float stepHeight = tileSize + 0.5f;
        sf::FloatRect stepBounds = boundsX;
        stepBounds.top -= stepHeight;

        if (mVel.y == 0.0f && !checkCollision(stepBounds)) {
            mPos.y -= stepHeight;
            mSprite.setPosition(mPos);
        } else {
            mPos.x -= dx;
            mSprite.setPosition(mPos);

            if (!mIsRoaring && mVel.y == 0.0f) mVel.y = -700.0f;
            hitWallX = true;
        }
    }

    // --- 4.2 LÓGICA DE ATASCO FÍSICO (Muros reales) ---
    if (hitWallX && !mIsFleeing && !mIsRoaring) {
        mStuckTimer += dtSec;
        if (mStuckTimer > 1.0f) {
            mIsFleeing = true;
            mFleeTimer = 8.0f;
            mFleeDirection = (dx > 0) ? -1 : 1;
            mStuckTimer = 0.0f;
            std::cout << "[BOSS] El T-REX no puede atravesar el muro y huye..." << std::endl;
        }
    } else if (!hitWallX && std::abs(mVel.x) > 0.0f) {
        // Solo reiniciamos el reloj si realmente estaba caminando y se liberó
        mStuckTimer = 0.0f;
    }

    // --- 4.3 MOVIMIENTO VERTICAL Y GRAVEDAD (EJE Y) ---
    mVel.y += 1000.0f * dtSec;
    if (mVel.y > 800.0f) mVel.y = 800.0f;

    float dy = mVel.y * dtSec;
    mPos.y += dy;
    mSprite.setPosition(mPos);

    sf::FloatRect boundsY = mSprite.getGlobalBounds();
    boundsY.left += (boundsY.width - 60.0f) / 2.0f;
    boundsY.width = 60.0f;

    if (checkCollision(boundsY)) {
        if (mVel.y > 0.0f) {
            int blockY = static_cast<int>(std::floor((boundsY.top + boundsY.height) / tileSize));
            float newY = blockY * tileSize;

            if (std::abs(mPos.y - newY) < tileSize * 2.0f) mPos.y = newY;
            else mPos.y -= dy;

            mVel.y = 0.0f;

        } else if (mVel.y < 0.0f) {
            mPos.y -= dy;
            mVel.y = 0.0f;
        }
        mSprite.setPosition(mPos);
    }

    // =====================================================
    // 5. VISUALES Y ROTACIÓN
    // =====================================================
    mSprite.setPosition(mPos);

    float currentScale = std::abs(mSprite.getScale().y);
    if (mFacingRight) mSprite.setScale(currentScale, currentScale);
    else mSprite.setScale(-currentScale, currentScale);
}

// =====================================================
// HITBOX DE COMBATE PERSONALIZADA
// =====================================================
sf::FloatRect TRex::getBounds() const {
    // 1. Cogemos la caja gigante original (148x118 escalada)
    sf::FloatRect combatBounds = mSprite.getGlobalBounds();

    // 2. Le pegamos tijeretazos al fondo transparente
    // (Ajusta estos píxeles si ves que la flecha aún atraviesa mucho el aire)

    combatBounds.left += 35.0f;       // Recortamos el aire por la izquierda (detrás de la cola)
    combatBounds.width -= 70.0f;      // Hacemos la caja más estrecha

    combatBounds.top += 40.0f;        // Recortamos el bloque de aire gigante por encima de su espalda
    combatBounds.height -= 45.0f;     // Recortamos el aire de los tobillos para centrar la caja en la "carne"

    return combatBounds;
}