#include "Game.h"
#include <cmath> // Necessary for std::sqrt
#include <iostream>
#include <algorithm> // For std::clamp, std::min, std::max

/**
 * @brief Constructor for the Game class.
 * Initializes the window, loads resources (textures, sounds, fonts),
 * sets up the initial inventory, and configures the UI and game states.
 */
Game::Game()
    : mWindow(sf::VideoMode(1920, 1080), "TerraForge C++")
    , mPlayer()
    , mWorld()
    , mSelectedBlock(1)
    , mGameTime(0.0f)
    , mAmbientLight(sf::Color::White)
    , mMiningPos(0, 0)
    , mMiningTimer(0.0f)
    , mCurrentHardness(0.0f)
    , mSpawnTimer(5.0f) // Start with 5 seconds so enemies spawn quickly at the beginning
{
    mWindow.setFramerateLimit(120);

    // --- INITIALIZE DEATH SYSTEM ---
    mIsPlayerDead = false;
    mRespawnTimer = 0.0f;

    // 1. Dark Background (Bloody tint)
    mDeathOverlay.setSize(sf::Vector2f(1920, 1080));
    mDeathOverlay.setFillColor(sf::Color(50, 0, 0, 150)); // Transparent dark red

    // 2. Title "YOU DIED"
    mDeathTitleText.setFont(mFont);
    mDeathTitleText.setString("HAS MUERTO");
    mDeathTitleText.setCharacterSize(100);
    mDeathTitleText.setFillColor(sf::Color::Red);
    mDeathTitleText.setOutlineColor(sf::Color::Black);
    mDeathTitleText.setOutlineThickness(5.0f);

    // Center the text
    sf::FloatRect titleBounds = mDeathTitleText.getLocalBounds();
    mDeathTitleText.setOrigin(titleBounds.width / 2.0f, titleBounds.height / 2.0f);
    mDeathTitleText.setPosition(1920 / 2.0f, 1080 / 2.0f - 50.0f);

    // 3. Subtitle "Respawning..."
    mDeathSubText.setFont(mFont);
    mDeathSubText.setCharacterSize(40);
    mDeathSubText.setFillColor(sf::Color::White);
    mDeathSubText.setOutlineColor(sf::Color::Black);
    mDeathSubText.setOutlineThickness(2.0f);

    // --- LOAD SOUNDS ---
    // 1. HIT (Weapon impact)
    if (mBufHit.loadFromFile("assets/Hit.wav")) {
        mSndHit.setBuffer(mBufHit);
        mSndHit.setVolume(50.0f);
        mSndHit.setPitch(1.0f); // Random pitch variation applied later to avoid sounding robotic
    }

    // 2. BREAK (Block destroyed)
    if (mBufBreak.loadFromFile("assets/Break.wav")) {
        mSndBreak.setBuffer(mBufBreak);
        mSndBreak.setVolume(70.0f);
    }

    // 3. BUILD (Block placed)
    if (mBufBuild.loadFromFile("assets/Build.wav")) {
        mSndBuild.setBuffer(mBufBuild);
        mSndBuild.setVolume(60.0f);
    }

    // --- LOAD SKY ---
    if (!mSkyTexture.loadFromFile("assets/Sky.png")) {
        std::cerr << "Error: Could not load Sky.png" << std::endl;
    } else {
        mSkySprite.setTexture(mSkyTexture);
    }

    // --- LOAD FONT ---
    if (!mFont.loadFromFile("assets/font.ttf")) {
        std::cerr << "Error: Could not load assets/font.ttf" << std::endl;
    }

    // --- LOAD HUD (Health Hearts) ---
    if (!mHeartFullTex.loadFromFile("assets/HeartFull.png")) {
        std::cerr << "Error: Could not load HeartFull.png" << std::endl;
    }
    if (!mHeartEmptyTex.loadFromFile("assets/HeartEmpty.png")) {
        std::cerr << "Error: Could not load HeartEmpty.png" << std::endl;
    }

    // --- LOAD FURNACE UI ---
    if (!mFurnaceBgTex.loadFromFile("assets/FurnaceUI.png")) { std::cerr << "Error: FurnaceUI.png\n"; }
    if (!mFurnaceFireTex.loadFromFile("assets/FurnaceFire.png")) { std::cerr << "Error: FurnaceFire.png\n"; }
    if (!mFurnaceArrowTex.loadFromFile("assets/FurnaceArrow.png")) { std::cerr << "Error: FurnaceArrow.png\n"; }

    mFurnaceBgSprite.setTexture(mFurnaceBgTex);
    mFurnaceFireSprite.setTexture(mFurnaceFireTex);
    mFurnaceArrowSprite.setTexture(mFurnaceArrowTex);

    // Base UI text configuration
    mUiText.setFont(mFont);
    mUiText.setCharacterSize(20);
    mUiText.setFillColor(sf::Color::White);
    mUiText.setOutlineColor(sf::Color::Black);
    mUiText.setOutlineThickness(1.0f);

    // --- GENERATE SOFT LIGHT TEXTURE (Radial Gradient) ---
    sf::Image lightImg;
    lightImg.create(256, 256, sf::Color::Transparent);

    for (int y = 0; y < 256; ++y) {
        for (int x = 0; x < 256; ++x) {
            float dx = x - 128.0f;
            float dy = y - 128.0f;
            float distance = std::sqrt(dx * dx + dy * dy);
            float radius = 128.0f;

            if (distance < radius) {
                // Calculate intensity (1.0 at center, 0.0 at edge)
                float intensity = 1.0f - (distance / radius);
                // Use power function for a non-linear, natural light falloff
                intensity = std::pow(intensity, 1.5f);
                lightImg.setPixel(x, y, sf::Color(255, 255, 255, static_cast<sf::Uint8>(255 * intensity)));
            }
        }
    }

    // --- PREPARE INVENTORY ---
    mBackpack.resize(30); // Initialize 30 empty slots

    // --- CRAFTING RECIPES ---
    // MANUAL RECIPES (RequiresTable = false)
    mRecipes.push_back({ItemID::WOOD_PICKAXE, 1, false, {{ItemID::WOOD, 10}}});
    mRecipes.push_back({ItemID::WOOD_SWORD, 1, false, {{ItemID::WOOD, 7}}});
    mRecipes.push_back({ItemID::TORCH, 5, false, {{ItemID::WOOD, 2}, {ItemID::LEAVES, 2}}});
    mRecipes.push_back({ItemID::CRAFTING_TABLE, 1, false, {{ItemID::WOOD, 10}}});

    // ADVANCED RECIPES (RequiresTable = true)
    mRecipes.push_back({ItemID::STONE_PICKAXE, 1, true, {{ItemID::WOOD, 5}, {ItemID::STONE, 5}}});
    mRecipes.push_back({ItemID::IRON_PICKAXE, 1, true, {{ItemID::IRON_INGOT, 5}, {ItemID::WOOD, 5}}});
    mRecipes.push_back({ItemID::TUNGSTEN_PICKAXE, 1, true, {{ItemID::TUNGSTEN_INGOT, 5}, {ItemID::WOOD, 5}}});
    mRecipes.push_back({ItemID::STONE_SWORD, 1, true, {{ItemID::WOOD, 2}, {ItemID::STONE, 6}}});
    mRecipes.push_back({ItemID::IRON_SWORD, 1, true, {{ItemID::WOOD, 2}, {ItemID::IRON_INGOT, 6}}});
    mRecipes.push_back({ItemID::TUNGSTEN_SWORD, 1, true, {{ItemID::WOOD, 2}, {ItemID::TUNGSTEN_INGOT, 6}}});
    mRecipes.push_back({ItemID::DOOR, 1, true, {{ItemID::WOOD, 6}}});
    mRecipes.push_back({ItemID::FURNACE, 1, true, {{ItemID::STONE, 10}}});
    mRecipes.push_back({ItemID::CHEST, 1, true, {{ItemID::IRON_INGOT, 2}, {ItemID::WOOD, 5}}});
    mRecipes.push_back({ItemID::BOW, 1, true, {{ItemID::WOOD, 5}}});
    mRecipes.push_back({ItemID::ARROW, 10, true, {{ItemID::WOOD, 5},{ItemID::STONE,5}}});

    // Special Recipe: Boss Summoning Item
    mRecipes.push_back({ItemID::MEAT_MEDALLION, 1, true, {{ItemID::MEAT, 30}}});

    // --- ITEM DATABASE (Defines weight and max stack for each item) ---
    // Blocks
    mItemDatabase[ItemID::DIRT] = {"Dirt", 1.0f, 99};
    mItemDatabase[ItemID::GRASS] = {"Grass", 1.0f, 99};
    mItemDatabase[ItemID::STONE] = {"Stone", 2.0f, 99};
    mItemDatabase[ItemID::WOOD] = {"Log", 1.5f, 99};
    mItemDatabase[ItemID::LEAVES] = {"Leaves", 0.1f, 99};
    mItemDatabase[ItemID::TORCH] = {"Torch", 0.2f, 99};
    mItemDatabase[ItemID::SAND] = {"Sand", 1.0f, 99};
    mItemDatabase[ItemID::SNOW] = {"Snow", 1.0f, 99};

    // Minerals
    mItemDatabase[ItemID::COAL] = {"Coal", 1.0f, 99};
    mItemDatabase[ItemID::COPPER] = {"Copper", 2.0f, 99};
    mItemDatabase[ItemID::IRON] = {"Iron", 2.0f, 99};
    mItemDatabase[ItemID::COBALT] = {"Cobalt", 2.0f, 99};
    mItemDatabase[ItemID::TUNGSTEN] = {"Tungsten", 2.0f, 99};

    // Tools and Consumables
    mItemDatabase[ItemID::WOOD_PICKAXE] = {"Wood Pickaxe", 5.0f, 1};
    mItemDatabase[ItemID::STONE_PICKAXE] = {"Stone Pickaxe", 5.0f, 1};
    mItemDatabase[ItemID::IRON_PICKAXE] = {"Iron Pickaxe", 5.0f, 1};
    mItemDatabase[ItemID::TUNGSTEN_PICKAXE] = {"Tungsten Pickaxe", 5.0f, 1};
    mItemDatabase[ItemID::MEAT] = {"Meat", 0.5f, 20};

    // Weapons
    mItemDatabase[ItemID::WOOD_SWORD] = {"Wood Sword", 4.0f, 1};
    mItemDatabase[ItemID::STONE_SWORD] = {"Stone Sword", 4.0f, 1};
    mItemDatabase[ItemID::IRON_SWORD] = {"Iron Sword", 4.0f, 1};
    mItemDatabase[ItemID::TUNGSTEN_SWORD] = {"Tungsten Sword", 4.0f, 1};
    mItemDatabase[ItemID::BOW] = {"Bow", 3.0f, 1};
    mItemDatabase[ItemID::ARROW] = {"Arrow", 0.1f, 99};

    // Structures
    mItemDatabase[ItemID::DOOR] = {"Wooden Door", 5.0f, 99};
    mItemDatabase[ItemID::CRAFTING_TABLE] = {"Crafting Table", 3.0f, 99};
    mItemDatabase[ItemID::FURNACE] = {"Furnace", 4.0f, 99};
    mItemDatabase[ItemID::CHEST] = {"Chest", 4.0f, 99};

    // Ingots
    mItemDatabase[ItemID::IRON_INGOT] = {"Iron Ingot", 1.5f, 99};
    mItemDatabase[ItemID::COPPER_INGOT] = {"Copper Ingot", 1.5f, 99};
    mItemDatabase[ItemID::COBALT_INGOT] = {"Cobalt Ingot", 1.5f, 99};
    mItemDatabase[ItemID::TUNGSTEN_INGOT] = {"Tungsten Ingot", 1.5f, 99};

    // Armor
    mItemDatabase[ItemID::WOOD_HELMET] = {"Wood Helmet", 2.0f, 1};
    mItemDatabase[ItemID::WOOD_CHEST]  = {"Wood Chest", 4.0f, 1};
    mItemDatabase[ItemID::WOOD_LEGS]   = {"Wood Legs", 3.0f, 1};
    mItemDatabase[ItemID::WOOD_BOOTS]  = {"Wood Boots", 1.5f, 1};

    // Special
    mItemDatabase[ItemID::MEAT_MEDALLION] = {"Meat Medallion", 10.0f, 1};

    // Give starting items for testing
    addItemToBackpack(CHEST, 1);
    addItemToBackpack(IRON, 14);

    // --- LOAD ENTITY TEXTURES ---
    if (!mDodoTexture.loadFromFile("assets/Dodo.png")) std::cerr << "Error: Missing Dodo.png" << std::endl;
    if (!mTroodonTexture.loadFromFile("assets/Troodon.png")) std::cerr << "Error: Missing Troodon.png" << std::endl;
    if (!mTRexTexture.loadFromFile("assets/TRex.png")) std::cerr << "Error: Missing TRex.png" << std::endl;

    if (!mWheelTexture.loadFromFile("assets/WheelGun.png")) {
        std::cerr << "Error: Missing WheelGun.png" << std::endl;
    } else {
        mWheelSprite.setTexture(mWheelTexture);
        mWheelSprite.setOrigin(mWheelTexture.getSize().x / 2.0f, mWheelTexture.getSize().y / 2.0f);
        mWheelSprite.setScale(2.0f, 2.0f);
    }

    // --- UI TEXT CONFIGURATION ---
    mMenuTitleText.setFont(mFont);
    mMenuTitleText.setString("TERRAFORGE");
    mMenuTitleText.setCharacterSize(120);
    mMenuTitleText.setFillColor(sf::Color::Yellow);
    mMenuTitleText.setOutlineColor(sf::Color::Black);
    mMenuTitleText.setOutlineThickness(6.0f);
    sf::FloatRect titleBoundsMenu = mMenuTitleText.getLocalBounds();
    mMenuTitleText.setOrigin(titleBoundsMenu.width / 2.0f, titleBoundsMenu.height / 2.0f);
    mMenuTitleText.setPosition(1920 / 2.0f, 300.0f);

    mMenuPlayText.setFont(mFont);
    mMenuPlayText.setString("> PLAY <");
    mMenuPlayText.setCharacterSize(60);
    mMenuPlayText.setFillColor(sf::Color::White);
    mMenuPlayText.setOutlineColor(sf::Color::Black);
    mMenuPlayText.setOutlineThickness(3.0f);
    sf::FloatRect playBounds = mMenuPlayText.getLocalBounds();
    mMenuPlayText.setOrigin(playBounds.width / 2.0f, playBounds.height / 2.0f);
    mMenuPlayText.setPosition(1920 / 2.0f, 600.0f);

    mMenuExitText.setFont(mFont);
    mMenuExitText.setString("> EXIT <");
    mMenuExitText.setCharacterSize(60);
    mMenuExitText.setFillColor(sf::Color::White);
    mMenuExitText.setOutlineColor(sf::Color::Black);
    mMenuExitText.setOutlineThickness(3.0f);
    sf::FloatRect exitBounds = mMenuExitText.getLocalBounds();
    mMenuExitText.setOrigin(exitBounds.width / 2.0f, exitBounds.height / 2.0f);
    mMenuExitText.setPosition(1920 / 2.0f, 750.0f);

    // Pause Menu Text
    mPauseTitleText = mDeathTitleText;
    mPauseTitleText.setString("PAUSED");
    mPauseTitleText.setFillColor(sf::Color::White);
    sf::FloatRect pauseBounds = mPauseTitleText.getLocalBounds();
    mPauseTitleText.setOrigin(pauseBounds.width / 2.0f, pauseBounds.height / 2.0f);

    // --- CINEMATIC TEXTS ---
    mCinematicTextYear.setFont(mFont);
    mCinematicTextYear.setString("Year 3808...");
    mCinematicTextYear.setCharacterSize(60);
    mCinematicTextYear.setFillColor(sf::Color(0, 255, 0, 0)); // Starts fully transparent

    mCinematicTextPlanet.setFont(mFont);
    mCinematicTextPlanet.setString("Planet XTR-0051");
    mCinematicTextPlanet.setCharacterSize(80);
    mCinematicTextPlanet.setFillColor(sf::Color(0, 255, 0, 0));

    sf::FloatRect yearBounds = mCinematicTextYear.getLocalBounds();
    mCinematicTextYear.setOrigin(yearBounds.width / 2.0f, yearBounds.height / 2.0f);
    mCinematicTextYear.setPosition(1920 / 2.0f, 1080 / 2.0f - 50.0f);

    sf::FloatRect planetBounds = mCinematicTextPlanet.getLocalBounds();
    mCinematicTextPlanet.setOrigin(planetBounds.width / 2.0f, planetBounds.height / 2.0f);
    mCinematicTextPlanet.setPosition(1920 / 2.0f, 1080 / 2.0f + 50.0f);

    // --- LOAD CAPSULE ASSET ---
    if (!mCapsuleTexture.loadFromFile("assets/Capsule.png")) {
        std::cerr << "Error: Missing Capsule.png" << std::endl;
    } else {
        mCapsuleSprite.setTexture(mCapsuleTexture);
        mCapsuleSprite.setOrigin(mCapsuleTexture.getSize().x / 2.0f, mCapsuleTexture.getSize().y / 2.0f);
        mCapsuleSprite.setScale(1.5f, 1.5f);
    }
}

