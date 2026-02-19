#pragma once

#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include "Player.h"
#include "World.h"
#include "Dodo.h"
#include <fstream>

class Game {
public:
    // Constructor and destructor
    Game();
    virtual ~Game();

    // Main method that runs the game loop
    void run();

    // Sky background elements
    sf::Texture mSkyTexture;
    sf::Sprite mSkySprite;

private:
    // Process window events and user input
    void processEvents();
    // Update game logic (physics, AI, etc.)
    void update(sf::Time dt);
    // Render all graphics to the window
    void render();

    // --- NUEVO: MÉTODOS DE GUARDADO ---
    void saveGame();
    void loadGame();

    // Core game variables
    sf::RenderWindow mWindow;
    Player mPlayer;
    World mWorld;

    // Player movement state (currently unused, input is checked directly)
    bool mIsMovingLeft = false;
    bool mIsMovingRight = false;
    // Variable to track the currently selected block for building
    int mSelectedBlock;
    std::map<int, int> mInventory;
    float mGameTime;
    sf::Color mAmbientLight;
    // --- TEXT AND FONTS (NEW) ---
    sf::Font mFont;
    sf::Text mUiText; // We will use this object to draw all numbers

    // --- SISTEMA DE MINERÍA ---
    sf::Vector2i mMiningPos;   // Coordenadas (X, Y) del bloque que estamos picando
    float mMiningTimer;        // Cuánto tiempo llevamos pulsando
    float mCurrentHardness;    // Cuánto tiempo necesita ese bloque para romperse
    sf::SoundBuffer mBufHit;
    sf::SoundBuffer mBufBreak;
    sf::SoundBuffer mBufBuild;

    sf::Sound mSndHit;
    sf::Sound mSndBreak;
    sf::Sound mSndBuild;

    sf::Texture mDodoTexture;
    std::vector<Dodo> mDodos;

    float mSoundTimer;
    };