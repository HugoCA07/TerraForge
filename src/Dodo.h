#pragma once
#include "Mob.h" // Include the parent

class Dodo : public Mob { // <--- INHERITANCE!
public:
    Dodo(sf::Vector2f startPos, const sf::Texture& texture);

    // We only need to declare what is unique to the Dodo: its brain
    void update(sf::Time dt, sf::Vector2f playerPos, World& world, float lightLevel) override;
};