Game::~Game() {}

/**
 * @brief Main game loop.
 * Calculates delta time and delegates to processing, updating, and rendering functions.
 */
void Game::run() {
    sf::Clock clock;
    while (mWindow.isOpen()) {
        sf::Time dt = clock.restart();
        // Limit delta time to prevent physics glitches during lag spikes (max 20 TPS compensation)
        if (dt.asSeconds() > 0.05f) {
            dt = sf::seconds(0.05f);
        }
        processEvents();
        update(dt);
        render();
    }
}

/**
 * @brief Processes OS events (window close, keyboard, mouse).
 * Handles menu navigation, UI toggling, and dragging items.
 */
void Game::processEvents() {
    sf::Event event;
    while (mWindow.pollEvent(event)) {
        if (event.type == sf::Event::Closed)
            mWindow.close();

        // --- ESCAPE KEY LOGIC ---
        if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape) {
            if (mGameState == GameState::Playing) {
                // Prioritize closing open UI panels before pausing the game
                if (mIsInventoryOpen || mIsFurnaceOpen || mIsChestOpen || mIsCraftingTableOpen) {
                    mIsInventoryOpen = false;
                    mIsFurnaceOpen = false;
                    mIsChestOpen = false;
                    mIsCraftingTableOpen = false;

                    // Return dragged item to inventory to prevent item loss/duplication
                    if (mDraggedItem.id != ItemID::AIR) {
                        addItemToBackpack(mDraggedItem.id, mDraggedItem.count);
                        mDraggedItem.id = ItemID::AIR;
                        mDraggedItem.count = 0;
                    }
                } else {
                    mGameState = GameState::Paused;
                }
            } else if (mGameState == GameState::Paused) {
                mGameState = GameState::Playing;
            } else if (mGameState == GameState::MainMenu) {
                mWindow.close();
            }
        }

        // --- MAIN MENU INTERACTION ---
        if (mGameState == GameState::MainMenu && event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
            sf::Vector2i mousePos = sf::Mouse::getPosition(mWindow);
            sf::Vector2f worldPos = mWindow.mapPixelToCoords(mousePos, mWindow.getDefaultView());

            if (mMenuPlayText.getGlobalBounds().contains(worldPos)) {
                // Initiate Cinematic Sequence
                mGameState = GameState::IntroCinematic;
                mCinematicTimer = 0.0f;
                mCinematicPhase = 0;

                // Spawn capsule high up and far left for a diagonal meteor crash
                float finalX = 100 * mWorld.getTileSize();
                mCapsulePos = sf::Vector2f(finalX - 3000.0f, -3000.0f);
                mCameraPos = mCapsulePos; // Force camera to follow the capsule
            }
            if (mMenuExitText.getGlobalBounds().contains(worldPos)) mWindow.close();
        }

        // --- PAUSE MENU INTERACTION ---
        if (mGameState == GameState::Paused && event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
            sf::Vector2i mousePos = sf::Mouse::getPosition(mWindow);
            sf::Vector2f worldPos = mWindow.mapPixelToCoords(mousePos, mWindow.getDefaultView());

            sf::FloatRect exitBounds = mMenuExitText.getGlobalBounds();
            exitBounds.top = 650.0f - (exitBounds.height / 2.0f); // Simulated Y pos matching render

            if (exitBounds.contains(worldPos)) {
                mGameState = GameState::MainMenu;
            }
        }

        // --- INVENTORY TOGGLE (E Key) ---
        if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::E) {
            mIsInventoryOpen = !mIsInventoryOpen;
            if (!mIsInventoryOpen) {
                mIsChestOpen = false;
                mIsFurnaceOpen = false;
                mIsCraftingTableOpen = false;

                if (mDraggedItem.id != ItemID::AIR) {
                    addItemToBackpack(mDraggedItem.id, mDraggedItem.count);
                    mDraggedItem.id = ItemID::AIR;
                    mDraggedItem.count = 0;
                }
            }
        }

        // --- WHEEL TOGGLE (Q Key) ---
        // Switches between Attack/Build Wheel and Armor Wheel
        if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Q) {
            if (mIsInventoryOpen) {
                mIsArmorWheelActive = !mIsArmorWheelActive;
            }
        }

        // --- WINDOW RESIZING ---
        if (event.type == sf::Event::Resized) {
            sf::FloatRect visibleArea(0.0f, 0.0f, static_cast<float>(event.size.width), static_cast<float>(event.size.height));
            mWindow.setView(sf::View(visibleArea));
        }

        // --- DRAG & DROP UI INTERACTIONS ---
        if (mGameState == GameState::Playing && mIsInventoryOpen) {
            if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
                sf::Vector2i pixelPos(event.mouseButton.x, event.mouseButton.y);
                sf::View uiView(sf::FloatRect(0.0f, 0.0f, static_cast<float>(mWindow.getSize().x), static_cast<float>(mWindow.getSize().y)));
                sf::Vector2f worldPos = mWindow.mapPixelToCoords(pixelPos, uiView);
                handleMouseClick(worldPos.x, worldPos.y);
            }
            else if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left) {
                sf::Vector2i pixelPos(event.mouseButton.x, event.mouseButton.y);
                sf::View uiView(sf::FloatRect(0.0f, 0.0f, static_cast<float>(mWindow.getSize().x), static_cast<float>(mWindow.getSize().y)));
                sf::Vector2f worldPos = mWindow.mapPixelToCoords(pixelPos, uiView);
                handleMouseRelease(worldPos.x, worldPos.y);
            }
        }
    }

    // Only process player movement if the game is active
    if (mGameState == GameState::Playing) {
        mPlayer.handleInput(mIsInventoryOpen);
    }
}

/**
 * @brief Helper function to determine the base mining time required for a block.
 */
float getBlockHardness(int id) {
    switch (id) {
        case ItemID::AIR: return 0.0f;
        case ItemID::DIRT: return 0.2f;  // Fast break
        case ItemID::GRASS: return 0.25f;
        case ItemID::WOOD: return 0.6f;
        case ItemID::LEAVES: return 0.1f;  // Instant break
        case ItemID::TORCH: return 0.1f;

        // Doors
        case ItemID::DOOR: case ItemID::DOOR_MID: case ItemID::DOOR_TOP:
        case ItemID::DOOR_OPEN: case ItemID::DOOR_OPEN_MID: case ItemID::DOOR_OPEN_TOP:
            return 0.5f;

        // Interactive Blocks
        case ItemID::CRAFTING_TABLE: return 1.5f;
        case ItemID::FURNACE: return 2.0f; // Hard stone structure
        case ItemID::CHEST: return 1.5f;

        // Stone and Minerals
        case ItemID::STONE: return 1.0f;
        case ItemID::COAL: return 1.2f;
        case ItemID::COPPER: return 1.5f;
        case ItemID::IRON: return 2.0f;
        case ItemID::COBALT: return 3.0f;
        case ItemID::TUNGSTEN: return 5.0f;

        case ItemID::BEDROCK: return -1.0f; // Indestructible

        default: return 1.0f;
    }
}

/**
 * @brief Tool Tier Progression. Determines the mining level of a given tool.
 */
int getPickaxeTier(int itemID) {
    switch (itemID) {
        case ItemID::WOOD_PICKAXE: return 1;
        case ItemID::STONE_PICKAXE: return 2;
        case ItemID::IRON_PICKAXE: return 3;
        case ItemID::TUNGSTEN_PICKAXE: return 4;
        default: return 0; // Fists or unrelated items
    }
}

/**
 * @brief Determines the minimum tool tier required to drop the item block.
 */
int getRequiredPickaxeTier(int blockID) {
    switch (blockID) {
        // Tier 1 (Wood Pickaxe+)
        case ItemID::STONE:
        case ItemID::COAL:
        case ItemID::FURNACE: return 1;

        // Tier 2 (Stone Pickaxe+)
        case ItemID::COPPER:
        case ItemID::IRON: return 2;

        // Tier 3 (Iron Pickaxe+)
        case ItemID::COBALT:
        case ItemID::TUNGSTEN: return 3;

        case ItemID::BEDROCK: return 999;

        // Dirt, wood, leaves, etc. can be broken by hand (Tier 0)
        default: return 0;
    }
}

/**
 * @brief Main logic update loop.
 * Handles the cinematic sequence, day/night cycle, physics, entity updates,
 * mining, block placing, furnace cooking, combat, and camera movement.
 * @param dt The delta time for current frame.
 */
