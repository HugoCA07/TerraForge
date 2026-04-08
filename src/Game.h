#pragma once

#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include "Player.h"
#include "World.h"
#include "Dodo.h"
#include "Troodon.h"
#include <memory> // Necessary for unique_ptr
#include <vector>
#include <utility>
#include <fstream>
#include "Projectile.h"
#include "TRex.h"

/**
 * @enum GameState
 * @brief Represents the current state of the game loop.
 */
enum class GameState {
    MainMenu,
    IntroCinematic, // <--- NEW: Story intro phase
    Playing,
    Paused
};

/**
 * @enum ItemID
 * @brief Unique identifiers for blocks, items, tools, and weapons.
 */
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

    // Weapons
    WOOD_SWORD = 31,
    STONE_SWORD = 32,
    IRON_SWORD = 33,
    TUNGSTEN_SWORD = 34,
    BOW = 35,       // <--- NEW: Bow
    ARROW = 36,     // <--- NEW: Arrow

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
    TUNGSTEN_INGOT = 54,

    // --- NEW: ARMOR ---
    WOOD_HELMET = 61,
    WOOD_CHEST = 62,
    WOOD_LEGS = 63,
    WOOD_BOOTS = 64,

    MEAT_MEDALLION = 70
};

/**
 * @struct Particle
 * @brief Simple structure for a visual particle effect.
 */
struct Particle {
    sf::Vector2f position;
    sf::Vector2f velocity;
    sf::Color color;
    float lifetime;
    float maxLifetime;
    float size;
};

/**
 * @class Game
 * @brief Core game manager class that handles the game loop, rendering, logic, and state.
 */
class Game {
public:
    Game();
    virtual ~Game();

    /**
     * @brief Main method that runs the game loop.
     */
    void run();

    // Sky background elements
    sf::Texture mSkyTexture;
    sf::Sprite mSkySprite;

private:
    struct ItemInfo {
        std::string name;
        float weight;
        int maxStack; // Maximum quantity per slot (e.g., 99 blocks, 1 pickaxe)
    };

    /**
     * @struct InventorySlot
     * @brief Represents a single slot in the inventory.
     */
    struct InventorySlot {
        int id = 0;       // 0 means empty
        int count = 0;    // Quantity of the item
    };

    // --- CRAFTING SYSTEM ---
    struct Recipe {
        int resultId;       // Target item ID to craft
        int resultCount;    // Amount produced
        bool requiresTable; // Requires Crafting Table nearby

        // List of ingredients required. Each pair is {ID, Quantity}
        std::vector<std::pair<int, int>> ingredients;
    };

    // Complete list of available crafting recipes
    std::vector<Recipe> mRecipes;

    // Helper functions for crafting
    bool canCraft(const Recipe& recipe);
    void craftItem(const Recipe& recipe);

    // Event handling, logic update, and rendering phases
    void processEvents();
    void update(sf::Time dt);
    void render();

    // Save and load system
    void saveGame();
    void loadGame();

    // Core game components
    sf::RenderWindow mWindow;
    Player mPlayer;
    World mWorld;

    int mSelectedBlock;
    int mActiveWheelSlot = 3; // 0=Usable, 1=Block, 2=Weapon 2, 3=Weapon 1 (Default)

    // --- NEW INVENTORY SYSTEM ---
    std::vector<InventorySlot> mBackpack; // 30-slot main inventory

    // Tactical Hotbar (Quick slots)
    InventorySlot mEquippedPrimary;   // Primary Weapon/Tool
    InventorySlot mEquippedSecondary; // Secondary Weapon/Tool
    InventorySlot mEquippedConsumable;// Food
    InventorySlot mEquippedBlock;     // Active building block

    // --- NEW: DEFENSE WHEEL (4 Slots) ---
    InventorySlot mArmorHead;
    InventorySlot mArmorChest;
    InventorySlot mArmorLegs;
    InventorySlot mArmorBoots;

    // Toggle switch: true = Defense Wheel (Blue), false = Attack/Tactical Wheel (Red/Normal)
    bool mIsArmorWheelActive = false;

    // State variables
    bool mIsInventoryOpen = false;
    float mCurrentWeight = 0.0f;
    const float mMaxWeight = 100.0f; // Weight limit; exceeding it slows the player

    // Database mapping item IDs to their properties
    std::map<int, ItemInfo> mItemDatabase;

    // Inventory management helpers
    bool addItemToBackpack(int id, int amount);
    void calculateTotalWeight();
    int getItemCount(int id);
    bool consumeItem(int id, int amount = 1);

    float mGameTime;
    sf::Color mAmbientLight;

    // --- NEW: SMOOTH CAMERA ---
    sf::Vector2f mCameraPos;

