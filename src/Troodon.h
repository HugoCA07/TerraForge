#pragma once
#include "Mob.h"

class Troodon : public Mob {
public:
    Troodon(sf::Vector2f startPos, const sf::Texture& texture);

    // Updates the Troodon's logic (AI, movement, collision)
    void update(sf::Time dt, sf::Vector2f playerPos, World& world, float lightLevel) override;
};