void Game::update(sf::Time dt) {
    // ==================================================
    // CINEMATIC DIRECTOR (Intro Sequence)
    // ==================================================
    if (mGameState == GameState::IntroCinematic) {
        mCinematicTimer += dt.asSeconds();

        // PHASE 0: Fade in Year Text
        if (mCinematicPhase == 0) {
            float alpha = std::clamp((mCinematicTimer / 2.0f) * 255.0f, 0.0f, 255.0f);
            mCinematicTextYear.setFillColor(sf::Color(0, 255, 0, static_cast<sf::Uint8>(alpha)));

            if (mCinematicTimer > 3.0f) {
                mCinematicPhase = 1;
                mCinematicTimer = 0.0f;
            }
        }
        // PHASE 1: Fade in Planet Text
        else if (mCinematicPhase == 1) {
            float alpha = std::clamp((mCinematicTimer / 2.0f) * 255.0f, 0.0f, 255.0f);
            mCinematicTextPlanet.setFillColor(sf::Color(0, 255, 0, static_cast<sf::Uint8>(alpha)));

            if (mCinematicTimer > 4.0f) {
                mCinematicPhase = 2;
                mCinematicTimer = 0.0f;
            }
        }
        // PHASE 2: Capsule Freefall & Crash
        else if (mCinematicPhase == 2) {
            // Meteor velocity
            mCapsulePos.x += 2000.0f * dt.asSeconds();
            mCapsulePos.y += 2000.0f * dt.asSeconds();

            mCameraPos = mCapsulePos; // Camera tracks the drop

            // Leave a fiery trail (using Torch ID colors)
            spawnParticles(mCapsulePos, ItemID::TORCH, 4);

            // Ground collision detection (Checking ahead of the capsule tip)
            int gridX = static_cast<int>((mCapsulePos.x + 20.0f) / mWorld.getTileSize());
            int gridY = static_cast<int>((mCapsulePos.y + 20.0f) / mWorld.getTileSize());

            if (World::isSolid(mWorld.getBlock(gridX, gridY))) {
                // MASSIVE IMPACT: Carve out a crater
                int craterRadius = 3;

                for (int ox = -craterRadius; ox <= craterRadius; ++ox) {
                    for (int oy = -craterRadius; oy <= craterRadius; ++oy) {
                        // Round out the edges
                        if (std::abs(ox) == craterRadius && std::abs(oy) == craterRadius) continue;
                        mWorld.setBlock(gridX + ox, gridY + oy, 0);
                    }
                }

                // Explosion Effects
                spawnParticles(mCapsulePos, ItemID::DIRT, 150);
                spawnParticles(mCapsulePos, ItemID::TORCH, 100);

                mSndBreak.setPitch(0.15f); // Deep impact sound
                mSndBreak.play();

                // Position player at the bottom of the crater
                mPlayer.setPosition(sf::Vector2f(mCapsulePos.x, mCapsulePos.y - mWorld.getTileSize()));
                mPlayer.setHp(100);

                mGameState = GameState::Playing; // Start normal gameplay
            }
        }

        // Apply cinematic camera view
        sf::View view = mWindow.getDefaultView();
        view.zoom(1.25f);
        view.setCenter(mCameraPos);
        mWindow.setView(view);

        return; // Halt normal game logic during cinematic
    }

    // Block logic updates if not currently playing (e.g. Menu, Paused)
    if (mGameState != GameState::Playing) return;

    updateDayNightCycle(dt.asSeconds());

    // ==================================================
    // DEATH STATE LOGIC
    // ==================================================
    if (mIsPlayerDead) {
        mRespawnTimer -= dt.asSeconds();
        int secondsLeft = static_cast<int>(std::ceil(mRespawnTimer));
        mDeathSubText.setString("Respawning in " + std::to_string(secondsLeft) + " seconds...");

        if (mRespawnTimer <= 0.0f) {
            respawnPlayer();
        }
        return; // Prevent player interactions while dead
    }

    // CHECK FOR DEATH
    if (mPlayer.getHp() <= 0) {
        mIsPlayerDead = true;
        mRespawnTimer = 5.0f;

        // Death Penalty: Lose half your meat
        int meatCount = getItemCount(50);
        if (meatCount > 0) {
            int meatToLose = meatCount / 2;
            if (meatToLose > 0) {
                consumeItem(50, meatToLose);
                std::cout << "You died and lost " << meatToLose << " pieces of meat." << std::endl;
            }
        }

        mSndBreak.setPitch(0.4f);
        mSndBreak.play();
        return;
    }

    // ==================================================
    // FURNACE PROCESSING LOGIC
    // ==================================================
    for (auto& pair : mActiveFurnaces) {
        FurnaceData& fd = pair.second;

        // Map input ore to output ingot
        int resultItem = 0;
        if (fd.input.id == ItemID::COPPER) resultItem = ItemID::COPPER_INGOT;
        else if (fd.input.id == ItemID::IRON) resultItem = ItemID::IRON_INGOT;
        else if (fd.input.id == ItemID::COBALT) resultItem = ItemID::COBALT_INGOT;
        else if (fd.input.id == ItemID::TUNGSTEN) resultItem = ItemID::TUNGSTEN_INGOT;

        // Validate smelting conditions (valid ore + space in output)
        bool canCook = (resultItem != 0 && fd.input.count > 0 &&
                       (fd.output.id == 0 || (fd.output.id == resultItem && fd.output.count < 99)));

        // Consume fuel if needed
        if (canCook && fd.fuelTimer <= 0.0f) {
            if ((fd.fuel.id == ItemID::WOOD || fd.fuel.id == ItemID::COAL) && fd.fuel.count > 0) {
                int fuelType = fd.fuel.id;
                fd.fuel.count--;
                if (fd.fuel.count == 0) fd.fuel.id = 0;

                fd.maxFuelTimer = (fuelType == ItemID::COAL) ? 40.0f : 10.0f;
                fd.fuelTimer = fd.maxFuelTimer;
            }
        }

        // Process smelting
        if (fd.fuelTimer > 0.0f) {
            fd.fuelTimer -= dt.asSeconds();
            if (fd.fuelTimer < 0.0f) fd.fuelTimer = 0.0f;

            if (canCook) {
                fd.smeltTimer += dt.asSeconds();
                if (fd.smeltTimer >= SMELT_TIME) {
                    fd.input.count--;
                    if (fd.input.count == 0) fd.input.id = 0;

                    fd.output.id = resultItem;
                    fd.output.count++;
                    fd.smeltTimer = 0.0f;
                }
            } else {
                fd.smeltTimer = 0.0f; // Pause progress if ore is removed or output is full
            }
        } else {
            fd.smeltTimer = 0.0f; // Pause if fire goes out
        }
    }

    // Update Player Armor visuals
    mPlayer.setArmorAnimTextures(
        mWorld.getArmorAnimTexture(mArmorHead.id),
        mWorld.getArmorAnimTexture(mArmorChest.id),
        mWorld.getArmorAnimTexture(mArmorLegs.id),
        mWorld.getArmorAnimTexture(mArmorBoots.id)
    );

    mPlayer.update(dt, mWorld);

    // ==================================================
    // DISTANCE CHECK: AUTO-CLOSE UI (Bluetooth Prevention)
    // ==================================================
    if (mIsChestOpen || mIsFurnaceOpen || mIsCraftingTableOpen) {
        sf::Vector2f playerCenter = mPlayer.getCenter();
        float pGridX = playerCenter.x / mWorld.getTileSize();
        float pGridY = playerCenter.y / mWorld.getTileSize();

        if (mIsChestOpen || mIsFurnaceOpen) {
            int targetX = mIsChestOpen ? mOpenChestPos.first : mOpenFurnacePos.first;
            int targetY = mIsChestOpen ? mOpenChestPos.second : mOpenFurnacePos.second;

            float dx = pGridX - targetX;
            float dy = pGridY - targetY;
            float distance = std::sqrt(dx * dx + dy * dy);

            // Close UI if player moves further than 5 blocks
            if (distance > 5.0f) {
                mIsChestOpen = false;
                mIsFurnaceOpen = false;
                mIsInventoryOpen = false;

                // Return dragged items
                if (mDraggedItem.id != ItemID::AIR) {
                    addItemToBackpack(mDraggedItem.id, mDraggedItem.count);
                    mDraggedItem.id = ItemID::AIR;
                    mDraggedItem.count = 0;
                }
                std::cout << "[UI] Moved too far away. Interface closed." << std::endl;
            }
        }
    }

    // Weight Penalty System
    calculateTotalWeight();
    mPlayer.setOverweight(mCurrentWeight > mMaxWeight);

    // Update player hand visual based on selected wheel slot
    mPlayer.setEquippedWeapon(mSelectedBlock);

    // --- ITEM PICKUP SYSTEM ---
    std::map<int, int> pickedUpItems;
    mWorld.update(dt, mPlayer.getCenter(), pickedUpItems); // World detects collisions with dropped items

    for (const auto& item : pickedUpItems) {
        int id = item.first;
        int cantidad = item.second;

        // If inventory is full, drop it back onto the ground
        if (!addItemToBackpack(id, cantidad)) {
            int pGridX = static_cast<int>(mPlayer.getPosition().x / mWorld.getTileSize());
            int pGridY = static_cast<int>(mPlayer.getPosition().y / mWorld.getTileSize());
            for(int i = 0; i < cantidad; i++) {
                mWorld.spawnItem(pGridX, pGridY, id);
            }
        }
    }

    // --- HOTBAR SHORTCUTS (1, 2, 3, 4) ---
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num1)) mActiveWheelSlot = 3;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num2)) mActiveWheelSlot = 2;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num3)) mActiveWheelSlot = 0;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num4)) mActiveWheelSlot = 1;

    InventorySlot* wheel[4] = { &mEquippedConsumable, &mEquippedBlock, &mEquippedSecondary, &mEquippedPrimary };
    mSelectedBlock = wheel[mActiveWheelSlot]->id;

    // ==================================================
    // SMOOTH CAMERA (Linear Interpolation - Lerp)
    // ==================================================
    sf::Vector2f targetPos = mPlayer.getPosition();
    if (mCameraPos.x == 0 && mCameraPos.y == 0) mCameraPos = targetPos; // Initialize if zero

    float cameraSpeed = 15.0f; // Adjust for snappier or looser tracking
    mCameraPos.x += (targetPos.x - mCameraPos.x) * cameraSpeed * dt.asSeconds();
    mCameraPos.y += (targetPos.y - mCameraPos.y) * cameraSpeed * dt.asSeconds();

    sf::View view = mWindow.getDefaultView();
    view.zoom(1.25f);
    view.setCenter(mCameraPos);
    mWindow.setView(view);

    // --- DEV CHEATS: SUMMON MOBS ---
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::O)) {
        sf::Vector2i mousePos = sf::Mouse::getPosition(mWindow);
        sf::Vector2f worldPos = mWindow.mapPixelToCoords(mousePos, mWindow.getView());
        int gX = static_cast<int>(std::floor(worldPos.x / mWorld.getTileSize()));
        int gY = static_cast<int>(std::floor(worldPos.y / mWorld.getTileSize()));
        if (mWorld.getBlock(gX, gY) == 0) {
            mMobs.push_back(std::make_unique<Dodo>(worldPos, mDodoTexture));
            sf::sleep(sf::milliseconds(200));
        }
    }

    if (sf::Keyboard::isKeyPressed(sf::Keyboard::P)) {
        sf::Vector2i mousePos = sf::Mouse::getPosition(mWindow);
        sf::Vector2f worldPos = mWindow.mapPixelToCoords(mousePos, mWindow.getView());
        int gX = static_cast<int>(std::floor(worldPos.x / mWorld.getTileSize()));
        int gY = static_cast<int>(std::floor(worldPos.y / mWorld.getTileSize()));
        if (mWorld.getBlock(gX, gY) == 0) {
            mMobs.push_back(std::make_unique<Troodon>(worldPos, mTroodonTexture));
            sf::sleep(sf::milliseconds(200));
        }
    }

    // --- RANGE CALCULATIONS FOR INTERACTION ---
    sf::Vector2i pixelPos = sf::Mouse::getPosition(mWindow);
    sf::Vector2f worldPos = mWindow.mapPixelToCoords(pixelPos);
    float tileSize = mWorld.getTileSize();

    int gridX = static_cast<int>(std::floor(worldPos.x / tileSize));
    int gridY = static_cast<int>(std::floor(worldPos.y / tileSize));

    float blockCenterX = (gridX * tileSize) + (tileSize / 2.0f);
    float blockCenterY = (gridY * tileSize) + (tileSize / 2.0f);
    sf::Vector2f playerCenter = mPlayer.getPosition();

    float dx = blockCenterX - playerCenter.x;
    float dy = blockCenterY - playerCenter.y;
    float distance = std::sqrt(dx*dx + dy*dy);
    float maxRange = 180.0f; // Interaction radius

    // ==================================================
    // COMBAT SYSTEM (Melee and Ranged)
    // ==================================================
    if (mPlayer.canDealDamage()) {
        int equippedID = mSelectedBlock;

        // --- RANGED: BOW ---
        if (equippedID == ItemID::BOW) {
            if (consumeItem(ItemID::ARROW, 1)) {
                sf::Vector2i mousePixel = sf::Mouse::getPosition(mWindow);
                sf::Vector2f mouseWorld = mWindow.mapPixelToCoords(mousePixel);
                sf::Vector2f pPos = mPlayer.getCenter();

                float dirX = mouseWorld.x - pPos.x;
                float dirY = mouseWorld.y - pPos.y;
                float length = std::sqrt(dirX*dirX + dirY*dirY);

                float speed = 1800.0f; // High velocity arrows
                sf::Vector2f velocity((dirX / length) * speed, (dirY / length) * speed);

                mProjectiles.push_back(std::make_unique<Projectile>(pPos, velocity, *mWorld.getTexture(ItemID::ARROW)));

                mSndBuild.setPitch(2.0f); // Higher pitch for arrow loose
                mSndBuild.play();
            } else {
                std::cout << "Out of arrows!" << std::endl;
            }
            mPlayer.registerHit(); // Lock damage flag for this swing/shot state
        }
        // --- MELEE: SWORDS & PICKAXES ---
        else {
            int toolDamage = 1; // Default fist damage
            if (equippedID == ItemID::WOOD_PICKAXE) toolDamage = 3;
            if (equippedID == ItemID::STONE_PICKAXE) toolDamage = 5;
            if (equippedID == ItemID::IRON_PICKAXE) toolDamage = 8;
            if (equippedID == ItemID::TUNGSTEN_PICKAXE) toolDamage = 12;
            if (equippedID == ItemID::WOOD_SWORD) toolDamage = 6;
            if (equippedID == ItemID::STONE_SWORD) toolDamage = 10;
            if (equippedID == ItemID::IRON_SWORD) toolDamage = 18;
            if (equippedID == ItemID::TUNGSTEN_SWORD) toolDamage = 30;

            sf::FloatRect attackHitbox = mPlayer.getWeaponHitbox();

            for (auto& mob : mMobs) {
                if (attackHitbox.intersects(mob->getBounds())) {
                    float dir = (mPlayer.getPosition().x < mob->getPosition().x) ? 1.0f : -1.0f;
                    if (mob->takeDamage(toolDamage, dir)) {
                        mSndHit.setPitch(1.0f + (rand() % 40) / 100.0f);
                        mSndHit.play();
                        spawnParticles(mob->getBounds().getPosition() + sf::Vector2f(mob->getBounds().getSize().x / 2.0f, 0.0f), ItemID::MEAT, 8);
                    }
                    mPlayer.registerHit();
                    break; // Hit only one mob per swing
                }
            }
        }
    }

    // ==================================================
    // MINING SYSTEM (Hold Left Click)
    // ==================================================
    if (!mIsInventoryOpen && mSelectedBlock != ItemID::BOW) {
        if (sf::Mouse::isButtonPressed(sf::Mouse::Left)) {
            if (distance <= maxRange) {
                // Determine if we targeted a new block
                if (mMiningPos.x != gridX || mMiningPos.y != gridY) {
                    mMiningTimer = 0.0f;
                    mMiningPos = sf::Vector2i(gridX, gridY);

                    int blockID = mWorld.getBlock(gridX, gridY);
                    if (blockID != 0) {
                        // Dynamic Tree Hardness (Bottom trunks take longer)
                        if (blockID == ItemID::WOOD) {
                            int blocksAbove = 0;
                            int checkY = gridY;
                            while (mWorld.getBlock(gridX, checkY) == ItemID::WOOD || mWorld.getBlock(gridX, checkY) == ItemID::LEAVES) {
                                blocksAbove++;
                                checkY--;
                            }
                            mCurrentHardness = std::max(0.6f, 0.2f * blocksAbove);
                        }
                        else {
                            mCurrentHardness = getBlockHardness(blockID);
                        }
                    } else {
                        mCurrentHardness = 0.0f; // Looking at air
                    }
                }

                // Process Mining
                if (mCurrentHardness > 0.0f) {
                    float miningSpeed = 1.0f;
                    int equippedID = mSelectedBlock;
                    int targetID = mWorld.getBlock(gridX, gridY);

                    int pickaxeTier = getPickaxeTier(equippedID);
                    int requiredTier = getRequiredPickaxeTier(targetID);

                    if (pickaxeTier < requiredTier) {
                        miningSpeed = 0.0f; // Invalid tool, cannot mine
                    } else {
                        if (equippedID == ItemID::WOOD_PICKAXE) miningSpeed = 3.0f;
                        else if (equippedID == ItemID::STONE_PICKAXE) miningSpeed = 5.0f;
                        else if (equippedID == ItemID::IRON_PICKAXE) miningSpeed = 10.0f;
                        else if (equippedID == ItemID::TUNGSTEN_PICKAXE) miningSpeed = 20.0f;
                        else if (equippedID >= ItemID::WOOD_SWORD && equippedID <= ItemID::TUNGSTEN_SWORD) miningSpeed = 1.5f;
                    }

                    mMiningTimer += dt.asSeconds() * miningSpeed;

                    // Block Breaks!
                    if (mMiningTimer >= mCurrentHardness) {
                        int brokenBlockID = mWorld.getBlock(mMiningPos.x, mMiningPos.y);
                        std::pair<int, int> posKey = {mMiningPos.x, mMiningPos.y};

                        // Drop contents of destroyed containers
                        if (brokenBlockID == ItemID::CHEST) {
                            if (mActiveChests.find(posKey) != mActiveChests.end()) {
                                for (const auto& slot : mActiveChests[posKey].slots) {
                                    if (slot.id != ItemID::AIR && slot.count > 0) {
                                        for(int i=0; i<slot.count; i++) mWorld.spawnItem(mMiningPos.x, mMiningPos.y, slot.id);
                                    }
                                }
                                mActiveChests.erase(posKey);
                            }
                        }
                        else if (brokenBlockID == ItemID::FURNACE) {
                            if (mActiveFurnaces.find(posKey) != mActiveFurnaces.end()) {
                                auto& fd = mActiveFurnaces[posKey];
                                if (fd.input.id != 0) for(int i=0; i<fd.input.count; i++) mWorld.spawnItem(mMiningPos.x, mMiningPos.y, fd.input.id);
                                if (fd.fuel.id != 0) for(int i=0; i<fd.fuel.count; i++) mWorld.spawnItem(mMiningPos.x, mMiningPos.y, fd.fuel.id);
                                if (fd.output.id != 0) for(int i=0; i<fd.output.count; i++) mWorld.spawnItem(mMiningPos.x, mMiningPos.y, fd.output.id);
                                mActiveFurnaces.erase(posKey);
                            }
                        }

                        // VFx
                        sf::Vector2f blockCenter(mMiningPos.x * tileSize + tileSize/2.0f, mMiningPos.y * tileSize + tileSize/2.0f);
                        spawnParticles(blockCenter, brokenBlockID, 12);

                        // Tree Domino Effect Logic
                        if (brokenBlockID == ItemID::WOOD) {
                            bool isTree = false;

                            // Find the base
                            int bottomY = mMiningPos.y;
                            while (mWorld.getBlock(mMiningPos.x, bottomY + 1) == ItemID::WOOD) bottomY++;
                            int blockUnder = mWorld.getBlock(mMiningPos.x, bottomY + 1);

                            bool restsOnNature = (blockUnder == ItemID::DIRT || blockUnder == ItemID::GRASS || blockUnder == ItemID::SNOW);

                            // Find the top
                            int topY = mMiningPos.y;
                            while (mWorld.getBlock(mMiningPos.x, topY - 1) == ItemID::WOOD) topY--;
                            bool hasLeavesOnTop = (mWorld.getBlock(mMiningPos.x, topY - 1) == ItemID::LEAVES);

                            if (restsOnNature && hasLeavesOnTop) isTree = true;

                            if (isTree) {
                                // Break upwards recursively
                                int currY = mMiningPos.y;
                                while (mWorld.getBlock(mMiningPos.x, currY) == ItemID::WOOD || mWorld.getBlock(mMiningPos.x, currY) == ItemID::LEAVES) {
                                    int currentBlock = mWorld.getBlock(mMiningPos.x, currY);
                                    mWorld.setBlock(mMiningPos.x, currY, 0);
                                    mWorld.spawnItem(mMiningPos.x, currY, currentBlock);

                                    // Clear surrounding leaves
                                    for(int ox = -2; ox <= 2; ++ox) {
                                        for(int oy = -2; oy <= 2; ++oy) {
                                            if(mWorld.getBlock(mMiningPos.x + ox, currY + oy) == ItemID::LEAVES) {
                                                mWorld.setBlock(mMiningPos.x + ox, currY + oy, 0);
                                                // 15% sapling drop rate
                                                if (rand() % 100 < 15) mWorld.spawnItem(mMiningPos.x + ox, currY + oy, ItemID::LEAVES);
                                            }
                                        }
                                    }
                                    currY--;
                                }
                            } else {
                                // Just a placed wooden block
                                mWorld.setBlock(mMiningPos.x, mMiningPos.y, 0);
                                mWorld.spawnItem(mMiningPos.x, mMiningPos.y, brokenBlockID);
                            }
                        }
                        else {
                            // Standard block break
                            mWorld.setBlock(mMiningPos.x, mMiningPos.y, 0);
                            mWorld.spawnItem(mMiningPos.x, mMiningPos.y, brokenBlockID);
                        }

                        mSndBreak.setPitch(0.8f + (rand() % 40) / 100.0f);
                        mSndBreak.play();

                        mCurrentHardness = 0.0f;
                        mMiningTimer = 0.0f;
                    }
                }
                else if (mCurrentHardness == -1.0f) {
                    mMiningTimer = 0.0f; // Prevent mining bedrock
                }
            } else {
                mMiningTimer = 0.0f; // Reset if dragged too far
                mMiningPos = sf::Vector2i(-1, -1);
            }
        }
        else {
            mMiningTimer = 0.0f; // Reset on mouse release
            mMiningPos = sf::Vector2i(-1, -1);
        }
    }

    if (mActionTimer > 0.0f) mActionTimer -= dt.asSeconds();

    // ==================================================
    // SECONDARY ACTION (Right Click: Place, Eat, Interact)
    // ==================================================
    if (sf::Mouse::isButtonPressed(sf::Mouse::Right)) {
        if (mActionTimer <= 0.0f) {

            // 1. Try environment interaction first (Doors, Workbenches)
            if (interactWithBlock(gridX, gridY)) {
                mActionTimer = 0.3f;
            }
            // 2. Consume Food
            else if (mSelectedBlock == ItemID::MEAT) {
                if (mPlayer.getHp() < mPlayer.getMaxHp() && wheel[mActiveWheelSlot]->count > 0) {
                    wheel[mActiveWheelSlot]->count--;
                    if (wheel[mActiveWheelSlot]->count == 0) wheel[mActiveWheelSlot]->id = 0;

                    mPlayer.heal(20);
                    mSndBuild.setPitch(0.5f); // Chomping sound approximation
                    mSndBuild.play();
                    mActionTimer = 0.25f;
                }
            }
            // 3. Summon Boss
            else if (mSelectedBlock == ItemID::MEAT_MEDALLION) {
                if (wheel[mActiveWheelSlot]->count > 0) {
                    wheel[mActiveWheelSlot]->count--;
                    if (wheel[mActiveWheelSlot]->count == 0) wheel[mActiveWheelSlot]->id = 0;

                    sf::Vector2f spawnPos(mPlayer.getPosition().x, mPlayer.getPosition().y - 800.0f); // Drop from sky
                    mMobs.push_back(std::make_unique<TRex>(spawnPos, mTRexTexture));

                    mSndBreak.setPitch(0.2f); // Deep roar
                    mSndBreak.play();

                    mActionTimer = 1.0f;
                    calculateTotalWeight();
                }
            }
            // 4. Place Building Blocks
            else if (distance <= maxRange) {
                bool isPlaceableBlock = ((mSelectedBlock >= ItemID::DIRT && mSelectedBlock <= ItemID::SNOW) ||
                                          mSelectedBlock == ItemID::DOOR || mSelectedBlock == ItemID::CRAFTING_TABLE ||
                                          mSelectedBlock == ItemID::FURNACE || mSelectedBlock == ItemID::CHEST);

                // Blocks must be anchored to existing blocks
                bool hasAdjacentBlock = (mWorld.getBlock(gridX - 1, gridY) != 0) ||
                                        (mWorld.getBlock(gridX + 1, gridY) != 0) ||
                                        (mWorld.getBlock(gridX, gridY - 1) != 0) ||
                                        (mWorld.getBlock(gridX, gridY + 1) != 0);

                if (isPlaceableBlock && wheel[mActiveWheelSlot]->count > 0) {
                    // MULTI-TILE: Doors require 3 vertical air spaces
                    if (mSelectedBlock == ItemID::DOOR) {
                        if (mWorld.getBlock(gridX, gridY) == ItemID::AIR &&
                            mWorld.getBlock(gridX, gridY - 1) == ItemID::AIR &&
                            mWorld.getBlock(gridX, gridY - 2) == ItemID::AIR &&
                            World::isSolid(mWorld.getBlock(gridX, gridY + 1)))
                        {
                            mWorld.setBlock(gridX, gridY, ItemID::DOOR);         // Base
                            mWorld.setBlock(gridX, gridY - 1, ItemID::DOOR_MID); // Middle
                            mWorld.setBlock(gridX, gridY - 2, ItemID::DOOR_TOP); // Top

                            wheel[mActiveWheelSlot]->count--;
                            if (wheel[mActiveWheelSlot]->count == 0) wheel[mActiveWheelSlot]->id = 0;

                            mSndBuild.setPitch(1.0f + (rand() % 20) / 100.0f);
                            mSndBuild.play();
                            mActionTimer = 0.15f;
                            calculateTotalWeight();
                        }
                    }
                    // SINGLE-TILE BLOCKS
                    else if (mWorld.getBlock(gridX, gridY) == 0 && hasAdjacentBlock) {
                        sf::FloatRect blockRect(gridX * tileSize, gridY * tileSize, tileSize, tileSize);

                        // Prevent placing blocks inside the player or enemies
                        if (!mPlayer.getGlobalBounds().intersects(blockRect)) {
                            bool isMobInWay = false;
                            for (auto& mob : mMobs) {
                                if (mob->getBounds().intersects(blockRect)) {
                                    isMobInWay = true; break;
                                }
                            }

                            if (!isMobInWay) {
                                mWorld.setBlock(gridX, gridY, mSelectedBlock);

                                wheel[mActiveWheelSlot]->count--;
                                if (wheel[mActiveWheelSlot]->count == 0) wheel[mActiveWheelSlot]->id = 0;

                                mSndBuild.setPitch(1.0f + (rand() % 20) / 100.0f);
                                mSndBuild.play();
                                mActionTimer = 0.15f;
                                calculateTotalWeight();
                            }
                        }
                    }
                }
            }
        }
    }

    // --- QUICK SAVE/LOAD (F5 / F6) ---
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::F5)) {
        saveGame();
        sf::sleep(sf::milliseconds(300));
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::F6)) {
        loadGame();
        sf::sleep(sf::milliseconds(300));
    }

    // ==========================================
    // MOB SPAWNER LOGIC
    // ==========================================
    mSpawnTimer -= dt.asSeconds();
    if (mSpawnTimer <= 0.0f) {
        mSpawnTimer = 10.0f + (rand() % 10); // 10-20 seconds between spawn attempts

        if (mMobs.size() < MAX_MOBS) {
            float playerX = mPlayer.getPosition().x;
            float spawnDistance = 1500.0f + (rand() % 1000); // Far off-screen
            float spawnX = (rand() % 2 == 0) ? (playerX - spawnDistance) : (playerX + spawnDistance);

            int gridX = static_cast<int>(std::floor(spawnX / mWorld.getTileSize()));
            float spawnY = 0.0f;
            bool groundFound = false;

            // Raycast down to find surface
            for (int y = 1; y < WORLD_HEIGHT; ++y) {
                if (World::isSolid(mWorld.getBlock(gridX, y))) {
                    bool isSpaceClear = true;
                    // Check a 3x3 area above the ground for safety
                    for (int checkX = gridX - 1; checkX <= gridX + 1; ++checkX) {
                        for (int checkY = y - 3; checkY <= y - 1; ++checkY) {
                            if (checkY >= 0 && World::isSolid(mWorld.getBlock(checkX, checkY))) {
                                isSpaceClear = false;
                                break;
                            }
                        }
                        if (!isSpaceClear) break;
                    }

                    if (isSpaceClear) {
                        spawnY = y * mWorld.getTileSize();
                        groundFound = true;
                    }
                    break;
                }
            }

            if (groundFound) {
                bool isNight = (mGameTime > (DAY_LENGTH * 0.5f));
                sf::Vector2f spawnPos(spawnX, spawnY);
                if (isNight) {
                    mMobs.push_back(std::make_unique<Troodon>(spawnPos, mTroodonTexture));
                } else {
                    mMobs.push_back(std::make_unique<Dodo>(spawnPos, mDodoTexture));
                }
            }
        }
    }

    // Determine global light factor for mob rendering
    float currentBrightness = (mGameTime > (DAY_LENGTH * 0.5f)) ? 0.0f : 1.0f;

    // --- MOB UPDATE & PLAYER DAMAGE COLLISION ---
    for (auto it = mMobs.begin(); it != mMobs.end(); ) {
        auto& mob = **it;
        mob.update(dt, mPlayer.getPosition(), mWorld, currentBrightness);

        if (!mob.isDead() && mob.getBounds().intersects(mPlayer.getGlobalBounds())) {
            float dir = (mPlayer.getPosition().x > mob.getPosition().x) ? 1.0f : -1.0f;

            // Compute Damage Reduction from Armor
            int totalDefense = 0;
            if (mArmorHead.id == ItemID::WOOD_HELMET) totalDefense += 2;
            if (mArmorChest.id == ItemID::WOOD_CHEST) totalDefense += 4;
            if (mArmorLegs.id == ItemID::WOOD_LEGS)   totalDefense += 3;
            if (mArmorBoots.id == ItemID::WOOD_BOOTS) totalDefense += 1;

            int finalDamage = std::max(1, mob.getDamage() - totalDefense); // Minimum 1 damage

            if (mPlayer.takeDamage(finalDamage, dir)) {
                mSndHit.setPitch(0.7f);
                mSndHit.play();
                std::cout << "Hit! Raw: " << mob.getDamage() << " | Blocked: " << totalDefense << " | Taken: " << finalDamage << std::endl;
            }
        }

        // Clean up corpses and drop loot
        if (mob.isDead()) {
            mWorld.spawnItem(50, mob.getPosition()); // Drop meat
            mSndBreak.setPitch(1.5f);
            mSndBreak.play();
            it = mMobs.erase(it);
        } else {
            ++it;
        }
    }

    // --- PARTICLE PHYSICS UPDATE ---
    for (auto it = mParticles.begin(); it != mParticles.end(); ) {
        it->velocity.y += 1200.0f * dt.asSeconds(); // Gravity
        it->position += it->velocity * dt.asSeconds();
        it->lifetime -= dt.asSeconds();

        if (it->lifetime <= 0.0f) it = mParticles.erase(it);
        else ++it;
    }

    // --- PROJECTILE PHYSICS UPDATE ---
    for (auto it = mProjectiles.begin(); it != mProjectiles.end(); ) {
        auto& proj = **it;
        proj.update(dt, mWorld);

        if (!proj.isDead()) {
            for (auto& mob : mMobs) {
                if (!mob->isDead() && proj.getBounds().intersects(mob->getBounds())) {
                    float dir = (proj.getVelocity().x > 0) ? 1.0f : -1.0f;
                    if (mob->takeDamage(proj.getDamage(), dir)) {
                        mSndHit.setPitch(1.2f);
                        mSndHit.play();
                        spawnParticles(mob->getPosition(), ItemID::MEAT, 10);
                        proj.kill();
                        break;
                    }
                }
            }
        }

        if (proj.isDead()) it = mProjectiles.erase(it);
        else ++it;
    }
}

