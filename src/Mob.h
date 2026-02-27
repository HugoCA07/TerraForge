#pragma once
#include <SFML/Graphics.hpp>
#include "World.h"

class Mob {
public:
    Mob(sf::Vector2f startPos, const sf::Texture& texture, int maxHp);
    virtual ~Mob() = default; // Important virtual destructor

    // PURE VIRTUAL: We force the children to have their own version of update
    virtual void update(sf::Time dt, sf::Vector2f playerPos, World& world, float lightLevel) = 0;

    // Common methods (programmed once for everyone)
    void render(sf::RenderWindow& window, sf::Color ambientLight);
    bool takeDamage(int amount, float knockbackDir);
    bool isDead() const { return mHp <= 0; }
    
    sf::FloatRect getBounds() const { return mSprite.getGlobalBounds(); }
    sf::Vector2f getPosition() const { return mPos; }
    int getDamage() const { return mAttackDamage; }

protected: // "protected" means that children (Dodo, Troodon) can touch these variables
    sf::Vector2f mPos;
    sf::Vector2f mVel;
    sf::Sprite mSprite;
    
    bool mFacingRight;
    int mHp;
    float mDamageTimer;

    int mAttackDamage;
};