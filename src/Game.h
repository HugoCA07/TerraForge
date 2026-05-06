#pragma once

#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include "Player.h"
#include "World.h"
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

    // --- BLOQUES (1 - 99) ---
    DIRT = 1,
    STONE = 2,
    WOOD = 3,
    LEAVES = 4,
    TORCH = 5,
    SAND = 6,
    SNOW = 7,
    BEDROCK = 8,
    DOOR = 10,
    DOOR_MID = 11,
    DOOR_TOP = 12,
    DOOR_OPEN = 13,
    DOOR_OPEN_MID = 14,
    DOOR_OPEN_TOP = 15,
    CRAFTING_TABLE = 20,
    FURNACE = 21,
    CHEST = 22,

    // --- ARMAS (100 - 199) ---
    WOOD_SWORD = 100,
    STONE_SWORD = 101,
    IRON_SWORD = 102,
    TUNGSTEN_SWORD = 103,
    BOW = 110,
    ARROW = 111,

    // --- ÍTEMS USABLES / CONSUMIBLES (200 - 299) ---
    MEAT = 200,
    MEAT_MEDALLION = 201,

    // --- HERRAMIENTAS (300 - 399) ---
    WOOD_PICKAXE = 300,
    STONE_PICKAXE = 301,
    IRON_PICKAXE = 302,
    TUNGSTEN_PICKAXE = 303,

    // --- ARMADURAS (400 - 499) ---
    WOOD_HELMET = 400,
    WOOD_CHEST = 401,
    WOOD_LEGS = 402,
    WOOD_BOOTS = 403,

    // --- MATERIALES / MINERALES (500 - 599) ---
    COAL = 500,
    COPPER = 501,
    IRON = 502,
    COBALT = 503,
    TUNGSTEN = 504,
    COPPER_INGOT = 510,
    IRON_INGOT = 511,
    COBALT_INGOT = 512,
    TUNGSTEN_INGOT = 513,

    // --- PAREDES DE FONDO (600 - 699) ---
    BG_DIRT = 600,
    BG_STONE = 601
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

    // --- MAIN MENU SYSTEM ---
    enum class MenuScreen { Main, PlaySelect };
    MenuScreen mCurrentMenuScreen = MenuScreen::Main;

    sf::Text mMenuTitleText;

    // Textos del Menú Principal
    sf::Text mMenuPlayText;
    sf::Text mMenuSettingsText;
    sf::Text mMenuCreditsText;
    sf::Text mMenuExitText;

    // Textos del Submenú Jugar
    sf::Text mMenuNewGameText;
    sf::Text mMenuLoadGameText;
    sf::Text mMenuBackText;

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