/**
 * @brief Main Render loop.
 * Draws backgrounds, lighting, world chunks, entities, and UI elements.
 */
void Game::render() {
    mWindow.clear(mAmbientLight);

    if (mGameState == GameState::MainMenu) {
        sf::View defaultView = mWindow.getDefaultView();
        mWindow.setView(defaultView);

        // Stretch sky to fill the menu window
        mSkySprite.setPosition(0.0f, 0.0f);
        if (mSkyTexture.getSize().x > 0) {
            mSkySprite.setScale(defaultView.getSize().x / mSkyTexture.getSize().x,
                                defaultView.getSize().y / mSkyTexture.getSize().y);
        }
        mSkySprite.setColor(sf::Color::White);
        mWindow.draw(mSkySprite);

        mWindow.draw(mMenuTitleText);
        mWindow.draw(mMenuPlayText);
        mWindow.draw(mMenuExitText);
    }
    else if (mGameState == GameState::IntroCinematic) {
        if (mCinematicPhase == 0 || mCinematicPhase == 1) {
            mWindow.setView(mWindow.getDefaultView());
            mWindow.clear(sf::Color::Black);
            mWindow.draw(mCinematicTextYear);
            if (mCinematicPhase == 1) mWindow.draw(mCinematicTextPlanet);
        }
        else if (mCinematicPhase == 2) {
            // Setup daylight for the crash
            mAmbientLight = sf::Color(255, 255, 255);
            sf::View currentView = mWindow.getView();

            mSkySprite.setPosition(currentView.getCenter().x - currentView.getSize().x / 2.0f,
                                   currentView.getCenter().y - currentView.getSize().y / 2.0f);
            if (mSkyTexture.getSize().x > 0) {
                mSkySprite.setScale(currentView.getSize().x / mSkyTexture.getSize().x,
                                    currentView.getSize().y / mSkyTexture.getSize().y);
            }
            mWindow.draw(mSkySprite);

            mWorld.render(mWindow, mAmbientLight);

            // Draw meteor trail particles
            sf::RectangleShape pShape;
            for (const auto& p : mParticles) {
                pShape.setSize(sf::Vector2f(p.size, p.size));
                pShape.setPosition(p.position);
                pShape.setFillColor(p.color);
                mWindow.draw(pShape);
            }

            mCapsuleSprite.setPosition(mCapsulePos);
            mWindow.draw(mCapsuleSprite);
        }
    }
    else {
        mWindow.clear(mAmbientLight);

        // ==========================================
        // DYNAMIC LIGHTING CALCULATION (Cave System)
        // ==========================================
        sf::Color skyLight = mAmbientLight;
        sf::Vector2f centerPos = mPlayer.getCenter();
        int pGridX = static_cast<int>(centerPos.x / mWorld.getTileSize());
        int pGridY = static_cast<int>(centerPos.y / mWorld.getTileSize());

        // Approximate surface height using nearby column samples
        float accumulatedSurfaceY = 0.0f;
        int columnsChecked = 0;
        int range = 10;
        for (int x = pGridX - range; x <= pGridX + range; ++x) {
            for (int y = 0; y < WORLD_HEIGHT; ++y) {
                if (mWorld.getBlock(x, y) != 0) {
                    accumulatedSurfaceY += y;
                    columnsChecked++;
                    break;
                }
            }
        }

        float averageSurfaceY = (columnsChecked > 0) ? (accumulatedSurfaceY / columnsChecked) : pGridY;
        float depth = pGridY - averageSurfaceY;
        float caveFactor = std::clamp(depth / 20.0f, 0.0f, 1.0f); // Darken progressively over 20 blocks down

        sf::Uint8 r = static_cast<sf::Uint8>(std::max(5.0f, skyLight.r * (1.0f - caveFactor)));
        sf::Uint8 g = static_cast<sf::Uint8>(std::max(5.0f, skyLight.g * (1.0f - caveFactor)));
        sf::Uint8 b = static_cast<sf::Uint8>(std::max(5.0f, skyLight.b * (1.0f - caveFactor)));
        sf::Color finalAmbient(r, g, b);

        // DRAW SKY
        sf::View currentView = mWindow.getView();
        mSkySprite.setPosition(currentView.getCenter().x - currentView.getSize().x / 2.0f,
                               currentView.getCenter().y - currentView.getSize().y / 2.0f);
        if (mSkyTexture.getSize().x > 0) {
            mSkySprite.setScale(currentView.getSize().x / mSkyTexture.getSize().x,
                                currentView.getSize().y / mSkyTexture.getSize().y);
        }
        mSkySprite.setColor(skyLight);
        mWindow.draw(mSkySprite);

        // Calculate Torch Influence on Player
        sf::Color playerColor = finalAmbient;
        float lightRadius = 250.0f;
        float maxTorchIntensity = 0.0f;

        for (int x = pGridX - 10; x <= pGridX + 10; ++x) {
            for (int y = pGridY - 10; y <= pGridY + 10; ++y) {
                if (mWorld.getBlock(x, y) == ItemID::TORCH) {
                    float torchX = x * mWorld.getTileSize() + mWorld.getTileSize()/2.0f;
                    float torchY = y * mWorld.getTileSize() + mWorld.getTileSize()/2.0f;
                    float dist = std::sqrt(std::pow(centerPos.x - torchX, 2) + std::pow(centerPos.y - torchY, 2));
                    if (dist < lightRadius) {
                        maxTorchIntensity = std::max(maxTorchIntensity, 1.0f - (dist / lightRadius));
                    }
                }
            }
        }

        float baseR = std::max(playerColor.r / 255.0f, maxTorchIntensity);
        float baseG = std::max(playerColor.g / 255.0f, maxTorchIntensity);
        float baseB = std::max(playerColor.b / 255.0f, maxTorchIntensity);
        playerColor.r = static_cast<sf::Uint8>(std::min(255.0f, baseR * 255.0f));
        playerColor.g = static_cast<sf::Uint8>(std::min(255.0f, baseG * 255.0f));
        playerColor.b = static_cast<sf::Uint8>(std::min(255.0f, baseB * 255.0f));

        // DRAW WORLD & ENTITIES
        mWorld.render(mWindow, finalAmbient);
        mPlayer.render(mWindow, playerColor);

        for (auto& mob : mMobs) mob->render(mWindow, finalAmbient);
        for (auto& proj : mProjectiles) proj->render(mWindow, finalAmbient);

        // DRAW PARTICLES (Fading and darkened by ambient light)
        sf::RectangleShape pShape;
        for (const auto& p : mParticles) {
            pShape.setSize(sf::Vector2f(p.size, p.size));
            pShape.setPosition(p.position);

            sf::Color c = p.color;
            c.a = static_cast<sf::Uint8>(255.0f * (p.lifetime / p.maxLifetime));
            c.r = (c.r * finalAmbient.r) / 255;
            c.g = (c.g * finalAmbient.g) / 255;
            c.b = (c.b * finalAmbient.b) / 255;

            pShape.setFillColor(c);
            mWindow.draw(pShape);
        }

        // DRAW UI
        if (mIsPlayerDead) {
            renderDeathScreen();
        } else {
            renderHUD();
            renderMenus();
        }

        // PAUSE OVERLAY
        if (mGameState == GameState::Paused) {
            mWindow.setView(mWindow.getDefaultView());
            sf::RectangleShape darkOverlay(sf::Vector2f(1920, 1080));
            darkOverlay.setFillColor(sf::Color(0, 0, 0, 150));
            mWindow.draw(darkOverlay);
            mWindow.draw(mPauseTitleText);

            mMenuExitText.setPosition(1920 / 2.0f, 650.0f);
            mWindow.draw(mMenuExitText);
            mMenuExitText.setPosition(1920 / 2.0f, 750.0f); // Reset for Main Menu
        }
    }
    mWindow.display();
}