    // UI and Fonts
    sf::Font mFont;
    sf::Text mUiText; // Reusable text object for rendering numbers/labels

    // --- MINING SYSTEM ---
    sf::Vector2i mMiningPos;   // Grid coordinates of the target block
    float mMiningTimer;        // Time spent holding the action
    float mCurrentHardness;    // Target block's required mining time

    sf::SoundBuffer mBufHit;
    sf::SoundBuffer mBufBreak;
    sf::SoundBuffer mBufBuild;

    sf::Sound mSndHit;
    sf::Sound mSndBreak;
    sf::Sound mSndBuild;

    float mActionTimer = 0.0f; // Cooldown timer for eating/building actions

    sf::Texture mDodoTexture;
    sf::Texture mTroodonTexture;
    sf::Texture mTRexTexture;

    // Dynamic list of active entities (enemies/animals)
    std::vector<std::unique_ptr<Mob>> mMobs;

    // Active projectiles
    std::vector<std::unique_ptr<Projectile>> mProjectiles;

    // --- INTERACTION AND MENU SYSTEM ---
    bool mIsCraftingTableOpen = false;
    bool mIsFurnaceOpen = false;

    // Triggered on right-clicking a block
    bool interactWithBlock(int gridX, int gridY);

    // Quick selection wheel UI
    sf::Texture mWheelTexture;
    sf::Sprite mWheelSprite;

    // --- DRAG & DROP SYSTEM ---
    InventorySlot mDraggedItem;

    // --- SPAWNER SYSTEM ---
    float mSpawnTimer;
    const size_t MAX_MOBS = 10; // Population limit

    // --- HEALTH HUD (HEARTS) ---
    sf::Texture mHeartFullTex;
    sf::Texture mHeartEmptyTex;
    sf::Sprite mHeartSprite;

    // --- DEATH SYSTEM ---
    bool mIsPlayerDead;
    float mRespawnTimer;         // Countdown timer before respawn

    sf::Text mDeathTitleText;
    sf::Text mDeathSubText;
    sf::RectangleShape mDeathOverlay;

    // --- FURNACE SYSTEM ---
    sf::Texture mFurnaceBgTex;
    sf::Sprite mFurnaceBgSprite;
    sf::Texture mFurnaceFireTex;
    sf::Sprite mFurnaceFireSprite;
    sf::Texture mFurnaceArrowTex;
    sf::Sprite mFurnaceArrowSprite;

    // State data for a single furnace
    struct FurnaceData {
        InventorySlot input;
        InventorySlot fuel;
        InventorySlot output;
        float fuelTimer = 0.0f;
        float maxFuelTimer = 0.0f;
        float smeltTimer = 0.0f;
    };

    // Dictionary of all active furnaces by coordinate
    std::map<std::pair<int, int>, FurnaceData> mActiveFurnaces;

    // Coordinates of the currently open furnace UI
    std::pair<int, int> mOpenFurnacePos;

    const float SMELT_TIME = 3.0f;     // Seconds required to smelt 1 item

    // --- CHEST SYSTEM ---
    struct ChestData {
        std::vector<InventorySlot> slots;
        ChestData() {
            slots.resize(24); // 6 columns x 4 rows
        }
    };

    std::map<std::pair<int, int>, ChestData> mActiveChests;

    bool mIsChestOpen = false;
    std::pair<int, int> mOpenChestPos;

    void respawnPlayer();

    // --- DAY AND NIGHT CYCLE ---
    void updateDayNightCycle(float dtAsSeconds);
    const float DAY_LENGTH = 120.0f; // Total day length in seconds

    void renderHUD();
    void renderMenus();
    void renderDeathScreen();

    void handleMouseClick(float mx, float my);
    void handleMouseRelease(float mx, float my);

    std::vector<Particle> mParticles;
    void spawnParticles(sf::Vector2f pos, int blockID, int count);

    GameState mGameState = GameState::MainMenu;

    // Main Menu Texts
    sf::Text mMenuTitleText;
    sf::Text mMenuPlayText;
    sf::Text mMenuExitText;
    sf::Text mPauseTitleText;

    // --- NEW: CINEMATIC SYSTEM ---
    float mCinematicTimer = 0.0f;
    int mCinematicPhase = 0; // 0 = Text 1, 1 = Text 2, 2 = Falling, 3 = Impact

    sf::Text mCinematicTextYear;
    sf::Text mCinematicTextPlanet;

    sf::Texture mCapsuleTexture;
    sf::Sprite mCapsuleSprite;
    sf::Vector2f mCapsulePos;
    sf::Vector2f mCapsuleVelocity;

    int mTotalDays = 1;
    float mShakeTimer = 0.0f;
};