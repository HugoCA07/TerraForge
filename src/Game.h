#pragma once

#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include "Player.h"
#include "World.h"
#include "Dodo.h"
#include "Troodon.h"
#include <memory> // NECESSARY FOR unique_ptr
#include <vector>
#include <utility>
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
    struct ItemInfo {
        std::string name;
        float weight;
        int maxStack; // How many fit in a single slot (e.g., 99 blocks, 1 pickaxe)
    };

    // An inventory slot
    struct InventorySlot {
        int id = 0;       // 0 means it's empty
        int count = 0;    // Quantity of the item
    };

    // --- CRAFTING SYSTEM ---
    struct Recipe {
        int resultId;       // The ID of the item we are going to craft (e.g., Iron Pickaxe)
        int resultCount;    // How many it gives us (usually 1)

        // A list of ingredients. Each ingredient is a pair {ID, Quantity}
        std::vector<std::pair<int, int>> ingredients;
    };

    // The complete list of all game recipes
    std::vector<Recipe> mRecipes;

    // Helper functions for crafting
    bool canCraft(const Recipe& recipe);
    void craftItem(const Recipe& recipe);

    // Process window events and user input
    void processEvents();
    // Update game logic (physics, AI, etc.)
    void update(sf::Time dt);
    // Render all graphics to the window
    void render();

    // --- NEW: SAVE METHODS ---
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
    int mActiveWheelSlot = 3; // 0=Usable, 1=Block, 2=Weapon 2, 3=Weapon 1 (Default)
    // --- NEW INVENTORY SYSTEM ---
    std::vector<InventorySlot> mBackpack; // The 30-slot backpack


    // The "Red Dead Redemption 2" system (Tactical Hotbar)
    InventorySlot mEquippedPrimary;   // Primary Weapon/Tool (e.g., Iron Pickaxe)
    InventorySlot mEquippedSecondary; // Secondary Weapon/Tool (e.g., Wood Pickaxe)
    InventorySlot mEquippedConsumable;// Food (e.g., Meat)
    InventorySlot mEquippedBlock;     // Active building block

    // State variables
    bool mIsInventoryOpen = false;
    float mCurrentWeight = 0.0f;
    const float mMaxWeight = 100.0f; // If you exceed 100, you move slowly

    // Dictionary to know the properties of each ID
    std::map<int, ItemInfo> mItemDatabase;

    // Helper functions we will have to program
    // New inventory functions
    bool addItemToBackpack(int id, int amount);
    void calculateTotalWeight();
    int getItemCount(int id);                  // Know how much we have of something
    bool consumeItem(int id, int amount = 1);  // Spend an item (returns true if possible)

    float mGameTime;
    sf::Color mAmbientLight;
    // --- TEXT AND FONTS (NEW) ---
    sf::Font mFont;
    sf::Text mUiText; // We will use this object to draw all numbers

    // --- MINING SYSTEM ---
    sf::Vector2i mMiningPos;   // Coordinates (X, Y) of the block we are mining
    float mMiningTimer;        // How long we have been pressing
    float mCurrentHardness;    // How much time that block needs to break
    sf::SoundBuffer mBufHit;
    sf::SoundBuffer mBufBreak;
    sf::SoundBuffer mBufBuild;

    sf::Sound mSndHit;
    sf::Sound mSndBreak;
    sf::Sound mSndBuild;

    float mActionTimer = 0.0f; // Timer for eating/building

    sf::Texture mDodoTexture;
    sf::Texture mTroodonTexture;

    // --- THE MASTER LIST OF ENTITIES ---
    std::vector<std::unique_ptr<Mob>> mMobs;

    float mSoundTimer;

    // Wheel Texture and Sprite
    sf::Texture mWheelTexture;
    sf::Sprite mWheelSprite;

    // --- DRAG & DROP SYSTEM ---
    InventorySlot mDraggedItem; // The item we are "floating" with the mouse
    int mDragSourceType = 0;    // 0 = None, 1 = Came from Backpack, 2 = Came from Wheel
    int mDragSourceIndex = -1;  // Exact slot it was in before picking it up

    // --- SPAWNER SYSTEM ---
    float mSpawnTimer;
    // Constant to not fill the world with monsters and lag the game
    const size_t MAX_MOBS = 10;
    };