/**
 * @brief Serializes the game state to a binary file.
 */
void Game::saveGame() {
    std::ofstream file("savegame.dat", std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Error: Could not create save file." << std::endl;
        return;
    }

    // 1. Player Position
    sf::Vector2f pos = mPlayer.getPosition();
    file.write(reinterpret_cast<const char*>(&pos), sizeof(pos));

    // 2. Backpack
    size_t backpackSize = mBackpack.size();
    file.write(reinterpret_cast<const char*>(&backpackSize), sizeof(backpackSize));
    for (const auto& slot : mBackpack) {
        file.write(reinterpret_cast<const char*>(&slot.id), sizeof(slot.id));
        file.write(reinterpret_cast<const char*>(&slot.count), sizeof(slot.count));
    }

    // 3. Hotbar/Tactical Wheel
    InventorySlot equipped[4] = { mEquippedPrimary, mEquippedSecondary, mEquippedBlock, mEquippedConsumable };
    for (int i = 0; i < 4; ++i) {
        file.write(reinterpret_cast<const char*>(&equipped[i].id), sizeof(equipped[i].id));
        file.write(reinterpret_cast<const char*>(&equipped[i].count), sizeof(equipped[i].count));
    }

    // 4. Armor
    InventorySlot armorToSave[4] = { mArmorHead, mArmorChest, mArmorLegs, mArmorBoots };
    for (int i = 0; i < 4; ++i) {
        file.write(reinterpret_cast<const char*>(&armorToSave[i].id), sizeof(armorToSave[i].id));
        file.write(reinterpret_cast<const char*>(&armorToSave[i].count), sizeof(armorToSave[i].count));
    }

    // 5. Chunk Data
    mWorld.saveToStream(file);

    // 6. Furnace States
    size_t furnaceCount = mActiveFurnaces.size();
    file.write(reinterpret_cast<const char*>(&furnaceCount), sizeof(furnaceCount));
    for (const auto& pair : mActiveFurnaces) {
        file.write(reinterpret_cast<const char*>(&pair.first.first), sizeof(pair.first.first));
        file.write(reinterpret_cast<const char*>(&pair.first.second), sizeof(pair.first.second));
        file.write(reinterpret_cast<const char*>(&pair.second), sizeof(FurnaceData));
    }

    // 7. Chest States
    size_t chestCount = mActiveChests.size();
    file.write(reinterpret_cast<const char*>(&chestCount), sizeof(chestCount));
    for (const auto& pair : mActiveChests) {
        file.write(reinterpret_cast<const char*>(&pair.first.first), sizeof(pair.first.first));
        file.write(reinterpret_cast<const char*>(&pair.first.second), sizeof(pair.first.second));
        for (const auto& slot : pair.second.slots) {
            file.write(reinterpret_cast<const char*>(&slot.id), sizeof(slot.id));
            file.write(reinterpret_cast<const char*>(&slot.count), sizeof(slot.count));
        }
    }

    file.close();
    std::cout << "--- GAME SAVED SUCCESSFULLY ---" << std::endl;
}

/**
 * @brief Deserializes the game state from a binary file.
 */
void Game::loadGame() {
    std::ifstream file("savegame.dat", std::ios::binary);
    if (!file.is_open()) {
        std::cout << "No save game found." << std::endl;
        return;
    }

    // 1. Player Position
    sf::Vector2f pos;
    file.read(reinterpret_cast<char*>(&pos), sizeof(pos));
    mPlayer.setPosition(pos);

    // 2. Backpack
    size_t backpackSize = 0;
    file.read(reinterpret_cast<char*>(&backpackSize), sizeof(backpackSize));
    mBackpack.resize(backpackSize);
    for (size_t i = 0; i < backpackSize; ++i) {
        int id = 0, count = 0;
        file.read(reinterpret_cast<char*>(&id), sizeof(id));
        file.read(reinterpret_cast<char*>(&count), sizeof(count));
        mBackpack[i].id = id;
        mBackpack[i].count = count;
    }

    // 3. Hotbar
    InventorySlot* equippedPointers[4] = { &mEquippedPrimary, &mEquippedSecondary, &mEquippedBlock, &mEquippedConsumable };
    for (int i = 0; i < 4; ++i) {
        file.read(reinterpret_cast<char*>(&equippedPointers[i]->id), sizeof(equippedPointers[i]->id));
        file.read(reinterpret_cast<char*>(&equippedPointers[i]->count), sizeof(equippedPointers[i]->count));
    }

    // 4. Armor
    InventorySlot* armorToLoad[4] = { &mArmorHead, &mArmorChest, &mArmorLegs, &mArmorBoots };
    for (int i = 0; i < 4; ++i) {
        file.read(reinterpret_cast<char*>(&armorToLoad[i]->id), sizeof(armorToLoad[i]->id));
        file.read(reinterpret_cast<char*>(&armorToLoad[i]->count), sizeof(armorToLoad[i]->count));
    }
    mPlayer.setEquippedWeapon(0); // Safely reset active hand

    // 5. Chunk Data
    mWorld.loadFromStream(file);

    // 6. Furnaces
    mActiveFurnaces.clear();
    size_t furnaceCount = 0;
    if (file.read(reinterpret_cast<char*>(&furnaceCount), sizeof(furnaceCount))) {
        for (size_t i = 0; i < furnaceCount; ++i) {
            int fx, fy; FurnaceData fd;
            file.read(reinterpret_cast<char*>(&fx), sizeof(fx));
            file.read(reinterpret_cast<char*>(&fy), sizeof(fy));
            file.read(reinterpret_cast<char*>(&fd), sizeof(FurnaceData));
            mActiveFurnaces[{fx, fy}] = fd;
        }
    }

    // 7. Chests
    mActiveChests.clear();
    size_t chestCount = 0;
    if (file.read(reinterpret_cast<char*>(&chestCount), sizeof(chestCount))) {
        for (size_t i = 0; i < chestCount; ++i) {
            int cx, cy; ChestData cd;
            file.read(reinterpret_cast<char*>(&cx), sizeof(cx));
            file.read(reinterpret_cast<char*>(&cy), sizeof(cy));
            for (int s = 0; s < 24; ++s) {
                file.read(reinterpret_cast<char*>(&cd.slots[s].id), sizeof(cd.slots[s].id));
                file.read(reinterpret_cast<char*>(&cd.slots[s].count), sizeof(cd.slots[s].count));
            }
            mActiveChests[{cx, cy}] = cd;
        }
    }

    file.close();
    std::cout << "--- GAME LOADED ---" << std::endl;
}

/**
 * @brief Aggregates the weight of all items carried by the player.
 */
void Game::calculateTotalWeight() {
    float oldWeight = mCurrentWeight;
    mCurrentWeight = 0.0f;

    for (const auto& slot : mBackpack) {
        if (slot.id != 0) mCurrentWeight += mItemDatabase[slot.id].weight * slot.count;
    }

    InventorySlot* wheel[4] = { &mEquippedPrimary, &mEquippedSecondary, &mEquippedBlock, &mEquippedConsumable };
    for (int i = 0; i < 4; ++i) {
        if (wheel[i]->id != 0) mCurrentWeight += mItemDatabase[wheel[i]->id].weight * wheel[i]->count;
    }

    if (mDraggedItem.id != 0) {
        mCurrentWeight += mItemDatabase[mDraggedItem.id].weight * mDraggedItem.count;
    }

    if (std::abs(mCurrentWeight - oldWeight) > 0.01f) {
        std::cout << "Current weight: " << mCurrentWeight << " / " << mMaxWeight << std::endl;
    }
}

/**
 * @brief Attempts to insert items into the player's inventories (Hotbar -> Armor -> Backpack).
 * @return True if all items were successfully stored.
 */
