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
#include "Projectile.h"

enum class GameState {
    MainMenu,
    Playing,
    Paused
};

enum ItemID {
    AIR = 0,

    // Basic Blocks
    DIRT = 1,
    GRASS = 2,
    STONE = 3,
    WOOD = 4,
    LEAVES = 5,
    TORCH = 6,
    SAND = 13,
    SNOW = 14,

    // Minerals
    COAL = 7,
    COPPER = 8,
    IRON = 9,
    COBALT = 10,
    TUNGSTEN = 11,
    BEDROCK = 12,

    // Tools
    WOOD_PICKAXE = 21,
    STONE_PICKAXE = 22,
    IRON_PICKAXE = 23,
    TUNGSTEN_PICKAXE = 24,

    // Weapons (NEW!)
    WOOD_SWORD = 31,
    STONE_SWORD = 32,
    IRON_SWORD = 33,
    TUNGSTEN_SWORD = 34,
    BOW = 35,       // <--- NUEVO: Arco
    ARROW = 36,     // <--- NUEVO: Flecha

    // Structures and Doors
    DOOR = 25,
    DOOR_MID = 26,
    DOOR_TOP = 27,
    DOOR_OPEN = 28,
    DOOR_OPEN_MID = 29,
    DOOR_OPEN_TOP = 30,

    CRAFTING_TABLE = 40,
    FURNACE = 41,
    CHEST = 42,

    // Consumables
    MEAT = 50,

    // Ingots
    IRON_INGOT = 51,
    COPPER_INGOT = 52,
    COBALT_INGOT = 53,
    TUNGSTEN_INGOT = 54
};

struct Particle {
    sf::Vector2f position;
    sf::Vector2f velocity;
    sf::Color color;
    float lifetime;
    float maxLifetime;
    float size;
};

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
        bool requiresTable; // Does it require the crafting table to be made? <--- NEW

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

    // --- PROYECTILES ---
    std::vector<std::unique_ptr<Projectile>> mProjectiles;

    // --- INTERACTION AND MENU SYSTEM ---
    bool mIsCraftingTableOpen = false;
    bool mIsFurnaceOpen = false;

    // Function to be called upon Right Clicking in the world
    bool interactWithBlock(int gridX, int gridY); // <-- Now is a bool

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

    // --- HEALTH HUD (HEARTS) ---
    sf::Texture mHeartFullTex;
    sf::Texture mHeartEmptyTex;
    sf::Sprite mHeartSprite;

    // --- DEATH SYSTEM ---
    bool mIsPlayerDead;          // Is the player currently dead?
    float mRespawnTimer;         // Countdown (5, 4, 3...)

    sf::Text mDeathTitleText;    // Giant text "YOU DIED"
    sf::Text mDeathSubText;      // Small text "Respawning in..."
    sf::RectangleShape mDeathOverlay; // Semi-transparent red/black background

    // --- FURNACE SYSTEM ---
    sf::Texture mFurnaceBgTex;
    sf::Sprite mFurnaceBgSprite;
    sf::Texture mFurnaceFireTex;
    sf::Sprite mFurnaceFireSprite;
    sf::Texture mFurnaceArrowTex;
    sf::Sprite mFurnaceArrowSprite;

    // --- BLOCK ENTITY SYSTEM (FURNACES) ---
    struct FurnaceData {
        InventorySlot input;
        InventorySlot fuel;
        InventorySlot output;
        float fuelTimer = 0.0f;
        float maxFuelTimer = 0.0f;
        float smeltTimer = 0.0f;
    };

    // A map that saves furnace data using its (X, Y) coordinates
    std::map<std::pair<int, int>, FurnaceData> mActiveFurnaces;

    // To know which furnace is open on screen
    std::pair<int, int> mOpenFurnacePos;

    // Logic timers
    const float SMELT_TIME = 3.0f;     // Takes 3 seconds to smelt 1 ingot

    // --- CHEST SYSTEM ---
    struct ChestData {
        // A chest will have 24 slots (6 columns x 4 rows)
        std::vector<InventorySlot> slots;

        // Constructor to initialize it empty
        ChestData() {
            slots.resize(24);
        }
    };

    // Map to save all chests in the world
    std::map<std::pair<int, int>, ChestData> mActiveChests;

    // Interface state variables
    bool mIsChestOpen = false;
    std::pair<int, int> mOpenChestPos;

    // Helper function to reset the player
    void respawnPlayer();

    // --- DAY AND NIGHT CYCLE ---
    void updateDayNightCycle(float dtAsSeconds);
    const float DAY_LENGTH = 120.0f; // A day lasts 2 minutes for quick testing! (We will increase it later)

    void renderHUD();
    void renderMenus();
    void renderDeathScreen();

    void handleMouseClick(float mx, float my);
    void handleMouseRelease(float mx, float my);

    std::vector<Particle> mParticles;
    void spawnParticles(sf::Vector2f pos, int blockID, int count);

    GameState mGameState = GameState::MainMenu;

    // Menu texts
    sf::Text mMenuTitleText;
    sf::Text mMenuPlayText;
    sf::Text mMenuExitText;
    sf::Text mPauseTitleText;

    // Render functions to avoid cluttering the main code
    void renderMainMenu();
    void renderPauseMenu();
    };