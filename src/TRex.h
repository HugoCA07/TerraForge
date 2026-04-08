#pragma once
#include "Boss.h"
#include "World.h"

class TRex : public Boss {
public:
    // --- CONSTANTES DE ANIMACIÓN (AAA) ---
    // Basado en tus datos: Cada frame es 148x118px
    static const int FRAME_WIDTH = 148;
    static const int FRAME_HEIGHT = 118;

    // Asumimos un layout estándar en tu spritesheet (ej: Fila 0 Idle, 1 Walk, 2 Roar, 3 Attack)
    enum class RexAnimState { Idle = 0, Walk = 1, Roar = 2, Attack = 3 };

    TRex(sf::Vector2f startPos, const sf::Texture& texture);

    void update(sf::Time dt, sf::Vector2f playerPos, World& world, float lightLevel) override;

    bool isRoaring() const { return mIsRoaring; }

private:
    float mRoarTimer;
    bool mIsRoaring;
    float mRoarDuration;

    // --- VARIABLES DE ANIMACIÓN ---
    RexAnimState mCurrentAnim;
    float mAnimTimer;
    int mCurrentFrame;
    int mFramesPerAnimation;

    // --- NUEVO: SISTEMA DE HUIDA TÁCTICA ---
    bool mIsFleeing;
    float mFleeTimer;
    float mStuckTimer;
    int mFleeDirection;

    // --- NUEVO: Hitbox de Combate Personalizada ---
    sf::FloatRect getBounds() const override;

    // --- NUEVO: SISTEMA DE COMBATE TERRARIA ---
    bool mIsAttacking;
    float mAttackCooldown; // Tiempo entre ataques
    float mAttackDuration; // Lo que dura la animación del mordisco
};