bool Game::addItemToBackpack(int id, int amount) {
    // 1. Try stacking in Tactical Wheel
    InventorySlot* wheel[4] = { &mEquippedConsumable, &mEquippedBlock, &mEquippedSecondary, &mEquippedPrimary };
    for (int i = 0; i < 4; ++i) {
        if (wheel[i]->id == id && wheel[i]->count < mItemDatabase[id].maxStack) {
            int space = mItemDatabase[id].maxStack - wheel[i]->count;
            if (amount <= space) {
                wheel[i]->count += amount;
                calculateTotalWeight();
                return true;
            } else {
                wheel[i]->count += space;
                amount -= space;
            }
        }
    }

    // 2. Try stacking in Armor Wheel
    InventorySlot* armor[4] = { &mArmorHead, &mArmorChest, &mArmorLegs, &mArmorBoots };
    for (int i = 0; i < 4; ++i) {
        if (armor[i]->id == id && armor[i]->count < mItemDatabase[id].maxStack) {
            int space = mItemDatabase[id].maxStack - armor[i]->count;
            if (amount <= space) {
                armor[i]->count += amount;
                calculateTotalWeight();
                return true;
            } else {
                armor[i]->count += space;
                amount -= space;
            }
        }
    }

    // 3. Try stacking in Backpack
    for (auto& slot : mBackpack) {
        if (slot.id == id && slot.count < mItemDatabase[id].maxStack) {
            int space = mItemDatabase[id].maxStack - slot.count;
            if (amount <= space) {
                slot.count += amount;
                calculateTotalWeight();
                return true;
            } else {
                slot.count += space;
                amount -= space;
            }
        }
    }

    // 4. Find an empty Backpack slot
    for (auto& slot : mBackpack) {
        if (slot.id == ItemID::AIR) {
            slot.id = id;
            if (amount <= mItemDatabase[id].maxStack) {
                slot.count = amount;
                calculateTotalWeight();
                return true;
            } else {
                slot.count = mItemDatabase[id].maxStack;
                amount -= mItemDatabase[id].maxStack;
            }
        }
    }
    return false; // Inventory full
}

int Game::getItemCount(int id) {
    int total = 0;
    for (const auto& slot : mBackpack) {
        if (slot.id == id) total += slot.count;
    }
    InventorySlot* wheel[4] = { &mEquippedConsumable, &mEquippedBlock, &mEquippedSecondary, &mEquippedPrimary };
    for (int i = 0; i < 4; ++i) {
        if (wheel[i]->id == id) total += wheel[i]->count;
    }
    return total;
}

bool Game::consumeItem(int id, int amount) {
    if (getItemCount(id) < amount) return false;

    int remaining = amount;

    // Consume from hotbar first
    InventorySlot* wheel[4] = { &mEquippedConsumable, &mEquippedBlock, &mEquippedSecondary, &mEquippedPrimary };
    for (int i = 0; i < 4; ++i) {
        if (wheel[i]->id == id) {
            if (wheel[i]->count >= remaining) {
                wheel[i]->count -= remaining;
                if (wheel[i]->count == 0) wheel[i]->id = ItemID::AIR;
                calculateTotalWeight();
                return true;
            } else {
                remaining -= wheel[i]->count;
                wheel[i]->count = 0;
                wheel[i]->id = ItemID::AIR;
            }
        }
    }

    // Consume from backpack
    for (auto& slot : mBackpack) {
        if (slot.id == id) {
            if (slot.count >= remaining) {
                slot.count -= remaining;
                if (slot.count == 0) slot.id = ItemID::AIR;
                calculateTotalWeight();
                return true;
            } else {
                remaining -= slot.count;
                slot.count = 0;
                slot.id = ItemID::AIR;
            }
        }
    }
    return false;
}

bool Game::canCraft(const Recipe& recipe) {
    for (const auto& ing : recipe.ingredients) {
        if (getItemCount(ing.first) < ing.second) return false;
    }
    return true;
}

void Game::craftItem(const Recipe& recipe) {
    if (!canCraft(recipe)) return;
    for (const auto& ing : recipe.ingredients) consumeItem(ing.first, ing.second);
    addItemToBackpack(recipe.resultId, recipe.resultCount);
    mSndBuild.setPitch(1.5f);
    mSndBuild.play();
}

/**
 * @brief Resurrects the player at the default spawn point and resets their health.
 */
void Game::respawnPlayer() {
    mIsPlayerDead = false;
    mPlayer.setHp(100);

    // Spawn point is fixed at column X = 100
    int gridX = 100;
    float spawnX = gridX * mWorld.getTileSize();
    float spawnY = 0.0f;

    // Scan downward for the surface
    for (int y = 0; y < WORLD_HEIGHT; ++y) {
        if (mWorld.getBlock(gridX, y) != 0) {
            spawnY = (y - 2) * mWorld.getTileSize();
            break;
        }
    }

    mPlayer.setPosition(sf::Vector2f(spawnX, spawnY));
    mCameraPos = sf::Vector2f(spawnX, spawnY); // Snap camera to avoid dizzying sweep
    mPlayer.setVelocity(sf::Vector2f(0.0f, 0.0f)); // Clear physics forces
}

/**
 * @brief Triggers block-specific behavior (e.g. Opening UI panels or Doors).
 */
bool Game::interactWithBlock(int gridX, int gridY) {
    int blockID = mWorld.getBlock(gridX, gridY);
    if (blockID == ItemID::AIR) return false;

    sf::Vector2f playerCenter = mPlayer.getCenter();
    float pGridX = playerCenter.x / mWorld.getTileSize();
    float pGridY = playerCenter.y / mWorld.getTileSize();

    float dx = pGridX - gridX;
    float dy = pGridY - gridY;
    if (std::sqrt(dx*dx + dy*dy) > 4.0f) return false; // Interaction range limit

    if (blockID == ItemID::CRAFTING_TABLE) {
        mIsCraftingTableOpen = !mIsCraftingTableOpen;
        mIsFurnaceOpen = false;
        mIsInventoryOpen = mIsCraftingTableOpen;
        return true;
    }

    if (blockID == ItemID::FURNACE) {
        mIsFurnaceOpen = !mIsFurnaceOpen;
        mIsInventoryOpen = mIsFurnaceOpen;
        mIsCraftingTableOpen = false;
        if (mIsFurnaceOpen) mOpenFurnacePos = {gridX, gridY};
        return true;
    }

    if (blockID >= ItemID::DOOR && blockID <= ItemID::DOOR_TOP) {
        int baseY = gridY;
        if (blockID == ItemID::DOOR_MID) baseY = gridY + 1;
        if (blockID == ItemID::DOOR_TOP) baseY = gridY + 2;

        mWorld.setBlock(gridX, baseY,     ItemID::DOOR_OPEN);
        mWorld.setBlock(gridX, baseY - 1, ItemID::DOOR_OPEN_MID);
        mWorld.setBlock(gridX, baseY - 2, ItemID::DOOR_OPEN_TOP);
        return true;
    }

    if (blockID >= ItemID::DOOR_OPEN && blockID <= ItemID::DOOR_OPEN_TOP) {
        int baseY = gridY;
        if (blockID == ItemID::DOOR_OPEN_MID) baseY = gridY + 1;
        if (blockID == ItemID::DOOR_OPEN_TOP) baseY = gridY + 2;

        mWorld.setBlock(gridX, baseY,     ItemID::DOOR);
        mWorld.setBlock(gridX, baseY - 1, ItemID::DOOR_MID);
        mWorld.setBlock(gridX, baseY - 2, ItemID::DOOR_TOP);
        return true;
    }

    if (blockID == ItemID::CHEST) {
        mIsChestOpen = !mIsChestOpen;
        mIsInventoryOpen = mIsChestOpen;
        mIsFurnaceOpen = false;
        mIsCraftingTableOpen = false;
        if (mIsChestOpen) {
            mOpenChestPos = {gridX, gridY};
            if (mActiveChests.find(mOpenChestPos) == mActiveChests.end()) {
                mActiveChests[mOpenChestPos] = ChestData();
            }
        }
        return true;
    }

    return false;
}

/**
 * @brief Interpolates global illumination colors to simulate a day/night cycle.
 */
void Game::updateDayNightCycle(float dtAsSeconds) {
    mGameTime += dtAsSeconds;
    if (mGameTime > DAY_LENGTH) {
        mGameTime -= DAY_LENGTH;
        mTotalDays++;
        std::cout << "Day " << mTotalDays << " has begun!" << std::endl;
    }

    float timeFraction = mGameTime / DAY_LENGTH;

    sf::Color nightColor(30, 30, 50);     // Dark blue
    sf::Color dawnColor(200, 120, 100);   // Orange/Pink
    sf::Color dayColor(255, 255, 255);    // White
    sf::Color duskColor(120, 60, 80);     // Purple/Red

    sf::Color startColor, targetColor;
    float lerpFactor = 0.0f;

    if (timeFraction < 0.25f) { // Dawn
        startColor = dawnColor; targetColor = dayColor;
        lerpFactor = timeFraction / 0.25f;
    }
    else if (timeFraction < 0.50f) { // Noon
        startColor = dayColor; targetColor = dayColor;
        lerpFactor = 1.0f;
    }
    else if (timeFraction < 0.75f) { // Dusk
        startColor = dayColor; targetColor = nightColor;
        lerpFactor = (timeFraction - 0.50f) / 0.25f;
    }
    else { // Night
        startColor = nightColor; targetColor = dawnColor;
        lerpFactor = (timeFraction - 0.75f) / 0.25f;
    }

    mAmbientLight.r = startColor.r + (targetColor.r - startColor.r) * lerpFactor;
    mAmbientLight.g = startColor.g + (targetColor.g - startColor.g) * lerpFactor;
    mAmbientLight.b = startColor.b + (targetColor.b - startColor.b) * lerpFactor;
    mSkySprite.setColor(mAmbientLight);
}

/**
 * @brief Renders the static On-Screen Display (Hearts, Active Item, Mining Bar).
 */
void Game::renderHUD() {
    float tileSize = mWorld.getTileSize();
    sf::Vector2i pixelPos = sf::Mouse::getPosition(mWindow);
    sf::Vector2f worldPos = mWindow.mapPixelToCoords(pixelPos);

    int gridX = static_cast<int>(std::floor(worldPos.x / tileSize));
    int gridY = static_cast<int>(std::floor(worldPos.y / tileSize));

    float blockCenterX = (gridX * tileSize) + (tileSize / 2.0f);
    float blockCenterY = (gridY * tileSize) + (tileSize / 2.0f);

    sf::Vector2f playerCenter = mPlayer.getPosition();
    float dist = std::sqrt(std::pow(blockCenterX - playerCenter.x, 2) + std::pow(blockCenterY - playerCenter.y, 2));
    float maxRange = 180.0f;

    // Grid Selector Reticle
    sf::RectangleShape selector(sf::Vector2f(tileSize, tileSize));
    selector.setPosition(gridX * tileSize, gridY * tileSize);
    selector.setFillColor(sf::Color::Transparent);
    selector.setOutlineThickness(2.0f);
    selector.setOutlineColor(dist <= maxRange ? sf::Color(255, 255, 255, 150) : sf::Color(255, 0, 0, 150));
    mWindow.draw(selector);

    // Mining Progress Bar
    if (mMiningTimer > 0.0f && mCurrentHardness > 0.0f) {
        float percent = std::min(mMiningTimer / mCurrentHardness, 1.0f);
        sf::RectangleShape barBg(sf::Vector2f(tileSize * 0.8f, 6.0f));
        barBg.setPosition(mMiningPos.x * tileSize + tileSize * 0.1f, mMiningPos.y * tileSize + tileSize * 0.1f);
        barBg.setFillColor(sf::Color::Black);
        mWindow.draw(barBg);

        sf::RectangleShape barFill(sf::Vector2f((tileSize * 0.8f) * percent, 4.0f));
        barFill.setPosition(barBg.getPosition().x + 1.0f, barBg.getPosition().y + 1.0f);
        barFill.setFillColor(sf::Color::White);
        mWindow.draw(barFill);
    }

    // Static UI Elements (Camera detached)
    sf::View currentView = mWindow.getView();
    mWindow.setView(mWindow.getDefaultView());

    // Active Item Display
    float uiX = 240.0f, uiY = 70.0f, slotSize = 40.0f;
    sf::RectangleShape activeSlotBg(sf::Vector2f(slotSize, slotSize));
    activeSlotBg.setPosition(uiX, uiY);
    activeSlotBg.setFillColor(sf::Color(0, 0, 0, 150));
    activeSlotBg.setOutlineThickness(2.0f);
    activeSlotBg.setOutlineColor(sf::Color::White);
    mWindow.draw(activeSlotBg);

    if (mSelectedBlock != ItemID::AIR) {
        const sf::Texture* tex = mWorld.getTexture(mSelectedBlock);
        if (tex != nullptr) {
            sf::Sprite icon(*tex);
            float scale = (slotSize - 10.0f) / tex->getSize().x;
            icon.setScale(scale, scale);
            icon.setPosition(uiX + 5.0f, uiY + 5.0f);
            mWindow.draw(icon);

            InventorySlot* wheel[4] = { &mEquippedConsumable, &mEquippedBlock, &mEquippedSecondary, &mEquippedPrimary };
            if (wheel[mActiveWheelSlot]->count > 0) {
                mUiText.setString(std::to_string(wheel[mActiveWheelSlot]->count));
                mUiText.setCharacterSize(14);
                sf::FloatRect textBounds = mUiText.getLocalBounds();
                mUiText.setPosition(uiX + slotSize - textBounds.width - 5.0f, uiY + slotSize - textBounds.height - 8.0f);
                mWindow.draw(mUiText);
            }
        }
    }

    // Health Hearts
    int hpPerHeart = 10;
    int maxHearts = mPlayer.getMaxHp() / hpPerHeart;
    int heartsToDraw = mPlayer.getHp() / hpPerHeart;
    float startX = 20.0f, startY = 20.0f, spacing = 40.0f;
    for (int i = 0; i < maxHearts; ++i) {
        mHeartSprite.setTexture(i < heartsToDraw ? mHeartFullTex : mHeartEmptyTex);
        mHeartSprite.setPosition(startX + (i * spacing), startY);
        mWindow.draw(mHeartSprite);
    }

    mWindow.setView(currentView);
}

/**
 * @brief Renders the interactive UI overlays (Inventory, Crafting, Chests, Furnaces).
 */
