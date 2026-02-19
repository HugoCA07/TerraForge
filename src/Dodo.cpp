#include "Dodo.h"
#include <cmath>

Dodo::Dodo(sf::Vector2f startPos, const sf::Texture& texture)
    : mPos(startPos)
    , mVel(0.f, 0.f)
    , mFacingRight(true)
    , mHp(20)            // 20 puntos de vida por defecto
    , mDamageTimer(0.f)  // Sin daño al nacer
{
    mSprite.setTexture(texture);
    // Origen en los pies para que la gravedad y rotación funcionen bien
    mSprite.setOrigin(texture.getSize().x / 2.0f, texture.getSize().y);
}

void Dodo::update(sf::Time dt, sf::Vector2f playerPos, World& world) {
    float tileSize = world.getTileSize();

    // -----------------------------------------------------
    // 1. IA (CEREBRO DEL DODO) - AHORA VA PRIMERO
    // -----------------------------------------------------
    float distX = playerPos.x - mPos.x;
    float distY = playerPos.y - mPos.y;
    float distTotal = std::sqrt(distX*distX + distY*distY);

    // Como aún no hemos sumado la gravedad, mVel.y sigue siendo 0
    // si el dodo chocó con el suelo en el frame anterior.
    if (mVel.y == 0.0f) {
        if (distTotal < 400.0f) {
            // ¡A por el jugador!
            if (distX > 0) {
                mVel.x = 100.0f;
                mFacingRight = true;
            } else {
                mVel.x = -100.0f;
                mFacingRight = false;
            }
        } else {
            // Se queda quieto
            mVel.x *= 0.8f;
        }
    }

    // -----------------------------------------------------
    // 2. GRAVEDAD
    // -----------------------------------------------------
    mVel.y += 900.0f * dt.asSeconds();

    // -----------------------------------------------------
    // 3. APLICAR MOVIMIENTO
    // -----------------------------------------------------
    mPos.x += mVel.x * dt.asSeconds();
    mPos.y += mVel.y * dt.asSeconds();

    // -----------------------------------------------------
    // 4. COLISIONES
    // -----------------------------------------------------
    int gridX = static_cast<int>(mPos.x / tileSize);
    int gridY = static_cast<int>(mPos.y / tileSize);

    // A) Colisión con el suelo
    if (world.getBlock(gridX, gridY) != 0) {
        mPos.y = gridY * tileSize;
        mVel.y = 0.0f; // <-- ESTO ES CLAVE. Le dice al próximo frame que estamos en el suelo.
    }

    // B) Colisión con paredes (Si choca, salta)
    int frontX = static_cast<int>((mPos.x + (mFacingRight ? 18 : -18)) / tileSize);
    if (world.getBlock(frontX, gridY - 1) != 0 && mVel.y == 0.0f) {
        mVel.y = -280.0f; // Pequeño salto
    }

    // -----------------------------------------------------
    // 5. ACTUALIZAR VISUALES
    // -----------------------------------------------------
    if (mDamageTimer > 0.0f) {
        mDamageTimer -= dt.asSeconds();
    }
    mSprite.setPosition(mPos);
    if (mFacingRight) mSprite.setScale(1.0f, 1.0f);
    else mSprite.setScale(-1.0f, 1.0f);
}

void Dodo::render(sf::RenderWindow& window, sf::Color ambientLight) {
    if (mDamageTimer > 0.0f) {
        mSprite.setColor(sf::Color::Red); // Parpadeo de daño
    } else {
        mSprite.setColor(ambientLight);   // Color normal/oscuridad
    }
    window.draw(mSprite);
}

bool Dodo::takeDamage(int amount, float knockbackDir) {
    // Si acaba de recibir daño, es invulnerable un instante
    if (mDamageTimer > 0.0f) return false;

    mHp -= amount;
    mDamageTimer = 0.4f; // 0.4 segundos de invulnerabilidad

    // Knockback (Empuje hacia arriba y hacia atrás)
    mVel.y = -200.0f;
    mVel.x = knockbackDir * 250.0f;

    return true; // Golpe exitoso
}