void Game::renderMenus() {
    if (mIsInventoryOpen || mIsFurnaceOpen) {
        sf::View uiView(sf::FloatRect(0.0f, 0.0f, static_cast<float>(mWindow.getSize().x), static_cast<float>(mWindow.getSize().y)));
        mWindow.setView(uiView);
        sf::RectangleShape overlay(sf::Vector2f(mWindow.getSize().x, mWindow.getSize().y));
        overlay.setFillColor(sf::Color(0, 0, 0, 150));
        mWindow.draw(overlay);
    }

    // Furnace Menu
    if (mIsFurnaceOpen) {
        mWindow.setView(mWindow.getDefaultView());
        float scale = 6.0f;
        mFurnaceBgSprite.setScale(scale, scale);

        float bgX = (mWindow.getSize().x - mFurnaceBgTex.getSize().x * scale) / 2.0f;
        float bgY = (mWindow.getSize().y - mFurnaceBgTex.getSize().y * scale) / 2.0f;
        mFurnaceBgSprite.setPosition(bgX, bgY);
        mWindow.draw(mFurnaceBgSprite);

        float firePercent = mActiveFurnaces[mOpenFurnacePos].maxFuelTimer > 0.0f ? std::clamp(mActiveFurnaces[mOpenFurnacePos].fuelTimer / mActiveFurnaces[mOpenFurnacePos].maxFuelTimer, 0.0f, 1.0f) : 0.0f;
        float arrowPercent = std::clamp(mActiveFurnaces[mOpenFurnacePos].smeltTimer / 3.0f, 0.0f, 1.0f);

        int fireHeight = static_cast<int>(9 * firePercent);
        int fireTexY = 26 + (9 - fireHeight);
        mFurnaceFireSprite.setTextureRect(sf::IntRect(39, fireTexY, 6, fireHeight));
        mFurnaceFireSprite.setScale(scale, scale);
        mFurnaceFireSprite.setPosition(bgX + (39 * scale), bgY + (fireTexY * scale));
        mWindow.draw(mFurnaceFireSprite);

        int arrowWidth = static_cast<int>(22 * arrowPercent);
        mFurnaceArrowSprite.setTextureRect(sf::IntRect(53, 24, arrowWidth, 13));
        mFurnaceArrowSprite.setScale(scale, scale);
        mFurnaceArrowSprite.setPosition(bgX + (53 * scale), bgY + (24 * scale));
        mWindow.draw(mFurnaceArrowSprite);

        auto drawFurnaceSlot = [&](InventorySlot& slot, float startX, float startY) {
            if (slot.id != ItemID::AIR && slot.count > 0) {
                sf::Sprite itemSprite(*mWorld.getTexture(slot.id));
                itemSprite.setScale(1.5f, 1.5f);
                itemSprite.setPosition(bgX + (startX * scale) + 5.0f, bgY + (startY * scale) + 5.0f);
                mWindow.draw(itemSprite);

                sf::Text countText(std::to_string(slot.count), *mDeathTitleText.getFont(), 16);
                countText.setOutlineColor(sf::Color::Black);
                countText.setOutlineThickness(1.5f);
                countText.setPosition(bgX + (startX * scale) + 20.0f, bgY + (startY * scale) + 20.0f);
                mWindow.draw(countText);
            }
        };

        drawFurnaceSlot(mActiveFurnaces[mOpenFurnacePos].input, 34.0f, 13.0f);
        drawFurnaceSlot(mActiveFurnaces[mOpenFurnacePos].fuel, 34.0f, 37.0f);
        drawFurnaceSlot(mActiveFurnaces[mOpenFurnacePos].output, 82.0f, 22.0f);
    }

    if (mIsInventoryOpen) {
        sf::View uiView(sf::FloatRect(0.0f, 0.0f, static_cast<float>(mWindow.getSize().x), static_cast<float>(mWindow.getSize().y)));
        mWindow.setView(uiView);

        float slotSize = 48.0f, padding = 8.0f;
        float startX = (mWindow.getSize().x / 2.0f) - ((10 * slotSize + 9 * padding) / 2.0f);
        float startY = mWindow.getSize().y - (3 * slotSize + 2 * padding) - 50.0f;

        // A) Backpack Grid
        for (int row = 0; row < 3; ++row) {
            for (int col = 0; col < 10; ++col) {
                int index = row * 10 + col;
                sf::RectangleShape slotBg(sf::Vector2f(slotSize, slotSize));
                slotBg.setPosition(startX + col * (slotSize + padding), startY + row * (slotSize + padding));
                slotBg.setFillColor(sf::Color(40, 40, 40, 200));
                slotBg.setOutlineThickness(2.0f);
                slotBg.setOutlineColor(sf::Color(100, 100, 100));
                mWindow.draw(slotBg);

                if (mBackpack[index].id != ItemID::AIR) {
                    const sf::Texture* tex = mWorld.getTexture(mBackpack[index].id);
                    if (tex) {
                        sf::Sprite icon(*tex);
                        float scale = (slotSize - 10.0f) / tex->getSize().x;
                        icon.setScale(scale, scale);
                        icon.setPosition(slotBg.getPosition().x + 5.0f, slotBg.getPosition().y + 5.0f);
                        mWindow.draw(icon);

                        mUiText.setString(std::to_string(mBackpack[index].count));
                        mUiText.setCharacterSize(14);
                        sf::FloatRect textBounds = mUiText.getLocalBounds();
                        mUiText.setPosition(slotBg.getPosition().x + slotSize - textBounds.width - 4.0f,
                                            slotBg.getPosition().y + slotSize - textBounds.height - 6.0f);
                        mWindow.draw(mUiText);
                    }
                }
            }
        }

        // B) Tactical Wheel / Crafting
        if (!mIsFurnaceOpen && !mIsChestOpen) {
            float wheelCX = mWindow.getSize().x / 2.0f;
            float wheelCY = mWindow.getSize().y / 2.0f - 100.0f;

            InventorySlot* wheelSlots[4];
            std::string wheelLabels[4];

            if (mIsArmorWheelActive) {
                wheelSlots[0] = &mArmorHead; wheelSlots[1] = &mArmorChest;
                wheelSlots[2] = &mArmorLegs; wheelSlots[3] = &mArmorBoots;
                wheelLabels[0] = "Head"; wheelLabels[1] = "Chest";
                wheelLabels[2] = "Legs"; wheelLabels[3] = "Boots";
                mWheelSprite.setColor(sf::Color(100, 150, 255)); // Blue tint
            } else {
                wheelSlots[0] = &mEquippedConsumable; wheelSlots[1] = &mEquippedBlock;
                wheelSlots[2] = &mEquippedSecondary;  wheelSlots[3] = &mEquippedPrimary;
                wheelLabels[0] = "Usable"; wheelLabels[1] = "Block";
                wheelLabels[2] = "Weapon 2"; wheelLabels[3] = "Weapon 1";
                mWheelSprite.setColor(sf::Color::White); // Normal
            }

            mWheelSprite.setPosition(wheelCX, wheelCY);
            mWindow.draw(mWheelSprite);

            float offset = 100.0f;
            sf::Vector2f wheelPositions[4] = { {0, -offset}, {0, offset}, {-offset, 0}, {offset, 0} };
            sf::Vector2f textOffsets[4] = { {0.0f, -45.0f}, {0.0f, 25.0f}, {-45.0f, -10.0f}, {45.0f, -10.0f} };

            for (int i = 0; i < 4; ++i) {
                float slotX = wheelCX + wheelPositions[i].x;
                float slotY = wheelCY + wheelPositions[i].y;

                mUiText.setString(wheelLabels[i]);
                mUiText.setCharacterSize(18);
                mUiText.setOutlineThickness(2.0f);
                sf::FloatRect textBounds = mUiText.getLocalBounds();
                mUiText.setPosition(slotX + textOffsets[i].x - (textBounds.width / 2.0f), slotY + textOffsets[i].y);
                mWindow.draw(mUiText);

                if (wheelSlots[i]->id != ItemID::AIR) {
                    const sf::Texture* tex = mWorld.getTexture(wheelSlots[i]->id);
                    if (tex) {
                        sf::Sprite icon(*tex);
                        float scale = 40.0f / tex->getSize().x;
                        icon.setScale(scale, scale);
                        icon.setOrigin(tex->getSize().x / 2.0f, tex->getSize().y / 2.0f);
                        icon.setPosition(slotX, slotY);
                        mWindow.draw(icon);

                        sf::Text qtyText = mUiText;
                        qtyText.setString(std::to_string(wheelSlots[i]->count));
                        qtyText.setCharacterSize(14);
                        qtyText.setPosition(slotX + 15.0f, slotY + 10.0f);
                        mWindow.draw(qtyText);
                    }
                }
            }

            // Crafting List
            float craftX = 50.0f, craftY = 100.0f, rowHeight = 60.0f, panelWidth = 320.0f;
            mUiText.setString(mIsCraftingTableOpen ? "CRAFTING TABLE" : "MANUAL CRAFTING");
            mUiText.setCharacterSize(24);
            mUiText.setOutlineThickness(2.0f);
            mUiText.setPosition(craftX, craftY - 40.0f);
            mWindow.draw(mUiText);

            int displayIndex = 0;
            for (size_t i = 0; i < mRecipes.size(); ++i) {
                const Recipe& recipe = mRecipes[i];
                if (recipe.requiresTable && !mIsCraftingTableOpen) continue;

                bool possible = canCraft(recipe);
                sf::RectangleShape rowBg(sf::Vector2f(panelWidth, rowHeight - 5.0f));
                rowBg.setPosition(craftX, craftY + displayIndex * rowHeight);
                rowBg.setFillColor(sf::Color(40, 40, 40, 200));
                rowBg.setOutlineThickness(possible ? 2.0f : 1.0f);
                rowBg.setOutlineColor(possible ? sf::Color(50, 200, 50, 200) : sf::Color(100, 100, 100, 150));
                mWindow.draw(rowBg);

                const sf::Texture* resTex = mWorld.getTexture(recipe.resultId);
                if (resTex) {
                    sf::Sprite resIcon(*resTex);
                    float scale = 40.0f / resTex->getSize().x;
                    resIcon.setScale(scale, scale);
                    resIcon.setPosition(craftX + 10.0f, craftY + displayIndex * rowHeight + 7.0f);
                    if (!possible) resIcon.setColor(sf::Color(255, 255, 255, 100));
                    mWindow.draw(resIcon);

                    if (recipe.resultCount > 1) {
                        mUiText.setString(std::to_string(recipe.resultCount));
                        mUiText.setCharacterSize(16);
                        mUiText.setFillColor(sf::Color::Yellow);
                        mUiText.setPosition(craftX + 35.0f, craftY + displayIndex * rowHeight + 32.0f);
                        mWindow.draw(mUiText);
                        mUiText.setFillColor(sf::Color::White);
                    }
                }

                float ingX = craftX + 80.0f;
                for (const auto& ing : recipe.ingredients) {
                    const sf::Texture* ingTex = mWorld.getTexture(ing.first);
                    if (ingTex) {
                        sf::Sprite ingIcon(*ingTex);
                        float scale = 24.0f / ingTex->getSize().x;
                        ingIcon.setScale(scale, scale);
                        ingIcon.setPosition(ingX, craftY + displayIndex * rowHeight + 15.0f);
                        if (!possible) ingIcon.setColor(sf::Color(255, 255, 255, 150));
                        mWindow.draw(ingIcon);

                        mUiText.setString(std::to_string(getItemCount(ing.first)) + "/" + std::to_string(ing.second));
                        mUiText.setCharacterSize(14);
                        mUiText.setFillColor(getItemCount(ing.first) >= ing.second ? sf::Color::White : sf::Color(255, 80, 80));
                        mUiText.setPosition(ingX + 30.0f, craftY + displayIndex * rowHeight + 18.0f);
                        mWindow.draw(mUiText);
                        mUiText.setFillColor(sf::Color::White);
                        ingX += 75.0f;
                    }
                }
                displayIndex++;
            }
        }

        // C) Chest Interface
        if (mIsChestOpen) {
            float cSlotSize = 60.0f, cPadding = 10.0f;
            int cols = 6, rows = 4;
            float chestStartX = (mWindow.getSize().x - ((cols * (cSlotSize + cPadding)) + cPadding)) / 2.0f;
            float chestStartY = (mWindow.getSize().y - ((rows * (cSlotSize + cPadding)) + cPadding)) / 2.0f - 100.0f;

            mUiText.setString("CHEST");
            mUiText.setCharacterSize(24);
            mUiText.setPosition(chestStartX, chestStartY - 40.0f);
            mWindow.draw(mUiText);

            sf::RectangleShape chestBg(sf::Vector2f(cols * (cSlotSize + cPadding) + cPadding, rows * (cSlotSize + cPadding) + cPadding));
            chestBg.setPosition(chestStartX - cPadding, chestStartY - cPadding);
            chestBg.setFillColor(sf::Color(60, 40, 20, 240));
            chestBg.setOutlineThickness(3.0f);
            chestBg.setOutlineColor(sf::Color::Black);
            mWindow.draw(chestBg);

            ChestData& currentChest = mActiveChests[mOpenChestPos];
            for (int i = 0; i < 24; ++i) {
                float x = chestStartX + (i % cols) * (cSlotSize + cPadding);
                float y = chestStartY + (i / cols) * (cSlotSize + cPadding);

                sf::RectangleShape slotRect(sf::Vector2f(cSlotSize, cSlotSize));
                slotRect.setPosition(x, y);
                slotRect.setFillColor(sf::Color(0, 0, 0, 150));
                slotRect.setOutlineThickness(1.0f);
                slotRect.setOutlineColor(sf::Color(100, 100, 100));
                mWindow.draw(slotRect);

                InventorySlot& slot = currentChest.slots[i];
                if (slot.id != ItemID::AIR) {
                    const sf::Texture* tex = mWorld.getTexture(slot.id);
                    if (tex) {
                        sf::Sprite spr(*tex);
                        float scale = (cSlotSize - 10.0f) / tex->getSize().x;
                        spr.setScale(scale, scale);
                        spr.setPosition(x + 5.0f, y + 5.0f);
                        mWindow.draw(spr);

                        mUiText.setString(std::to_string(slot.count));
                        mUiText.setCharacterSize(16);
                        mUiText.setPosition(x + cSlotSize - 20.0f, y + cSlotSize - 25.0f);
                        mWindow.draw(mUiText);
                    }
                }
            }
        }

        // D) Floating Dragged Item
        if (mDraggedItem.id != ItemID::AIR) {
            const sf::Texture* tex = mWorld.getTexture(mDraggedItem.id);
            if (tex) {
                sf::Vector2i mousePos = sf::Mouse::getPosition(mWindow);
                sf::Sprite dragIcon(*tex);
                float scale = 40.0f / tex->getSize().x;
                dragIcon.setScale(scale, scale);
                dragIcon.setOrigin(tex->getSize().x / 2.0f, tex->getSize().y / 2.0f);
                dragIcon.setPosition(static_cast<float>(mousePos.x), static_cast<float>(mousePos.y));
                mWindow.draw(dragIcon);

                mUiText.setString(std::to_string(mDraggedItem.count));
                mUiText.setCharacterSize(16);
                mUiText.setPosition(mousePos.x + 10.0f, mousePos.y + 10.0f);
                mWindow.draw(mUiText);
            }
        }
    }
}

void Game::renderDeathScreen() {
    float screenW = static_cast<float>(mWindow.getSize().x);
    float screenH = static_cast<float>(mWindow.getSize().y);
    sf::View deathView(sf::FloatRect(0.0f, 0.0f, screenW, screenH));
    mWindow.setView(deathView);
    mWindow.draw(mDeathOverlay);
    mWindow.draw(mDeathTitleText);
    mWindow.draw(mDeathSubText);
}

void Game::handleMouseClick(float mx, float my) {
    float slotSize = 48.0f, padding = 8.0f;
    float startX = (mWindow.getSize().x / 2.0f) - ((10 * slotSize + 9 * padding) / 2.0f);
    float startY = mWindow.getSize().y - (3 * slotSize + 2 * padding) - 50.0f;

    // A. Pick up from Backpack
    for (int row = 0; row < 3; ++row) {
        for (int col = 0; col < 10; ++col) {
            float sx = startX + col * (slotSize + padding);
            float sy = startY + row * (slotSize + padding);
            if (sf::FloatRect(sx, sy, slotSize, slotSize).contains(mx, my)) {
                int index = row * 10 + col;
                if (mBackpack[index].id != ItemID::AIR) {
                    mDraggedItem = mBackpack[index];
                    mBackpack[index].id = ItemID::AIR;
                    mBackpack[index].count = 0;
                    return;
                }
            }
        }
    }

    // B. Pick up from Active Wheel
    if (mDraggedItem.id == ItemID::AIR && !mIsFurnaceOpen && !mIsChestOpen) {
        float wheelCX = mWindow.getSize().x / 2.0f;
        float wheelCY = mWindow.getSize().y / 2.0f - 100.0f;
        float offset = 100.0f;
        sf::Vector2f wheelPositions[4] = { {0, -offset}, {0, offset}, {-offset, 0}, {offset, 0} };

        InventorySlot* wheelSlots[4];
        if (mIsArmorWheelActive) {
            wheelSlots[0] = &mArmorHead; wheelSlots[1] = &mArmorChest;
            wheelSlots[2] = &mArmorLegs; wheelSlots[3] = &mArmorBoots;
        } else {
            wheelSlots[0] = &mEquippedConsumable; wheelSlots[1] = &mEquippedBlock;
            wheelSlots[2] = &mEquippedSecondary;  wheelSlots[3] = &mEquippedPrimary;
        }

        for (int i = 0; i < 4; ++i) {
            float sx = wheelCX + wheelPositions[i].x - 30.0f;
            float sy = wheelCY + wheelPositions[i].y - 30.0f;
            if (sf::FloatRect(sx, sy, 60.0f, 60.0f).contains(mx, my) && wheelSlots[i]->id != ItemID::AIR) {
                mDraggedItem = *wheelSlots[i];
                wheelSlots[i]->id = ItemID::AIR;
                wheelSlots[i]->count = 0;
                calculateTotalWeight();
                return;
            }
        }
    }

    // C. Click Crafting Panel
    if (mDraggedItem.id == ItemID::AIR && !mIsFurnaceOpen && !mIsChestOpen) {
        float craftX = 50.0f, craftY = 100.0f, rowHeight = 60.0f, panelWidth = 320.0f;
        int displayIndex = 0;
        for (size_t i = 0; i < mRecipes.size(); ++i) {
            if (mRecipes[i].requiresTable && !mIsCraftingTableOpen) continue;
            float sx = craftX, sy = craftY + displayIndex * rowHeight;
            if (sf::FloatRect(sx, sy, panelWidth, rowHeight - 5.0f).contains(mx, my)) {
                if (canCraft(mRecipes[i])) craftItem(mRecipes[i]);
                return;
            }
            displayIndex++;
        }
    }

    // D. Pick up from Furnace
    if (mIsFurnaceOpen && mDraggedItem.id == ItemID::AIR) {
        float scale = 6.0f;
        float bgX = (mWindow.getSize().x - mFurnaceBgTex.getSize().x * scale) / 2.0f;
        float bgY = (mWindow.getSize().y - mFurnaceBgTex.getSize().y * scale) / 2.0f;

        auto pickFurnaceSlot = [&](InventorySlot& slot, float texX, float texY, float texW, float texH) {
            if (sf::FloatRect(bgX + texX * scale, bgY + texY * scale, texW * scale, texH * scale).contains(mx, my)) {
                if (slot.id != ItemID::AIR) {
                    mDraggedItem = slot;
                    slot.id = ItemID::AIR;
                    slot.count = 0;
                    return true;
                }
            }
            return false;
        };

        if (pickFurnaceSlot(mActiveFurnaces[mOpenFurnacePos].input, 34.0f, 13.0f, 17.0f, 11.0f)) return;
        if (pickFurnaceSlot(mActiveFurnaces[mOpenFurnacePos].fuel, 34.0f, 37.0f, 17.0f, 11.0f)) return;
        if (pickFurnaceSlot(mActiveFurnaces[mOpenFurnacePos].output, 81.0f, 21.0f, 23.0f, 17.0f)) return;
    }

    // E. Pick up from Chest
    if (mIsChestOpen && mDraggedItem.id == ItemID::AIR) {
        float cSlotSize = 60.0f, cPad = 10.0f;
        int cols = 6, rows = 4;
        float chestStartX = (mWindow.getSize().x - ((cols * (cSlotSize + cPad)) + cPad)) / 2.0f;
        float chestStartY = (mWindow.getSize().y - ((rows * (cSlotSize + cPad)) + cPad)) / 2.0f - 100.0f;

        ChestData& currentChest = mActiveChests[mOpenChestPos];
        for (int i = 0; i < 24; ++i) {
            float sx = chestStartX + (i % cols) * (cSlotSize + cPad);
            float sy = chestStartY + (i / cols) * (cSlotSize + cPad);

            if (sf::FloatRect(sx, sy, cSlotSize, cSlotSize).contains(mx, my) && currentChest.slots[i].id != ItemID::AIR) {
                mDraggedItem = currentChest.slots[i];
                currentChest.slots[i].id = ItemID::AIR;
                currentChest.slots[i].count = 0;
                calculateTotalWeight();
                return;
            }
        }
    }
}

void Game::handleMouseRelease(float mx, float my) {
    if (mDraggedItem.id == ItemID::AIR) return;

    float slotSize = 48.0f, padding = 8.0f;
    float startX = (mWindow.getSize().x / 2.0f) - ((10 * slotSize + 9 * padding) / 2.0f);
    float startY = mWindow.getSize().y - (3 * slotSize + 2 * padding) - 50.0f;

    // A. Drop in Backpack
    for (int row = 0; row < 3; ++row) {
        for (int col = 0; col < 10; ++col) {
            float sx = startX + col * (slotSize + padding);
            float sy = startY + row * (slotSize + padding);
            if (sf::FloatRect(sx, sy, slotSize, slotSize).contains(mx, my)) {
                int index = row * 10 + col;
                InventorySlot temp = mBackpack[index];
                mBackpack[index] = mDraggedItem;
                mDraggedItem = temp;
                return;
            }
        }
    }

    // B. Drop in Tactical Wheel
    if (!mIsFurnaceOpen && !mIsChestOpen) {
        float wheelCX = mWindow.getSize().x / 2.0f;
        float wheelCY = mWindow.getSize().y / 2.0f - 100.0f;
        sf::Vector2f wheelPositions[4] = { {0, -100.0f}, {0, 100.0f}, {-100.0f, 0}, {100.0f, 0} };

        InventorySlot* wheelSlots[4];
        if (mIsArmorWheelActive) {
            wheelSlots[0] = &mArmorHead; wheelSlots[1] = &mArmorChest;
            wheelSlots[2] = &mArmorLegs; wheelSlots[3] = &mArmorBoots;
        } else {
            wheelSlots[0] = &mEquippedConsumable; wheelSlots[1] = &mEquippedBlock;
            wheelSlots[2] = &mEquippedSecondary;  wheelSlots[3] = &mEquippedPrimary;
        }

        for (int i = 0; i < 4; ++i) {
            float sx = wheelCX + wheelPositions[i].x - 30.0f;
            float sy = wheelCY + wheelPositions[i].y - 30.0f;
            if (sf::FloatRect(sx, sy, 60.0f, 60.0f).contains(mx, my)) {
                int dragID = mDraggedItem.id;
                bool allowed = false;

                // Slot filtering validation
                if (mIsArmorWheelActive) {
                    if (i == 0 && dragID == ItemID::WOOD_HELMET) allowed = true;
                    else if (i == 1 && dragID == ItemID::WOOD_CHEST) allowed = true;
                    else if (i == 2 && dragID == ItemID::WOOD_LEGS) allowed = true;
                    else if (i == 3 && dragID == ItemID::WOOD_BOOTS) allowed = true;
                } else {
                    if (i == 0 && (dragID == ItemID::MEAT || dragID == ItemID::TORCH || dragID == ItemID::MEAT_MEDALLION)) allowed = true;
                    else if (i == 1 && (((dragID >= ItemID::DIRT && dragID <= ItemID::SNOW) && dragID != ItemID::TORCH) || dragID == ItemID::DOOR || dragID == ItemID::CRAFTING_TABLE || dragID == ItemID::FURNACE || dragID == ItemID::CHEST)) allowed = true;
                    else if ((i == 2 || i == 3) && ((dragID >= ItemID::WOOD_PICKAXE && dragID <= ItemID::TUNGSTEN_PICKAXE) || (dragID >= ItemID::WOOD_SWORD && dragID <= ItemID::BOW))) allowed = true;
                }

                if (allowed) {
                    if (wheelSlots[i]->id == dragID) {
                        int spaceLeft = mItemDatabase[dragID].maxStack - wheelSlots[i]->count;
                        if (mDraggedItem.count <= spaceLeft) {
                            wheelSlots[i]->count += mDraggedItem.count;
                            mDraggedItem.id = ItemID::AIR; mDraggedItem.count = 0;
                        } else {
                            wheelSlots[i]->count += spaceLeft;
                            mDraggedItem.count -= spaceLeft;
                        }
                    } else {
                        InventorySlot temp = *wheelSlots[i];
                        *wheelSlots[i] = mDraggedItem;
                        mDraggedItem = temp;
                    }
                    calculateTotalWeight();
                    return;
                } else {
                    std::cout << "Invalid item for this slot!" << std::endl;
                }
            }
        }
    }

    // C. Drop in Furnace
    if (mIsFurnaceOpen) {
        float scale = 6.0f;
        float bgX = (mWindow.getSize().x - mFurnaceBgTex.getSize().x * scale) / 2.0f;
        float bgY = (mWindow.getSize().y - mFurnaceBgTex.getSize().y * scale) / 2.0f;

        auto tryDropFurnace = [&](InventorySlot& slot, float texX, float texY, float texW, float texH, bool isOutput) {
            if (sf::FloatRect(bgX + texX * scale, bgY + texY * scale, texW * scale, texH * scale).contains(mx, my)) {
                if (isOutput) {
                    std::cout << "Cannot insert items into output tray!" << std::endl;
                    return false;
                }
                InventorySlot temp = slot;
                slot = mDraggedItem;
                mDraggedItem = temp;
                return true;
            }
            return false;
        };

        if (tryDropFurnace(mActiveFurnaces[mOpenFurnacePos].input, 34.0f, 13.0f, 17.0f, 11.0f, false)) return;
        if (tryDropFurnace(mActiveFurnaces[mOpenFurnacePos].fuel, 34.0f, 37.0f, 17.0f, 11.0f, false)) return;
        if (tryDropFurnace(mActiveFurnaces[mOpenFurnacePos].output, 81.0f, 21.0f, 23.0f, 17.0f, true)) return;
    }

    // D. Drop in Chest
    if (mIsChestOpen) {
        float cSlotSize = 60.0f, cPad = 10.0f;
        int cols = 6, rows = 4;
        float chestStartX = (mWindow.getSize().x - ((cols * (cSlotSize + cPad)) + cPad)) / 2.0f;
        float chestStartY = (mWindow.getSize().y - ((rows * (cSlotSize + cPad)) + cPad)) / 2.0f - 100.0f;

        ChestData& currentChest = mActiveChests[mOpenChestPos];
        for (int i = 0; i < 24; ++i) {
            float sx = chestStartX + (i % cols) * (cSlotSize + cPad);
            float sy = chestStartY + (i / cols) * (cSlotSize + cPad);

            if (sf::FloatRect(sx, sy, cSlotSize, cSlotSize).contains(mx, my)) {
                InventorySlot& clickedSlot = currentChest.slots[i];
                if (clickedSlot.id == mDraggedItem.id) {
                    int spaceLeft = mItemDatabase[clickedSlot.id].maxStack - clickedSlot.count;
                    if (mDraggedItem.count <= spaceLeft) {
                        clickedSlot.count += mDraggedItem.count;
                        mDraggedItem.id = ItemID::AIR; mDraggedItem.count = 0;
                    } else {
                        clickedSlot.count += spaceLeft;
                        mDraggedItem.count -= spaceLeft;
                    }
                } else {
                    InventorySlot temp = clickedSlot;
                    clickedSlot = mDraggedItem;
                    mDraggedItem = temp;
                }
                calculateTotalWeight();
                return;
            }
        }
    }

    // E. Dropped in empty space -> Add to inventory or toss into world
    if (mDraggedItem.id != ItemID::AIR) {
        if (!addItemToBackpack(mDraggedItem.id, mDraggedItem.count)) {
            int pGridX = static_cast<int>(mPlayer.getPosition().x / mWorld.getTileSize());
            int pGridY = static_cast<int>(mPlayer.getPosition().y / mWorld.getTileSize());
            for(int i = 0; i < mDraggedItem.count; i++) {
                mWorld.spawnItem(pGridX, pGridY, mDraggedItem.id);
            }
        }
        mDraggedItem.id = ItemID::AIR;
        mDraggedItem.count = 0;
    }
}

/**
 * @brief Spawns visual debris particles of a specific material type.
 */
void Game::spawnParticles(sf::Vector2f pos, int itemID, int count) {
    sf::Color pColor = sf::Color::White;
    switch(itemID) {
        case ItemID::DIRT: pColor = sf::Color(139, 69, 19); break;
        case ItemID::GRASS: pColor = sf::Color(34, 139, 34); break;
        case ItemID::STONE: case ItemID::COAL:
        case ItemID::FURNACE: pColor = sf::Color(128, 128, 128); break;
        case ItemID::WOOD: case ItemID::CRAFTING_TABLE:
        case ItemID::CHEST: case ItemID::DOOR:
            pColor = sf::Color(101, 67, 33); break;
        case ItemID::LEAVES: pColor = sf::Color(50, 205, 50); break;
        case ItemID::MEAT: pColor = sf::Color(200, 0, 0); break;       // Blood
        default: pColor = sf::Color(200, 200, 200); break;
    }

    for(int i = 0; i < count; ++i) {
        Particle p;
        p.position = pos;
        float vx = (rand() % 300) - 150.0f;
        float vy = -((rand() % 200) + 150.0f); // Always burst upwards
        p.velocity = sf::Vector2f(vx, vy);
        p.color = pColor;
        p.maxLifetime = 0.3f + (rand() % 40) / 100.0f;
        p.lifetime = p.maxLifetime;
        p.size = 4.0f + (rand() % 4);
        mParticles.push_back(p);
    }
}