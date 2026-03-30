#include "Game.h"
#include <cmath> // Necessary for std::sqrt
#include <iostream>

/**
 * Constructor for the Game class.
 * Initializes the window, loads resources (textures, sounds, fonts),
 * sets up the initial inventory, and configures the UI.
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
    , mSpawnTimer(5.0f) // Start with 5 seconds so it spawns quickly the first time
{
    mWindow.setFramerateLimit(120);

    // --- INITIALIZE DEATH SYSTEM ---
    mIsPlayerDead = false;
    mRespawnTimer = 0.0f;

    // 1. Dark Background (Bloody)
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
    // 1. HIT (Strike)
    if (mBufHit.loadFromFile("assets/Hit.wav")) {
        mSndHit.setBuffer(mBufHit);
        mSndHit.setVolume(50.0f); // Volume at 50%
        // Random pitch variation so it doesn't sound robotic
        mSndHit.setPitch(1.0f);
    }

    // 2. BREAK (Breakage)
    if (mBufBreak.loadFromFile("assets/Break.wav")) {
        mSndBreak.setBuffer(mBufBreak);
        mSndBreak.setVolume(70.0f);
    }

    // 3. BUILD (Build)
    if (mBufBuild.loadFromFile("assets/Build.wav")) {
        mSndBuild.setBuffer(mBufBuild);
        mSndBuild.setVolume(60.0f);
    }

    if (!mSkyTexture.loadFromFile("assets/Sky.png")) {
        // Error handling
    } else {
        mSkySprite.setTexture(mSkyTexture);
    }

    // --- LOAD FONT ---
    if (!mFont.loadFromFile("assets/font.ttf")) {
        // If it fails, warn in console (SFML will use an ugly default font or draw nothing)
        std::cerr << "Error: Could not load assets/font.ttf" << std::endl;
    }

    // --- LOAD HUD ---
    // Load your caveman designs
    if (!mHeartFullTex.loadFromFile("assets/HeartFull.png")) {
        // Basic error handling (in case you forget to put the image)
        std::cerr << "Error: Could not load HeartFull.png" << std::endl;
    }
    if (!mHeartEmptyTex.loadFromFile("assets/HeartEmpty.png")) {
        std::cerr << "Error: Could not load HeartEmpty.png" << std::endl;
    }

    // --- LOAD FURNACE INTERFACE ---
    if (!mFurnaceBgTex.loadFromFile("assets/FurnaceUI.png")) { std::cerr << "Error FurnaceUI\n"; }
    if (!mFurnaceFireTex.loadFromFile("assets/FurnaceFire.png")) { std::cerr << "Error FurnaceFire\n"; }
    if (!mFurnaceArrowTex.loadFromFile("assets/FurnaceArrow.png")) { std::cerr << "Error FurnaceArrow\n"; }

    mFurnaceBgSprite.setTexture(mFurnaceBgTex);
    mFurnaceFireSprite.setTexture(mFurnaceFireTex);
    mFurnaceArrowSprite.setTexture(mFurnaceArrowTex);

    // Base text configuration
    mUiText.setFont(mFont);
    mUiText.setCharacterSize(20);
    mUiText.setFillColor(sf::Color::White);
    mUiText.setOutlineColor(sf::Color::Black);
    mUiText.setOutlineThickness(1.0f);

    // --- GENERATE SOFT LIGHT TEXTURE ---
    sf::Image lightImg;
    lightImg.create(256, 256, sf::Color::Transparent);

    // We create a radial gradient: White in the center, transparent at the edges
    for (int y = 0; y < 256; ++y) {
        for (int x = 0; x < 256; ++x) {
            float dx = x - 128.0f;
            float dy = y - 128.0f;
            float distance = std::sqrt(dx * dx + dy * dy);
            float radius = 128.0f;

            if (distance < radius) {
                // We calculate intensity (1.0 in the center, 0.0 at the edge)
                float intensity = 1.0f - (distance / radius);
                // We use pow to make the light falloff more natural (non-linear)
                intensity = std::pow(intensity, 1.5f);

                // White color with variable Alpha
                lightImg.setPixel(x, y, sf::Color(255, 255, 255, static_cast<sf::Uint8>(255 * intensity)));
            }
        }
    }

    // --- PREPARE NEW INVENTORY ---
    mBackpack.resize(30); // Create 30 empty slots

    // --- CRAFTING RECIPES ---
    // MANUAL (false)
    mRecipes.push_back({ItemID::WOOD_PICKAXE, 1, false, {{ItemID::WOOD, 10}}});
    mRecipes.push_back({ItemID::WOOD_SWORD, 1, false, {{ItemID::WOOD, 7}}});
    mRecipes.push_back({ItemID::TORCH, 5, false, {{ItemID::WOOD, 2}, {ItemID::LEAVES, 2}}});
    mRecipes.push_back({ItemID::CRAFTING_TABLE, 1, false, {{ItemID::WOOD, 10}}});

    // ADVANCED - REQUIRE TABLE (true)
    mRecipes.push_back({ItemID::STONE_PICKAXE, 1, true, {{ItemID::WOOD, 5}, {ItemID::STONE, 5}}});
    mRecipes.push_back({ItemID::IRON_PICKAXE, 1, true, {{ItemID::IRON_INGOT, 5}, {ItemID::WOOD, 5}}});
    mRecipes.push_back({ItemID::TUNGSTEN_PICKAXE, 1, true, {{ItemID::TUNGSTEN_INGOT, 5}, {ItemID::WOOD, 5}}});
    mRecipes.push_back({ItemID::STONE_SWORD, 1, true, {{ItemID::WOOD, 2}, {ItemID::STONE, 6}}});
    mRecipes.push_back({ItemID::IRON_SWORD, 1, true, {{ItemID::WOOD, 2}, {ItemID::IRON_INGOT, 6}}});
    mRecipes.push_back({ItemID::TUNGSTEN_SWORD, 1, true, {{ItemID::WOOD, 2}, {ItemID::TUNGSTEN_INGOT, 6}}});
    mRecipes.push_back({ItemID::DOOR, 1, true, {{ItemID::WOOD, 6}}});
    mRecipes.push_back({ItemID::FURNACE, 1, true, {{ItemID::STONE, 10}}});
    mRecipes.push_back({ItemID::CHEST, 1, true, {{ItemID::IRON_INGOT, 2}, {ItemID::WOOD, 5}}});

    // --- ITEM DATABASE ---
    mItemDatabase[ItemID::DIRT] = {"Dirt", 1.0f, 99};
    mItemDatabase[ItemID::GRASS] = {"Grass", 1.0f, 99};
    mItemDatabase[ItemID::STONE] = {"Stone", 2.0f, 99};
    mItemDatabase[ItemID::WOOD] = {"Log", 1.5f, 99};
    mItemDatabase[ItemID::LEAVES] = {"Leaves", 0.1f, 99};
    mItemDatabase[ItemID::TORCH] = {"Torch", 0.2f, 99};
    // --- NUEVO: REGISTRAMOS LOS BIOMAS PARA QUE SE APILEN BIEN ---
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
    // --- NUEVO: ARCO Y FLECHAS ---
    mItemDatabase[ItemID::BOW] = {"Bow", 3.0f, 1};      // Solo cabe 1 arco por slot
    mItemDatabase[ItemID::ARROW] = {"Arrow", 0.1f, 99}; // Caben 99 flechas

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

    // Give initial items for testing
    addItemToBackpack(ItemID::DIRT, 15);
    addItemToBackpack(ItemID::MEAT, 5);
    addItemToBackpack(ItemID::TORCH, 5);
    // --- NUEVO: KIT DE ARQUERO ---
    addItemToBackpack(ItemID::BOW, 1);
    addItemToBackpack(ItemID::ARROW, 50);

    if (!mDodoTexture.loadFromFile("assets/Dodo.png")) {
        std::cerr << "Error: Missing Dodos" << std::endl;
    }

    if (!mTroodonTexture.loadFromFile("assets/Troodon.png")) {
        std::cerr << "Error: Missing Troodons" << std::endl;
    }

    if (!mWheelTexture.loadFromFile("assets/WheelGun.png")) {
        std::cerr << "Error: Missing WheelGun.png" << std::endl;
    } else {
        mWheelSprite.setTexture(mWheelTexture);
        // Center the origin so it's easy to place in the middle of the screen
        mWheelSprite.setOrigin(mWheelTexture.getSize().x / 2.0f, mWheelTexture.getSize().y / 2.0f);
        // If the image is too small or too large, you can adjust the scale here:
        mWheelSprite.setScale(2.0f, 2.0f);
    }

    // --- MAIN MENU TEXTS ---
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

    // Pause Text
    mPauseTitleText = mDeathTitleText; // Copy the style
    mPauseTitleText.setString("PAUSED");
    mPauseTitleText.setFillColor(sf::Color::White);
    sf::FloatRect pauseBounds = mPauseTitleText.getLocalBounds();
    mPauseTitleText.setOrigin(pauseBounds.width / 2.0f, pauseBounds.height / 2.0f);
}

Game::~Game() {
}

/**
 * Main game loop.
 * Handles the frame rate, processes events, updates game logic, and renders the scene.
 */
void Game::run() {
    sf::Clock clock;
    while (mWindow.isOpen()) {
        sf::Time dt = clock.restart();
        // Limit delta time to prevent physics glitches on lag spikes
        if (dt.asSeconds() > 0.05f) {
            dt = sf::seconds(0.05f);
        }
        processEvents();
        update(dt);
        render();
    }
}

/**
 * Processes user input events (keyboard, mouse, window close).
 */
void Game::processEvents() {
    sf::Event event;
    while (mWindow.pollEvent(event)) {
        if (event.type == sf::Event::Closed)
            mWindow.close();
        // --- 1. ESCAPE KEY LOGIC (IMPROVED) ---
        if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape) {
            if (mGameState == GameState::Playing) {
                // If there are open menus, ESC closes them FIRST instead of pausing
                if (mIsInventoryOpen || mIsFurnaceOpen || mIsChestOpen || mIsCraftingTableOpen) {
                    mIsInventoryOpen = false;
                    mIsFurnaceOpen = false;
                    mIsChestOpen = false;
                    mIsCraftingTableOpen = false;

                    // If you had an object floating on the mouse, it returns to the backpack
                    if (mDraggedItem.id != ItemID::AIR) {
                        addItemToBackpack(mDraggedItem.id, mDraggedItem.count);
                        mDraggedItem.id = ItemID::AIR;
                        mDraggedItem.count = 0;
                    }
                } else {
                    // If the screen is clean, we pause the game
                    mGameState = GameState::Paused;
                }
            } else if (mGameState == GameState::Paused) {
                mGameState = GameState::Playing; // Unpause
            } else if (mGameState == GameState::MainMenu) {
                mWindow.close(); // Exit from the main menu
            }
        }

        // --- 2. CLICKS IN THE MAIN MENU ---
        if (mGameState == GameState::MainMenu && event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
            sf::Vector2i mousePos = sf::Mouse::getPosition(mWindow);
            sf::Vector2f worldPos = mWindow.mapPixelToCoords(mousePos, mWindow.getDefaultView());

            if (mMenuPlayText.getGlobalBounds().contains(worldPos)) mGameState = GameState::Playing;
            if (mMenuExitText.getGlobalBounds().contains(worldPos)) mWindow.close();
        }

        // --- 3. CLICKS IN THE PAUSE MENU (NEW!) ---
        if (mGameState == GameState::Paused && event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
            sf::Vector2i mousePos = sf::Mouse::getPosition(mWindow);
            sf::Vector2f worldPos = mWindow.mapPixelToCoords(mousePos, mWindow.getDefaultView());

            // We simulate the collision box where we draw it in render() (Y = 650)
            sf::FloatRect exitBounds = mMenuExitText.getGlobalBounds();
            exitBounds.top = 650.0f - (exitBounds.height / 2.0f);

            if (exitBounds.contains(worldPos)) {
                mGameState = GameState::MainMenu; // Return to start
            }
        }

        // --- OPEN/CLOSE INVENTORY (Key E) ---
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

        if (event.type == sf::Event::Resized) {
            sf::FloatRect visibleArea(0.0f, 0.0f, event.size.width, event.size.height);
            mWindow.setView(sf::View(visibleArea));
        }

        // ==========================================================
        // DRAG & DROP SYSTEM (Now delegated to clean functions)
        // ==========================================================
        if (mGameState == GameState::Playing) {
            if (mIsInventoryOpen) {
                if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
                    sf::Vector2i pixelPos(event.mouseButton.x, event.mouseButton.y);
                    sf::View uiView(sf::FloatRect(0.0f, 0.0f, mWindow.getSize().x, mWindow.getSize().y));
                    sf::Vector2f worldPos = mWindow.mapPixelToCoords(pixelPos, uiView);
                    handleMouseClick(worldPos.x, worldPos.y);
                }
                else if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left) {
                    sf::Vector2i pixelPos(event.mouseButton.x, event.mouseButton.y);
                    sf::View uiView(sf::FloatRect(0.0f, 0.0f, mWindow.getSize().x, mWindow.getSize().y));
                    sf::Vector2f worldPos = mWindow.mapPixelToCoords(pixelPos, uiView);
                    handleMouseRelease(worldPos.x, worldPos.y);
                }
            }
        }
    }
    // Player movement and jump controls (ONLY IF PLAYING)
    if (mGameState == GameState::Playing) {
        mPlayer.handleInput(mIsInventoryOpen);
    }
}

// Helper function to determine hardness
float getBlockHardness(int id) {
    switch (id) {
        case ItemID::AIR: return 0.0f;
        case ItemID::DIRT: return 0.2f;  // (Very fast)
        case ItemID::GRASS: return 0.25f;
        case ItemID::WOOD: return 0.6f;
        case ItemID::LEAVES: return 0.1f;  // (Instant)
        case ItemID::TORCH: return 0.1f;

            // --- DOORS (Closed and Open) ---
        case ItemID::DOOR: case ItemID::DOOR_MID: case ItemID::DOOR_TOP:
        case ItemID::DOOR_OPEN: case ItemID::DOOR_OPEN_MID: case ItemID::DOOR_OPEN_TOP:
            return 0.5f;

            // --- TABLES AND FURNACES ---
        case ItemID::CRAFTING_TABLE: return 1.5f; // (Requires a few axe swings)
        case ItemID::FURNACE: return 2.0f; // (Hard, it's made of stone)
        case ItemID::CHEST: return 1.5f;

            // STONE AND MINERALS
        case ItemID::STONE: return 1.0f;  // (Standard)
        case ItemID::COAL: return 1.2f;
        case ItemID::COPPER: return 1.5f;
        case ItemID::IRON: return 2.0f;
        case ItemID::COBALT: return 3.0f; // (Hard)
        case ItemID::TUNGSTEN: return 5.0f; // (Very hard)

        case ItemID::BEDROCK: return -1.0f; // (Indestructible)

        default: return 1.0f;
    }
}

// --- NEW: PROGRESSION SYSTEM (TIERS) ---
// Returns the level of the pickaxe you have in your hand
int getPickaxeTier(int itemID) {
    switch (itemID) {
        case ItemID::WOOD_PICKAXE: return 1;
        case ItemID::STONE_PICKAXE: return 2;
        case ItemID::IRON_PICKAXE: return 3;
        case ItemID::TUNGSTEN_PICKAXE: return 4;
        default: return 0; // Fists or other items
    }
}

// Returns the MINIMUM level of pickaxe necessary to break a block
int getRequiredPickaxeTier(int blockID) {
    switch (blockID) {
        // Level 1 Blocks (Require Wood Pickaxe or higher)
        case ItemID::STONE:
        case ItemID::COAL:
        case ItemID::FURNACE: return 1;

            // Level 2 Blocks (Require Stone Pickaxe or higher)
        case ItemID::COPPER:
        case ItemID::IRON: return 2;

            // Level 3 Blocks (Require Iron Pickaxe or higher)
        case ItemID::COBALT:
        case ItemID::TUNGSTEN: return 3; // FIXED! Now Iron (Tier 3) can mine it

        case ItemID::BEDROCK: return 999; // Impossible

            // Dirt, wood, leaves, chests, tables, etc. can be broken with fists (Level 0)
        default: return 0;
    }
}

// ---------------------------------------------------------
// UPDATE (LOGIC)
// ---------------------------------------------------------
/**
 * Updates the game state.
 * Handles player movement, world updates, physics, interactions (mining, building, combat),
 * day/night cycle, and entity logic (Dodos, Troodons).
 * @param dt Delta time (time elapsed since last frame).
 */
void Game::update(sf::Time dt) {
    if (mGameState != GameState::Playing) return;
    // Update day and night cycle
    updateDayNightCycle(dt.asSeconds());
    // ==================================================
    // 1. DEATH LOGIC (BLOCKS THE REST OF THE GAME)
    // ==================================================
    if (mIsPlayerDead) {
        mRespawnTimer -= dt.asSeconds();

        // We update the countdown text
        int secondsLeft = static_cast<int>(std::ceil(mRespawnTimer));
        mDeathSubText.setString("Respawning in " + std::to_string(secondsLeft) + " seconds...");

        // If the time runs out, we REVIVE
        if (mRespawnTimer <= 0.0f) {
            respawnPlayer();
        }

        return; // We exit the function
    }


    // ==================================================
    // 2. CHECK IF WE JUST DIED
    // ==================================================
    if (mPlayer.getHp() <= 0) {
        mIsPlayerDead = true;
        mRespawnTimer = 5.0f; // 5 Seconds of waiting

        // Penalty: You lose half of your meat!
        int meatCount = getItemCount(50);
        if (meatCount > 0) {
            int meatToLose = meatCount / 2;
            if (meatToLose > 0) {
                consumeItem(50, meatToLose);
                std::cout << "You died and lost " << meatToLose << " pieces of meat..." << std::endl;
            }
        }

        // Dramatic death sound
        mSndBreak.setPitch(0.4f);
        mSndBreak.play();

        return; // We exit so the player stops moving and falls to the ground
    }

    // ==================================================
    // 2. FURNACE LOGIC (MULTIPLE FURNACES)
    // ==================================================
    for (auto& pair : mActiveFurnaces) {
        FurnaceData& fd = pair.second;

        // 1. What mineral are we trying to smelt?
        int resultItem = 0;
        if (fd.input.id == ItemID::COPPER) resultItem = ItemID::COPPER_INGOT;
        else if (fd.input.id == ItemID::IRON) resultItem = ItemID::IRON_INGOT;
        else if (fd.input.id == ItemID::COBALT) resultItem = ItemID::COBALT_INGOT;
        else if (fd.input.id == ItemID::TUNGSTEN) resultItem = ItemID::TUNGSTEN_INGOT;

        // Can we cook? (There is mineral, and the output is empty or has space for the same ingot)
        bool canCook = (resultItem != 0 && fd.input.count > 0 &&
                       (fd.output.id == 0 || (fd.output.id == resultItem && fd.output.count < 99)));

        // 2. Fuel Consumption Logic (Wood or Coal)
        if (canCook && fd.fuelTimer <= 0.0f) {
            // We accept Wood or Coal
            if ((fd.fuel.id == ItemID::WOOD || fd.fuel.id == ItemID::COAL) && fd.fuel.count > 0) {
                int fuelType = fd.fuel.id;

                fd.fuel.count--;
                if (fd.fuel.count == 0) fd.fuel.id = 0;

                // Coal lasts 40 seconds, wood only 10
                fd.maxFuelTimer = (fuelType == ItemID::COAL) ? 40.0f : 10.0f;
                fd.fuelTimer = fd.maxFuelTimer;
            }
        }

        // 3. Smelting Process
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
                fd.smeltTimer = 0.0f; // It pauses if you take out the mineral
            }
        } else {
            fd.smeltTimer = 0.0f; // It pauses if the fire goes out
        }
    }

    mPlayer.update(dt, mWorld);

    // We recalculate the weight constantly to avoid bugs
    calculateTotalWeight();
    mPlayer.setOverweight(mCurrentWeight > mMaxWeight);

    // We pass the ID of the item equipped in the right hand to the player
    mPlayer.setEquippedWeapon(mSelectedBlock);

    // --- UPDATE WORLD ITEMS ---
    // 1. Create a temporary inventory for this frame
    std::map<int, int> pickedUpItems;

    // 2. The world puts here what the player picks up from the ground
    mWorld.update(dt, mPlayer.getCenter(), pickedUpItems);

    // 3. Transfer picked up items to our new weight/backpack system
    for (const auto& item : pickedUpItems) {
        int id = item.first;
        int cantidad = item.second;

        // Try to insert it. (If backpack is full, it would be lost for now,
        // but we will improve it in Phase 2)
        addItemToBackpack(id, cantidad);
    }

    // --- TACTICAL WHEEL SELECTION (Keys 1, 2, 3, 4) ---
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num1)) mActiveWheelSlot = 3; // Right (Weapon 1)
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num2)) mActiveWheelSlot = 2; // Left (Weapon 2)
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num3)) mActiveWheelSlot = 0; // Up (Usable)
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num4)) mActiveWheelSlot = 1; // Down (Blocks)

    // Constantly update mSelectedBlock looking at what is in the wheel
    InventorySlot* wheel[4] = { &mEquippedConsumable, &mEquippedBlock, &mEquippedSecondary, &mEquippedPrimary };
    mSelectedBlock = wheel[mActiveWheelSlot]->id;

    // CAMERA
    sf::View view = mWindow.getDefaultView();
    view.zoom(1.25f);
    view.setCenter(mPlayer.getPosition());
    mWindow.setView(view);

    // --- SUMMON DODO (Key O) ---
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::O)) {
        // 1. Calculate where the mouse is
        sf::Vector2i mousePos = sf::Mouse::getPosition(mWindow);
        sf::Vector2f worldPos = mWindow.mapPixelToCoords(mousePos, mWindow.getView());

        // 2. Convert to grid coordinates
        int gX = static_cast<int>(std::floor(worldPos.x / mWorld.getTileSize()));
        int gY = static_cast<int>(std::floor(worldPos.y / mWorld.getTileSize()));

        // 3. Check if it is air (0) and create it
        if (mWorld.getBlock(gX, gY) == 0) {
            mMobs.push_back(std::make_unique<Dodo>(worldPos, mDodoTexture));
            sf::sleep(sf::milliseconds(200));
        }
    }

    // --- SUMMON TROODON (Key P) ---
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::P)) {
        // 1. Calculate where the mouse is
        sf::Vector2i mousePos = sf::Mouse::getPosition(mWindow);
        sf::Vector2f worldPos = mWindow.mapPixelToCoords(mousePos, mWindow.getView());

        // 2. Convert to grid coordinates
        int gX = static_cast<int>(std::floor(worldPos.x / mWorld.getTileSize()));
        int gY = static_cast<int>(std::floor(worldPos.y / mWorld.getTileSize()));

        // 3. Check if it is air (0) and create it
        if (mWorld.getBlock(gX, gY) == 0) {
            mMobs.push_back(std::make_unique<Troodon>(worldPos, mTroodonTexture));
            sf::sleep(sf::milliseconds(200));
        }
    }

    // --- RANGE AND INTERACTION LOGIC ---
    sf::Vector2i pixelPos = sf::Mouse::getPosition(mWindow);
    sf::Vector2f worldPos = mWindow.mapPixelToCoords(pixelPos);
    float tileSize = mWorld.getTileSize();

    // 1. DETERMINE WHICH BLOCK WE ARE LOOKING AT
    int gridX = static_cast<int>(std::floor(worldPos.x / tileSize));
    int gridY = static_cast<int>(std::floor(worldPos.y / tileSize));



    // 2. CALCULATE DISTANCE TO THE CENTER OF THAT BLOCK
    float blockCenterX = (gridX * tileSize) + (tileSize / 2.0f);
    float blockCenterY = (gridY * tileSize) + (tileSize / 2.0f);
    sf::Vector2f playerCenter = mPlayer.getPosition();

    float dx = blockCenterX - playerCenter.x;
    float dy = blockCenterY - playerCenter.y;
    float distance = std::sqrt(dx*dx + dy*dy);
    float maxRange = 180.0f;

    // ==================================================
    // 1. COMBAT SYSTEM (Melee y Ranged)
    // ==================================================
    if (mPlayer.canDealDamage()) {
        int equippedID = mSelectedBlock;

        // --- SISTEMA DE ARCO (RANGED) ---
        if (equippedID == ItemID::BOW) {

            // ¿Tenemos flechas en el inventario? (Consume 1)
            if (consumeItem(ItemID::ARROW, 1)) {

                // Calculamos la dirección del ratón
                sf::Vector2i mousePixel = sf::Mouse::getPosition(mWindow);
                sf::Vector2f mouseWorld = mWindow.mapPixelToCoords(mousePixel);
                sf::Vector2f playerPos = mPlayer.getCenter();

                float dx = mouseWorld.x - playerPos.x;
                float dy = mouseWorld.y - playerPos.y;
                float length = std::sqrt(dx*dx + dy*dy);

                // ¡VELOCIDAD ABSURDA PARA QUE VIAJEN RÁPIDO!
                float speed = 1800.0f;
                sf::Vector2f velocity((dx / length) * speed, (dy / length) * speed);

                // Creamos la flecha (le pasamos la textura de la flecha del inventario, ID 36)
                mProjectiles.push_back(std::make_unique<Projectile>(playerPos, velocity, *mWorld.getTexture(36)));

                // Sonido de disparo (reusamos un sonido pero más agudo)
                mSndBuild.setPitch(2.0f);
                mSndBuild.play();
            } else {
                std::cout << "¡No te quedan flechas!" << std::endl;
            }
            mPlayer.registerHit(); // Seguro para no disparar 60 flechas a la vez
        }
        // --- SISTEMA DE ESPADAS/PICOS (MELEE) ---
        else {
            int toolDamage = 1;
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
                    break;
                }
            }
        }
    }

    // ==================================================
    // 2. MINING SYSTEM (Depends on keeping the Click pressed)
    // ==================================================
    if (!mIsInventoryOpen && mSelectedBlock != ItemID::BOW) {
        if (sf::Mouse::isButtonPressed(sf::Mouse::Left)) {

            // --- NEW! DISTANCE SAFETY CATCH IS BACK ---
            // We only start mining if the mouse is close enough
            if (distance <= maxRange) {

                // Have we changed blocks?
                if (mMiningPos.x != gridX || mMiningPos.y != gridY) {
                    mMiningTimer = 0.0f;
                    mMiningPos = sf::Vector2i(gridX, gridY);

                    // When you left click on a block to start mining:
                    int blockID = mWorld.getBlock(gridX, gridY);

                    if (blockID != 0) {
                        mMiningPos = sf::Vector2i(gridX, gridY);
                        mMiningTimer = 0.0f;

                        // --- NEW: DYNAMIC LOGGING SYSTEM ---
                        if (blockID == ItemID::WOOD) { // If it's Wood
                            int blocksAbove = 0;
                            int checkY = gridY;

                            // We count how many wood blocks or leaves are directly above
                            while (mWorld.getBlock(gridX, checkY) == ItemID::WOOD || mWorld.getBlock(gridX, checkY) == ItemID::LEAVES) {
                                blocksAbove++;
                                checkY--; // We look up
                            }

                            // Each block adds 0.2s to the mining time (Base 0.6s)
                            mCurrentHardness = 0.2f * blocksAbove;
                            if (mCurrentHardness < 0.6f) mCurrentHardness = 0.6f;
                        }
                        else {
                            // Normal hardness for the rest of the blocks
                            mCurrentHardness = getBlockHardness(blockID); // Or however you have it programmed
                        }
                    }
                }

                // Mine
                if (mCurrentHardness > 0.0f) {
                    float miningSpeed = 1.0f;
                    int equippedID = mSelectedBlock;
                    int targetID = mWorld.getBlock(gridX, gridY);

                    // 1. Check if our pickaxe has enough level
                    int pickaxeTier = getPickaxeTier(equippedID);
                    int requiredTier = getRequiredPickaxeTier(targetID);

                    if (pickaxeTier < requiredTier) {
                        // The pickaxe is not good enough for this material
                        miningSpeed = 0.0f;
                    } else {
                        // 2. If we can mine it, we apply the speed of our pickaxe
                        if (equippedID == ItemID::WOOD_PICKAXE) miningSpeed = 3.0f;
                        else if (equippedID == ItemID::STONE_PICKAXE) miningSpeed = 5.0f;
                        else if (equippedID == ItemID::IRON_PICKAXE) miningSpeed = 10.0f;
                        else if (equippedID == ItemID::TUNGSTEN_PICKAXE) miningSpeed = 20.0f;

                        // --- NEW: SWORDS ARE BETTER THAN FISTS ---
                        // Fists have a speed of 1.0f. We give swords 1.5f.
                        else if (equippedID >= ItemID::WOOD_SWORD && equippedID <= ItemID::TUNGSTEN_SWORD) {
                            miningSpeed = 1.5f;
                        }
                    }

                    // We add the progress (if miningSpeed is 0, the bar doesn't advance)
                    mMiningTimer += dt.asSeconds() * miningSpeed;

                    // The block has been broken!
                    if (mMiningTimer >= mCurrentHardness) {
                        int brokenBlockID = mWorld.getBlock(mMiningPos.x, mMiningPos.y);

                        // --- PARTICLE EXPLOSION! ---
                        float tileSize = mWorld.getTileSize();
                        sf::Vector2f blockCenter(mMiningPos.x * tileSize + tileSize/2.0f, mMiningPos.y * tileSize + tileSize/2.0f);
                        spawnParticles(blockCenter, brokenBlockID, 12); // Shoots 12 particles

                        // --- NEW: TREE DOMINO EFFECT ---
                        if (brokenBlockID == ItemID::WOOD) { // If we just broke Wood
                            int currY = mMiningPos.y;

                            // While the current block is wood or leaves...
                            while (mWorld.getBlock(mMiningPos.x, currY) == ItemID::WOOD ||
                                   mWorld.getBlock(mMiningPos.x, currY) == ItemID::LEAVES) {

                                int currentBlock = mWorld.getBlock(mMiningPos.x, currY);

                                // 1. Break the central block (Trunk or leaf)
                                mWorld.setBlock(mMiningPos.x, currY, 0);

                                // --- DROP THE ITEM IN THE WORLD ---
                                // We use your native function that already does the physical jumps
                                mWorld.spawnItem(mMiningPos.x, currY, currentBlock);

                                // 2. Clear the surrounding leaves (3x3 Radius)
                                for(int ox = -2; ox <= 2; ++ox) {
                                    for(int oy = -2; oy <= 2; ++oy) {
                                        if(mWorld.getBlock(mMiningPos.x + ox, currY + oy) == ItemID::LEAVES) { // If it's a leaf
                                            mWorld.setBlock(mMiningPos.x + ox, currY + oy, 0);
                                            // 15% chance to drop extra leaves (simulating sprouts/saplings)
                                            if (rand() % 100 < 15) mWorld.spawnItem(mMiningPos.x + ox, currY + oy, ItemID::LEAVES);
                                        }
                                    }
                                }

                                // We go up a level to destroy the next block
                                currY--;
                            }
                        }
                        else {
                            // Normal behavior for Stone, Dirt, etc.
                            mWorld.setBlock(mMiningPos.x, mMiningPos.y, 0);

                            // We drop the item to the ground using your native function
                            mWorld.spawnItem(mMiningPos.x, mMiningPos.y, brokenBlockID);
                        }

                        // We add a sound for when it breaks
                        mSndBreak.setPitch(0.8f + (rand() % 40) / 100.0f);
                        mSndBreak.play();

                        // We reset the mining
                        mCurrentHardness = 0.0f;
                        mMiningTimer = 0.0f;
                    }
                }
                else if (mCurrentHardness == -1.0f) {
                    mMiningTimer = 0.0f;
                }

            } else {
                // If we keep pressed but the mouse is VERY FAR, we reset the mining
                mMiningTimer = 0.0f;
                mMiningPos = sf::Vector2i(-1, -1);
            }

        }
        else {
            // Left button released: Reset mining
            mMiningTimer = 0.0f;
            mMiningPos = sf::Vector2i(-1, -1);
        }
    }

    // --------------------------------------------------
    // UPDATE ACTION TIMER
    // --------------------------------------------------
    if (mActionTimer > 0.0f) {
        mActionTimer -= dt.asSeconds(); // Time goes down each frame
    }

    // --------------------------------------------------
    // RIGHT CLICK: PLACE BLOCKS OR EAT!
    // --------------------------------------------------
    if (sf::Mouse::isButtonPressed(sf::Mouse::Right)) {

        // We can ONLY perform an action if the timer has reached 0
        if (mActionTimer <= 0.0f) {

            // --- 0. TRY TO INTERACT FIRST ---
            // If the function returns 'true', it's because we've opened a furnace or a door.
            // If so, we don't eat or place blocks.
            if (interactWithBlock(gridX, gridY)) {
                mActionTimer = 0.3f; // Pause so as not to open and close the door at the speed of light
            }
            // --- 1. IF THERE WAS NO INTERACTION, IS IT FOOD? ---
            else if (mSelectedBlock == ItemID::MEAT) {
                if (mPlayer.getHp() < mPlayer.getMaxHp() && wheel[mActiveWheelSlot]->count > 0) {

                    wheel[mActiveWheelSlot]->count--; // Spend from the wheel
                    if (wheel[mActiveWheelSlot]->count == 0) wheel[mActiveWheelSlot]->id = 0;

                    mPlayer.heal(20);
                    mSndBuild.setPitch(0.5f);
                    mSndBuild.play();
                    mActionTimer = 0.25f;
                }
            }
            else if (distance <= maxRange) {
                // --- CORRECCIÓN: Permitimos colocar hasta la Nieve ---
                bool isPlaceableBlock = ((mSelectedBlock >= ItemID::DIRT && mSelectedBlock <= ItemID::SNOW) ||
                                          mSelectedBlock == ItemID::DOOR || mSelectedBlock == ItemID::CRAFTING_TABLE ||
                                          mSelectedBlock == ItemID::FURNACE || mSelectedBlock == ItemID::CHEST);

                // We check if there is any block touching in a cross shape
                bool hasAdjacentBlock = (mWorld.getBlock(gridX - 1, gridY) != 0) ||
                                        (mWorld.getBlock(gridX + 1, gridY) != 0) ||
                                        (mWorld.getBlock(gridX, gridY - 1) != 0) ||
                                        (mWorld.getBlock(gridX, gridY + 1) != 0);

                if (isPlaceableBlock && wheel[mActiveWheelSlot]->count > 0) {

                    // =======================================
                    // SPECIAL CASE: PLACE DOOR (ID 25)
                    // =======================================
                    if (mSelectedBlock == ItemID::DOOR) {
                        if (mWorld.getBlock(gridX, gridY) == ItemID::AIR &&
                            mWorld.getBlock(gridX, gridY - 1) == ItemID::AIR &&
                            mWorld.getBlock(gridX, gridY - 2) == ItemID::AIR &&
                            World::isSolid(mWorld.getBlock(gridX, gridY + 1)))
                        {
                            mWorld.setBlock(gridX, gridY, ItemID::DOOR);     // Down
                            mWorld.setBlock(gridX, gridY - 1, ItemID::DOOR_MID); // Middle
                            mWorld.setBlock(gridX, gridY - 2, ItemID::DOOR_TOP); // Up

                            wheel[mActiveWheelSlot]->count--;
                            if (wheel[mActiveWheelSlot]->count == 0) wheel[mActiveWheelSlot]->id = 0;

                            mSndBuild.setPitch(1.0f + (rand() % 20) / 100.0f);
                            mSndBuild.play();
                            mActionTimer = 0.15f;
                            calculateTotalWeight();
                        }
                    }
                    // =======================================
                    // NORMAL CASE: ANY OTHER BLOCK
                    // =======================================
                    else if (mWorld.getBlock(gridX, gridY) == 0 && hasAdjacentBlock) {
                        sf::FloatRect blockRect(gridX * tileSize, gridY * tileSize, tileSize, tileSize);

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
        }// End of timer check
    }

    // --------------------------------------------------
    // SAVE SYSTEM (F5 / F6)
    // --------------------------------------------------
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::F5)) {
        saveGame();
        sf::sleep(sf::milliseconds(300));
    }

    if (sf::Keyboard::isKeyPressed(sf::Keyboard::F6)) {
        loadGame();
        sf::sleep(sf::milliseconds(300));
    }

    // --------------------------------------------------
    // UPDATE ALL MOBS (AI, DAMAGE AND CORPSES)
    // --------------------------------------------------

    // ==========================================
    // AUTOMATIC CREATURE SPAWNER (ROBUSTO)
    // ==========================================
    mSpawnTimer -= dt.asSeconds();

    if (mSpawnTimer <= 0.0f) {
        // Reiniciamos el reloj: entre 10 y 20 segundos hasta el próximo spawn
        mSpawnTimer = 10.0f + (rand() % 10);

        // Solo spawneamos si no hemos llegado al límite de población
        if (mMobs.size() < MAX_MOBS) {

            // 1. DISTANCIA: Muy lejos (Entre 1500 y 2500 píxeles de distancia)
            float playerX = mPlayer.getPosition().x;
            float spawnDistance = 1500.0f + (rand() % 1000);

            float spawnX;
            if (rand() % 2 == 0) {
                spawnX = playerX - spawnDistance; // Izquierda
            } else {
                spawnX = playerX + spawnDistance; // Derecha
            }

            // 2. Encontrar el suelo real (Y) en esa coordenada X
            float tileSize = mWorld.getTileSize();
            int gridX = std::max(0, static_cast<int>(spawnX / tileSize));

            float spawnY = 0.0f;
            bool groundFound = false;

            // Lanzamos un raycast desde el cielo hacia abajo
            for (int y = 1; y < WORLD_HEIGHT; ++y) {
                int blockID = mWorld.getBlock(gridX, y);

                // ¡CORRECCIÓN! Solo nos detenemos si el bloque es SÓLIDO (Tierra, piedra, etc.)
                // Ignoramos hojas, madera o antorchas.
                if (World::isSolid(blockID)) {

                    // Hemos encontrado suelo firme. ¿Hay espacio para que nazca el bicho?
                    // Escaneamos un área de 3x3 bloques JUSTO ENCIMA de este suelo
                    bool isSpaceClear = true;

                    for (int checkX = gridX - 1; checkX <= gridX + 1; ++checkX) {
                        for (int checkY = y - 3; checkY <= y - 1; ++checkY) {
                            if (checkY >= 0) {
                                int aboveBlock = mWorld.getBlock(checkX, checkY);
                                // Si hay cualquier bloque sólido estorbando, cancelamos
                                if (World::isSolid(aboveBlock)) {
                                    isSpaceClear = false;
                                    break;
                                }
                            }
                        }
                        if (!isSpaceClear) break;
                    }

                    if (isSpaceClear) {
                        // ¡Sitio perfecto y amplio! Lo ponemos 2 bloques por encima del suelo para que caiga suave
                        spawnY = (y - 2) * tileSize;
                        groundFound = true;
                    }

                    // Independientemente de si había espacio o no, rompemos el bucle.
                    // Ya hemos tocado el suelo, no queremos seguir buscando bajo tierra y que spawneen en cuevas cerradas.
                    break;
                }
            }

            // 3. ¡HÁGASE LA LUZ! (O la oscuridad)
            if (groundFound) {
                bool isNight = (mGameTime > (DAY_LENGTH * 0.5f));
                sf::Vector2f spawnPos(spawnX, spawnY);

                if (isNight) {
                    mMobs.push_back(std::make_unique<Troodon>(spawnPos, mTroodonTexture));
                    std::cout << "Un Troodon acecha en la oscuridad en X: " << gridX << std::endl;
                } else {
                    mMobs.push_back(std::make_unique<Dodo>(spawnPos, mDodoTexture));
                    std::cout << "Un Dodo salvaje ha aparecido en X: " << gridX << std::endl;
                }
            }
        }
    }

    float currentBrightness = (mGameTime > (DAY_LENGTH * 0.5f)) ? 0.0f : 1.0f;

    for (auto it = mMobs.begin(); it != mMobs.end(); ) {

        auto& mob = **it; // Extract the object to make it easier to read

        mob.update(dt, mPlayer.getPosition(), mWorld, currentBrightness);

        // --- DOES THE MOB TOUCH THE PLAYER? ---
        if (!mob.isDead() && mob.getBounds().intersects(mPlayer.getGlobalBounds())) {
            float dir = (mPlayer.getPosition().x > mob.getPosition().x) ? 1.0f : -1.0f;

            // We pass the specific damage of that animal!
            if (mPlayer.takeDamage(mob.getDamage(), dir)) {
                mSndHit.setPitch(0.7f);
                mSndHit.play();
            }
        }

        // --- REMOVE CORPSES ---
        if (mob.isDead()) {
            mWorld.spawnItem(50, mob.getPosition()); // Drops meat
            mSndBreak.setPitch(1.5f);
            mSndBreak.play();
            it = mMobs.erase(it); // Automatically deleted from memory
        } else {
            ++it;
        }
    }

    // ==================================================
    // UPDATE PARTICLES (Gravity and Death)
    // ==================================================
    for (auto it = mParticles.begin(); it != mParticles.end(); ) {
        it->velocity.y += 1200.0f * dt.asSeconds(); // Gravity Force
        it->position += it->velocity * dt.asSeconds();
        it->lifetime -= dt.asSeconds();

        if (it->lifetime <= 0.0f) {
            it = mParticles.erase(it); // It disintegrates
        } else {
            ++it;
        }
    }

    // ==================================================
    // UPDATE PROJECTILES
    // ==================================================
    for (auto it = mProjectiles.begin(); it != mProjectiles.end(); ) {
        auto& proj = **it;
        proj.update(dt, mWorld);

        // Si la flecha no ha tocado una pared, comprobamos si toca a un monstruo
        if (!proj.isDead()) {
            for (auto& mob : mMobs) {
                if (!mob->isDead() && proj.getBounds().intersects(mob->getBounds())) {
                    // Calculamos la dirección del impacto basada en la velocidad de la flecha
                    float dir = (proj.getVelocity().x > 0) ? 1.0f : -1.0f;

                    if (mob->takeDamage(proj.getDamage(), dir)) {
                        mSndHit.setPitch(1.2f);
                        mSndHit.play();
                        spawnParticles(mob->getPosition(), ItemID::MEAT, 10);
                        proj.kill(); // La flecha desaparece al clavarse en el monstruo
                        break;
                    }
                }
            }
        }

        if (proj.isDead()) {
            it = mProjectiles.erase(it);
        } else {
            ++it;
        }
    }
}

// ---------------------------------------------------------
// RENDER (DRAWING)
// ---------------------------------------------------------
/**
 * Renders the game to the window.
 * Draws the sky, world, player, entities, UI (inventory, health bar), and other visual elements.
 */
void Game::render() {
    mWindow.clear(mAmbientLight);

    if (mGameState == GameState::MainMenu) {
        // DRAW MAIN MENU
        mWindow.setView(mWindow.getDefaultView());
        mSkySprite.setColor(sf::Color::White); // Clear sky in background
        mWindow.draw(mSkySprite);
        mWindow.draw(mMenuTitleText);
        mWindow.draw(mMenuPlayText);
        mWindow.draw(mMenuExitText);
    }
    else {
        // 1. We clear the window with ambient light
        mWindow.clear(mAmbientLight);

        // ==========================================
        // 1. LIGHT AND SKY CALCULATION (We keep your original logic)
        // ==========================================
        sf::Color skyLight = mAmbientLight;
        sf::Vector2f centerPos = mPlayer.getCenter();
        int pGridX = static_cast<int>(centerPos.x / mWorld.getTileSize());
        int pGridY = static_cast<int>(centerPos.y / mWorld.getTileSize());

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
        float caveFactor = (depth > 0) ? std::min(depth / 20.0f, 1.0f) : 0.0f;

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

        // PLAYER LIGHT (Torches)
        sf::Color playerColor = finalAmbient;
        sf::Vector2f playerPos = mPlayer.getCenter();
        float searchRadius = 10.0f;
        float lightRadius = 250.0f;
        float maxTorchIntensity = 0.0f;

        for (int x = pGridX - searchRadius; x <= pGridX + searchRadius; ++x) {
            for (int y = pGridY - searchRadius; y <= pGridY + searchRadius; ++y) {
                if (mWorld.getBlock(x, y) == ItemID::TORCH) { // Using your new Enum!
                    float torchX = x * mWorld.getTileSize() + mWorld.getTileSize()/2.0f;
                    float torchY = y * mWorld.getTileSize() + mWorld.getTileSize()/2.0f;
                    float dist = std::sqrt(std::pow(playerPos.x - torchX, 2) + std::pow(playerPos.y - torchY, 2));
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

        // ==========================================
        // 2. DRAW WORLD AND ENTITIES
        // ==========================================
        mWorld.render(mWindow, finalAmbient);
        mPlayer.render(mWindow, playerColor);

        for (auto& mob : mMobs) {
            mob->render(mWindow, finalAmbient);
        }

        for (auto& mob : mMobs) {
            mob->render(mWindow, finalAmbient);
        }

        // Dibujar proyectiles
        for (auto& proj : mProjectiles) {
            proj->render(mWindow, finalAmbient);
        }

        // ==========================================
        // DRAW PARTICLES
        // ==========================================
        sf::RectangleShape pShape;
        for (const auto& p : mParticles) {
            pShape.setSize(sf::Vector2f(p.size, p.size));
            pShape.setPosition(p.position);

            sf::Color c = p.color;
            // Fade Out Effect: They become transparent when dying
            c.a = static_cast<sf::Uint8>(255.0f * (p.lifetime / p.maxLifetime));

            // Magic! We darken them with finalAmbient so they don't shine in caves
            c.r = (c.r * finalAmbient.r) / 255;
            c.g = (c.g * finalAmbient.g) / 255;
            c.b = (c.b * finalAmbient.b) / 255;

            pShape.setFillColor(c);
            mWindow.draw(pShape);
        }

        // ==========================================
        // 3. DRAW INTERFACE (UI)
        // ==========================================
        if (mIsPlayerDead) {
            renderDeathScreen();
        } else {
            renderHUD();    // Cursor, mining bar, health, item in hand
            renderMenus();  // Inventory, Furnaces, Chests, Crafting
        }
        if (mGameState == GameState::Paused) {
            mWindow.setView(mWindow.getDefaultView());
            sf::RectangleShape darkOverlay(sf::Vector2f(1920, 1080));
            darkOverlay.setFillColor(sf::Color(0, 0, 0, 150));
            mWindow.draw(darkOverlay);
            mWindow.draw(mPauseTitleText);

            // We reuse the Exit button from the main menu
            mMenuExitText.setPosition(1920 / 2.0f, 650.0f);
            mWindow.draw(mMenuExitText);
            mMenuExitText.setPosition(1920 / 2.0f, 750.0f); // We return it to its original place for later
        }
    }
    mWindow.display();
}

/**
 * Saves the current game state to a file ("savegame.dat").
 * Serializes player position, inventory, and world data.
 */
void Game::saveGame() {
    std::ofstream file("savegame.dat", std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Error: Could not create save file." << std::endl;
        return;
    }

    // 1. SAVE PLAYER (Position X, Y)
    sf::Vector2f pos = mPlayer.getPosition();
    file.write(reinterpret_cast<const char*>(&pos), sizeof(pos));

    // 2. SAVE INVENTORY (NEW SYSTEM)
    // Save backpack size (should be 30)
    size_t backpackSize = mBackpack.size();
    file.write(reinterpret_cast<const char*>(&backpackSize), sizeof(backpackSize));

    // Save ALL slots, even empty ones (ID 0) to maintain order
    for (const auto& slot : mBackpack) {
        int id = slot.id;
        int count = slot.count;
        file.write(reinterpret_cast<const char*>(&id), sizeof(id));
        file.write(reinterpret_cast<const char*>(&count), sizeof(count));
    }

    // 2.5 SAVE EQUIPPED ITEMS (Tactical Wheel)
    InventorySlot equipped[4] = { mEquippedPrimary, mEquippedSecondary, mEquippedBlock, mEquippedConsumable };
    for (int i = 0; i < 4; ++i) {
        file.write(reinterpret_cast<const char*>(&equipped[i].id), sizeof(equipped[i].id));
        file.write(reinterpret_cast<const char*>(&equipped[i].count), sizeof(equipped[i].count));
    }


    // 3. SAVE WORLD
    mWorld.saveToStream(file);

    // 4. SAVE FURNACES (NEW!)
    size_t furnaceCount = mActiveFurnaces.size();
    file.write(reinterpret_cast<const char*>(&furnaceCount), sizeof(furnaceCount));

    for (const auto& pair : mActiveFurnaces) {
        file.write(reinterpret_cast<const char*>(&pair.first.first), sizeof(pair.first.first));
        file.write(reinterpret_cast<const char*>(&pair.first.second), sizeof(pair.first.second));
        file.write(reinterpret_cast<const char*>(&pair.second), sizeof(FurnaceData));
    }

    // 5. SAVE CHESTS
    size_t chestCount = mActiveChests.size();
    file.write(reinterpret_cast<const char*>(&chestCount), sizeof(chestCount));

    for (const auto& pair : mActiveChests) {
        file.write(reinterpret_cast<const char*>(&pair.first.first), sizeof(pair.first.first));
        file.write(reinterpret_cast<const char*>(&pair.first.second), sizeof(pair.first.second));

        // We save the 24 slots of each chest
        for (const auto& slot : pair.second.slots) {
            file.write(reinterpret_cast<const char*>(&slot.id), sizeof(slot.id));
            file.write(reinterpret_cast<const char*>(&slot.count), sizeof(slot.count));
        }
    }

    file.close();
    std::cout << "--- GAME SAVED SUCCESSFULLY ---" << std::endl;
}

/**
 * Loads the game state from a file ("savegame.dat").
 * Deserializes player position, inventory, and world data.
 */
void Game::loadGame() {
    std::ifstream file("savegame.dat", std::ios::binary);
    if (!file.is_open()) {
        std::cout << "No save game found." << std::endl;
        return;
    }

    // 1. LOAD PLAYER
    sf::Vector2f pos;
    file.read(reinterpret_cast<char*>(&pos), sizeof(pos));
    mPlayer.setPosition(pos);

    // 2. LOAD INVENTORY (NEW SYSTEM)
    size_t backpackSize = 0;
    file.read(reinterpret_cast<char*>(&backpackSize), sizeof(backpackSize));

    // Adjust our backpack to the saved size (for safety)
    mBackpack.resize(backpackSize);

    // Read each slot and put it in our backpack
    for (size_t i = 0; i < backpackSize; ++i) {
        int id = 0, count = 0;
        file.read(reinterpret_cast<char*>(&id), sizeof(id));
        file.read(reinterpret_cast<char*>(&count), sizeof(count));
        mBackpack[i].id = id;
        mBackpack[i].count = count;
    }

    // 2.5 LOAD EQUIPPED ITEMS (Tactical Wheel)
    InventorySlot* equippedPointers[4] = { &mEquippedPrimary, &mEquippedSecondary, &mEquippedBlock, &mEquippedConsumable };
    for (int i = 0; i < 4; ++i) {
        file.read(reinterpret_cast<char*>(&equippedPointers[i]->id), sizeof(equippedPointers[i]->id));
        file.read(reinterpret_cast<char*>(&equippedPointers[i]->count), sizeof(equippedPointers[i]->count));
    }

    mPlayer.setEquippedWeapon(0);

    // 3. LOAD WORLD
    mWorld.loadFromStream(file);

    // 4. LOAD FURNACES (NEW!)
    mActiveFurnaces.clear(); // We clear current furnaces just in case
    size_t furnaceCount = 0;

    // We read how many furnaces there are (if this read fails, it's because it's an old save)
    if (file.read(reinterpret_cast<char*>(&furnaceCount), sizeof(furnaceCount))) {
        for (size_t i = 0; i < furnaceCount; ++i) {
            int fx, fy;
            FurnaceData fd;

            file.read(reinterpret_cast<char*>(&fx), sizeof(fx));
            file.read(reinterpret_cast<char*>(&fy), sizeof(fy));
            file.read(reinterpret_cast<char*>(&fd), sizeof(FurnaceData));

            mActiveFurnaces[{fx, fy}] = fd;
        }
    }

    // 5. LOAD CHESTS
    mActiveChests.clear();
    size_t chestCount = 0;

    if (file.read(reinterpret_cast<char*>(&chestCount), sizeof(chestCount))) {
        for (size_t i = 0; i < chestCount; ++i) {
            int cx, cy;
            ChestData cd;

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

void Game::calculateTotalWeight() {
    float oldWeight = mCurrentWeight;
    mCurrentWeight = 0.0f;

    // 1. Add what is in the Backpack
    for (const auto& slot : mBackpack) {
        if (slot.id != 0) {
            mCurrentWeight += mItemDatabase[slot.id].weight * slot.count;
        }
    }

    // 2. Add what is in the Tactical Wheel
    InventorySlot* wheel[4] = { &mEquippedPrimary, &mEquippedSecondary, &mEquippedBlock, &mEquippedConsumable };
    for (int i = 0; i < 4; ++i) {
        if (wheel[i]->id != 0) {
            mCurrentWeight += mItemDatabase[wheel[i]->id].weight * wheel[i]->count;
        }
    }

    // 3. Add what you have grabbed with the mouse (So they don't cheat by floating items!)
    if (mDraggedItem.id != 0) {
        mCurrentWeight += mItemDatabase[mDraggedItem.id].weight * mDraggedItem.count;
    }

    // We only print if the weight has changed
    if (std::abs(mCurrentWeight - oldWeight) > 0.01f) {
        std::cout << "Current weight: " << mCurrentWeight << " / " << mMaxWeight << std::endl;
    }
}

bool Game::addItemToBackpack(int id, int amount) {
    // 1. FIRST: Try to stack in the TACTICAL WHEEL
    InventorySlot* wheel[4] = { &mEquippedConsumable, &mEquippedBlock, &mEquippedSecondary, &mEquippedPrimary };
    for (int i = 0; i < 4; ++i) {
        if (wheel[i]->id == id && wheel[i]->count < mItemDatabase[id].maxStack) {
            int space = mItemDatabase[id].maxStack - wheel[i]->count;
            if (amount <= space) {
                wheel[i]->count += amount;
                calculateTotalWeight();
                return true; // Everything was saved
            } else {
                wheel[i]->count += space;
                amount -= space; // We still have leftovers, we keep looking
            }
        }
    }

    // 2. SECOND: Try to stack in the BACKPACK (in slots where there is already that item)
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

    // 3. THIRD: If leftover, find an empty slot in the BACKPACK
    for (auto& slot : mBackpack) {
        if (slot.id == 0) {
            slot.id = id;

            // If the quantity we have left fits in a single slot
            if (amount <= mItemDatabase[id].maxStack) {
                slot.count = amount;
                calculateTotalWeight();
                return true;
            } else {
                // If we picked up a giant pile (e.g. 150 stone), we fill this slot and keep looking
                slot.count = mItemDatabase[id].maxStack;
                amount -= mItemDatabase[id].maxStack;
            }
        }
    }

    // If we reach here, the inventory is 100% full
    return false;
}

int Game::getItemCount(int id) {
    int total = 0;
    for (const auto& slot : mBackpack) {
        if (slot.id == id) total += slot.count;
    }
    return total;
}

bool Game::consumeItem(int id, int amount) {
    if (getItemCount(id) < amount) return false; // Not enough

    int remaining = amount;
    for (auto& slot : mBackpack) {
        if (slot.id == id) {
            if (slot.count >= remaining) {
                slot.count -= remaining;
                if (slot.count == 0) slot.id = 0; // Empty slot
                calculateTotalWeight();
                return true;
            } else {
                remaining -= slot.count;
                slot.count = 0;
                slot.id = 0;
            }
        }
    }
    return false;
}

// Checks if we have all ingredients for a recipe
bool Game::canCraft(const Recipe& recipe) {
    for (const auto& ing : recipe.ingredients) {
        if (getItemCount(ing.first) < ing.second) {
            return false; // Missing this ingredient
        }
    }
    return true; // We have everything!
}

// Charges materials and gives the item
void Game::craftItem(const Recipe& recipe) {
    if (!canCraft(recipe)) return; // For safety, check again

    // 1. Charge ingredients
    for (const auto& ing : recipe.ingredients) {
        consumeItem(ing.first, ing.second);
    }

    // 2. Deliver crafted product
    addItemToBackpack(recipe.resultId, recipe.resultCount);

    // Crafting sound (use build sound for example)
    mSndBuild.setPitch(1.5f);
    mSndBuild.play();
}

void Game::respawnPlayer() {
    // 1. Remove the death state
    mIsPlayerDead = false;

    // 2. Restore health to maximum
    // (Make sure to have a setter for health in Player.h, e.g.: void setHp(int hp) { mHp = hp; })
    mPlayer.setHp(100);

    // 3. Teleport to Spawn (Find a safe place on the surface)
    // We are going to make it so you always respawn in column X = 100 of your world
    int gridX = 100;
    float spawnX = gridX * mWorld.getTileSize();
    float spawnY = 0.0f;

    // We scan from the sky (Y=0) downwards looking for the first solid block
    for (int y = 0; y < WORLD_HEIGHT; ++y) {
        if (mWorld.getBlock(gridX, y) != 0) {
            // We found the ground! We put the player 2 blocks above so they don't get stuck
            spawnY = (y - 2) * mWorld.getTileSize();
            break;
        }
    }

    // We move the player to those safe coordinates
    mPlayer.setPosition(sf::Vector2f(spawnX, spawnY));

    // 4. Reset velocity
    // Super important! If you died falling down a ravine, the player still has falling velocity.
    // If we don't reset it, when you respawn you will crash into the ground and die again!
    mPlayer.setVelocity(sf::Vector2f(0.0f, 0.0f));
}

bool Game::interactWithBlock(int gridX, int gridY) {
    int blockID = mWorld.getBlock(gridX, gridY);
    if (blockID == ItemID::AIR) return false;

    // 1. CHECK DISTANCE
    sf::Vector2f playerCenter = mPlayer.getCenter();
    float pGridX = playerCenter.x / mWorld.getTileSize();
    float pGridY = playerCenter.y / mWorld.getTileSize();

    float dx = pGridX - gridX;
    float dy = pGridY - gridY;
    float distance = std::sqrt(dx*dx + dy*dy);

    if (distance > 4.0f) { // Maximum reach: 4 blocks
        return false;
    }

    //  CRAFTING TABLE
    if (blockID == ItemID::CRAFTING_TABLE) {
        mIsCraftingTableOpen = !mIsCraftingTableOpen;
        mIsFurnaceOpen = false;
        mIsInventoryOpen = mIsCraftingTableOpen;
        std::cout << "You have opened the Crafting Table." << std::endl;
        return true;
    }

    // FURNACE
    if (blockID == ItemID::FURNACE) {
        mIsFurnaceOpen = !mIsFurnaceOpen;
        mIsInventoryOpen = mIsFurnaceOpen;
        mIsCraftingTableOpen = false;

        if (mIsFurnaceOpen) {
            mOpenFurnacePos = {gridX, gridY};
        }
        std::cout << "You have opened the Furnace at " << gridX << ", " << gridY << std::endl;
        return true;
    }

    //  CLOSED DOOR
    if (blockID >= ItemID::DOOR && blockID <= ItemID::DOOR_TOP) {
        int baseY = gridY;
        if (blockID == ItemID::DOOR_MID) baseY = gridY + 1;
        if (blockID == ItemID::DOOR_TOP) baseY = gridY + 2;

        mWorld.setBlock(gridX, baseY,     ItemID::DOOR_OPEN);
        mWorld.setBlock(gridX, baseY - 1, ItemID::DOOR_OPEN_MID);
        mWorld.setBlock(gridX, baseY - 2, ItemID::DOOR_OPEN_TOP);
        std::cout << "Door Opened!" << std::endl;
        return true;
    }

    //  OPEN DOOR
    if (blockID >= ItemID::DOOR_OPEN && blockID <= ItemID::DOOR_OPEN_TOP) {
        int baseY = gridY;
        if (blockID == ItemID::DOOR_OPEN_MID) baseY = gridY + 1;
        if (blockID == ItemID::DOOR_OPEN_TOP) baseY = gridY + 2;

        mWorld.setBlock(gridX, baseY,     ItemID::DOOR);
        mWorld.setBlock(gridX, baseY - 1, ItemID::DOOR_MID);
        mWorld.setBlock(gridX, baseY - 2, ItemID::DOOR_TOP);
        std::cout << "Door Closed!" << std::endl;
        return true;
    }

    // CHEST
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
        std::cout << "You have opened a Chest at " << gridX << ", " << gridY << std::endl;
        return true;
    }

    return false;
}

void Game::updateDayNightCycle(float dtAsSeconds) {
    // 1. Advance the clock
    mGameTime += dtAsSeconds;
    if (mGameTime > DAY_LENGTH) {
        mGameTime -= DAY_LENGTH; // The day starts again
    }

    // We calculate in what percentage of the day we are (0.0 = start, 1.0 = end)
    float timeFraction = mGameTime / DAY_LENGTH;

    // 2. Define the colors of each phase
    sf::Color nightColor(30, 30, 50);     // Dark blue (Night)
    sf::Color dawnColor(200, 120, 100);   // Orange/Pink (Sunrise)
    sf::Color dayColor(255, 255, 255);    // Pure white (Noon)
    sf::Color duskColor(120, 60, 80);     // Dark Red/Purple (Sunset)

    sf::Color startColor;
    sf::Color targetColor;
    float lerpFactor = 0.0f;

    // 3. Decide in which phase we are and calculate the transition
    if (timeFraction < 0.25f) {
        // 0% to 25%: Sunrise -> Day
        startColor = dawnColor;
        targetColor = dayColor;
        lerpFactor = timeFraction / 0.25f;
    }
    else if (timeFraction < 0.50f) {
        // 25% to 50%: Full Day
        startColor = dayColor;
        targetColor = dayColor; // Stays white
        lerpFactor = 1.0f;
    }
    else if (timeFraction < 0.75f) {
        // 50% to 75%: Day -> Sunset -> Night
        startColor = dayColor;
        targetColor = nightColor;
        lerpFactor = (timeFraction - 0.50f) / 0.25f;
    }
    else {
        // 75% to 100%: Night -> Sunrise
        startColor = nightColor;
        targetColor = dawnColor;
        lerpFactor = (timeFraction - 0.75f) / 0.25f;
    }

    // 4. Apply mathematical interpolation to mix the colors
    mAmbientLight.r = startColor.r + (targetColor.r - startColor.r) * lerpFactor;
    mAmbientLight.g = startColor.g + (targetColor.g - startColor.g) * lerpFactor;
    mAmbientLight.b = startColor.b + (targetColor.b - startColor.b) * lerpFactor;

    // 5. Darken the sky too!
    // I assume you have mSkySprite for the background, if it's called differently change it
    mSkySprite.setColor(mAmbientLight);
}

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

    // 1. Mouse Selector
    sf::RectangleShape selector(sf::Vector2f(tileSize, tileSize));
    selector.setPosition(gridX * tileSize, gridY * tileSize);
    selector.setFillColor(sf::Color::Transparent);
    selector.setOutlineThickness(2.0f);
    selector.setOutlineColor(dist <= maxRange ? sf::Color(255, 255, 255, 150) : sf::Color(255, 0, 0, 150));
    mWindow.draw(selector);

    // 2. Mining Progress Bar
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

    // 3. Active Item (Fixed HUD)
    sf::View currentView = mWindow.getView();
    mWindow.setView(mWindow.getDefaultView());

    float uiX = 240.0f;
    float uiY = 70.0f;
    float slotSize = 40.0f;

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

    // 4. Health Hearts
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

void Game::renderMenus() {
    // 1. General Dark Background
    if (mIsInventoryOpen || mIsFurnaceOpen) {
        sf::View uiView(sf::FloatRect(0.0f, 0.0f, mWindow.getSize().x, mWindow.getSize().y));
        mWindow.setView(uiView);
        sf::RectangleShape overlay(sf::Vector2f(mWindow.getSize().x, mWindow.getSize().y));
        overlay.setFillColor(sf::Color(0, 0, 0, 150));
        mWindow.draw(overlay);
    }

    // 2. Furnace Interface
    if (mIsFurnaceOpen) {
        mWindow.setView(mWindow.getDefaultView());
        float scale = 6.0f;
        mFurnaceBgSprite.setScale(scale, scale);

        float bgX = (mWindow.getSize().x - mFurnaceBgTex.getSize().x * scale) / 2.0f;
        float bgY = (mWindow.getSize().y - mFurnaceBgTex.getSize().y * scale) / 2.0f;
        mFurnaceBgSprite.setPosition(bgX, bgY);
        mWindow.draw(mFurnaceBgSprite);

        float firePercent = mActiveFurnaces[mOpenFurnacePos].maxFuelTimer > 0.0f ?
            std::clamp(mActiveFurnaces[mOpenFurnacePos].fuelTimer / mActiveFurnaces[mOpenFurnacePos].maxFuelTimer, 0.0f, 1.0f) : 0.0f;
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
                countText.setFillColor(sf::Color::White);
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

    // 3. Inventory Menus (Backpack, Wheel, Chests, Crafting)
    if (mIsInventoryOpen) {
        sf::View uiView(sf::FloatRect(0.0f, 0.0f, mWindow.getSize().x, mWindow.getSize().y));
        mWindow.setView(uiView);

        float slotSize = 48.0f;
        float padding = 8.0f;

        // A) Backpack
        float startX = (mWindow.getSize().x / 2.0f) - ((10 * slotSize + 9 * padding) / 2.0f);
        float startY = mWindow.getSize().y - (3 * slotSize + 2 * padding) - 50.0f;

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

        // B) Tactical Wheel and Crafting
        if (!mIsFurnaceOpen && !mIsChestOpen) {
            float wheelCX = mWindow.getSize().x / 2.0f;
            float wheelCY = mWindow.getSize().y / 2.0f - 100.0f;
            mWheelSprite.setPosition(wheelCX, wheelCY);
            mWindow.draw(mWheelSprite);

            InventorySlot* wheelSlots[4] = { &mEquippedConsumable, &mEquippedBlock, &mEquippedSecondary, &mEquippedPrimary };
            float offset = 100.0f;
            sf::Vector2f wheelPositions[4] = { sf::Vector2f(0, -offset), sf::Vector2f(0, offset), sf::Vector2f(-offset, 0), sf::Vector2f(offset, 0) };
            std::string wheelLabels[4] = {"Usable", "Block", "Weapon 2", "Weapon 1"};
            sf::Vector2f textOffsets[4] = { sf::Vector2f(0.0f, -45.0f), sf::Vector2f(0.0f, 25.0f), sf::Vector2f(-45.0f, -10.0f), sf::Vector2f(45.0f, -10.0f) };

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
                        float scale = 40.0f / tex->getSize().x; // Icon size
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

            // Crafting Menu
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

        // C) Chest
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

        // D) Drag & Drop
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

    mDeathOverlay.setSize(sf::Vector2f(screenW, screenH));

    sf::FloatRect titleBounds = mDeathTitleText.getLocalBounds();
    mDeathTitleText.setOrigin(titleBounds.left + titleBounds.width / 2.0f, titleBounds.top + titleBounds.height / 2.0f);
    mDeathTitleText.setPosition(screenW / 2.0f, screenH / 2.0f - 50.0f);

    sf::FloatRect subBounds = mDeathSubText.getLocalBounds();
    mDeathSubText.setOrigin(subBounds.left + subBounds.width / 2.0f, subBounds.top + subBounds.height / 2.0f);
    mDeathSubText.setPosition(screenW / 2.0f, screenH / 2.0f + 50.0f);

    mWindow.draw(mDeathOverlay);
    mWindow.draw(mDeathTitleText);
    mWindow.draw(mDeathSubText);
}

void Game::handleMouseClick(float mx, float my) {
    float slotSize = 48.0f;
    float padding = 8.0f;
    float startX = (mWindow.getSize().x / 2.0f) - ((10 * slotSize + 9 * padding) / 2.0f);
    float startY = mWindow.getSize().y - (3 * slotSize + 2 * padding) - 50.0f;

    // A. Click in Backpack?
    for (int row = 0; row < 3; ++row) {
        for (int col = 0; col < 10; ++col) {
            float sx = startX + col * (slotSize + padding);
            float sy = startY + row * (slotSize + padding);
            if (sf::FloatRect(sx, sy, slotSize, slotSize).contains(mx, my)) {
                int index = row * 10 + col;
                if (mBackpack[index].id != ItemID::AIR) {
                    mDraggedItem = mBackpack[index];
                    mDragSourceType = 1;
                    mBackpack[index].id = ItemID::AIR;
                    mBackpack[index].count = 0;
                    return; // If we already picked something up, we leave
                }
            }
        }
    }

    // B. Click in Tactical Wheel?
    if (mDraggedItem.id == ItemID::AIR && !mIsFurnaceOpen && !mIsChestOpen) {
        float wheelCX = mWindow.getSize().x / 2.0f;
        float wheelCY = mWindow.getSize().y / 2.0f - 100.0f;
        float offset = 100.0f;
        sf::Vector2f wheelPositions[4] = { sf::Vector2f(0, -offset), sf::Vector2f(0, offset), sf::Vector2f(-offset, 0), sf::Vector2f(offset, 0) };
        InventorySlot* wheelSlots[4] = { &mEquippedConsumable, &mEquippedBlock, &mEquippedSecondary, &mEquippedPrimary };

        for (int i = 0; i < 4; ++i) {
            float sx = wheelCX + wheelPositions[i].x - 30.0f;
            float sy = wheelCY + wheelPositions[i].y - 30.0f;
            if (sf::FloatRect(sx, sy, 60.0f, 60.0f).contains(mx, my) && wheelSlots[i]->id != ItemID::AIR) {
                mDraggedItem = *wheelSlots[i];
                mDragSourceType = 2;
                wheelSlots[i]->id = ItemID::AIR;
                wheelSlots[i]->count = 0;
                return;
            }
        }
    }

    // C. Click in Crafting Menu?
    if (mDraggedItem.id == ItemID::AIR && !mIsFurnaceOpen && !mIsChestOpen) {
        float craftX = 50.0f, craftY = 100.0f, rowHeight = 60.0f, panelWidth = 320.0f;
        int displayIndex = 0;
        for (size_t i = 0; i < mRecipes.size(); ++i) {
            if (mRecipes[i].requiresTable && !mIsCraftingTableOpen) continue;

            float sx = craftX;
            float sy = craftY + displayIndex * rowHeight;
            if (sf::FloatRect(sx, sy, panelWidth, rowHeight - 5.0f).contains(mx, my)) {
                if (canCraft(mRecipes[i])) craftItem(mRecipes[i]);
                return;
            }
            displayIndex++;
        }
    }

    // D. Click in Furnace?
    if (mIsFurnaceOpen && mDraggedItem.id == ItemID::AIR) {
        float scale = 6.0f;
        float bgX = (mWindow.getSize().x - mFurnaceBgTex.getSize().x * scale) / 2.0f;
        float bgY = (mWindow.getSize().y - mFurnaceBgTex.getSize().y * scale) / 2.0f;

        auto checkFurnaceSlot = [&](InventorySlot& slot, float texX, float texY, float texW, float texH) {
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

        if (checkFurnaceSlot(mActiveFurnaces[mOpenFurnacePos].input, 34.0f, 13.0f, 17.0f, 11.0f)) return;
        if (checkFurnaceSlot(mActiveFurnaces[mOpenFurnacePos].fuel, 34.0f, 37.0f, 17.0f, 11.0f)) return;
        if (checkFurnaceSlot(mActiveFurnaces[mOpenFurnacePos].output, 81.0f, 21.0f, 23.0f, 17.0f)) return;
    }

    // E. Click in Chest?
    if (mIsChestOpen && mDraggedItem.id == ItemID::AIR) {
        float slotSizeChest = 60.0f, paddingChest = 10.0f;
        int cols = 6, rows = 4;
        float chestStartX = (mWindow.getSize().x - ((cols * (slotSizeChest + paddingChest)) + paddingChest)) / 2.0f;
        float chestStartY = (mWindow.getSize().y - ((rows * (slotSizeChest + paddingChest)) + paddingChest)) / 2.0f - 100.0f;

        ChestData& currentChest = mActiveChests[mOpenChestPos];
        for (int i = 0; i < 24; ++i) {
            float sx = chestStartX + (i % cols) * (slotSizeChest + paddingChest);
            float sy = chestStartY + (i / cols) * (slotSizeChest + paddingChest);

            if (sf::FloatRect(sx, sy, slotSizeChest, slotSizeChest).contains(mx, my) && currentChest.slots[i].id != ItemID::AIR) {
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
                return; // Dropped successfully!
            }
        }
    }

    // B. Drop in Tactical Wheel
    if (!mIsFurnaceOpen && !mIsChestOpen) {
        float wheelCX = mWindow.getSize().x / 2.0f;
        float wheelCY = mWindow.getSize().y / 2.0f - 100.0f;
        sf::Vector2f wheelPositions[4] = { sf::Vector2f(0, -100.0f), sf::Vector2f(0, 100.0f), sf::Vector2f(-100.0f, 0), sf::Vector2f(100.0f, 0) };
        InventorySlot* wheelSlots[4] = { &mEquippedConsumable, &mEquippedBlock, &mEquippedSecondary, &mEquippedPrimary };

        for (int i = 0; i < 4; ++i) {
            float sx = wheelCX + wheelPositions[i].x - 30.0f;
            float sy = wheelCY + wheelPositions[i].y - 30.0f;
            if (sf::FloatRect(sx, sy, 60.0f, 60.0f).contains(mx, my)) {
                int dragID = mDraggedItem.id;
                bool allowed = false;

                if (i == 0 && (dragID == ItemID::MEAT || dragID == ItemID::TORCH)) allowed = true; // Up (Consumables)

                // --- CORRECCIÓN: Ahora aceptamos bloques hasta la Nieve (ID 14) ---
                else if (i == 1 && (((dragID >= ItemID::DIRT && dragID <= ItemID::SNOW) && dragID != ItemID::TORCH) || dragID == ItemID::DOOR || dragID == ItemID::CRAFTING_TABLE || dragID == ItemID::FURNACE || dragID == ItemID::CHEST)) allowed = true; // Down (Blocks)

                else if ((i == 2 || i == 3) && ((dragID >= ItemID::WOOD_PICKAXE && dragID <= ItemID::TUNGSTEN_PICKAXE) || (dragID >= ItemID::WOOD_SWORD && dragID <= ItemID::BOW))) {
                    // --- NUEVO: Aceptamos desde la espada de madera hasta el ARCO ---
                    allowed = true; // Sides (Pickaxes, Swords and Bows allowed)
                }

                if (allowed) {
                    InventorySlot temp = *wheelSlots[i];
                    *wheelSlots[i] = mDraggedItem;
                    mDraggedItem = temp;
                    return;
                } else {
                    std::cout << "You cannot equip that item in this slot!" << std::endl;
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
                    std::cout << "You cannot put items in the output tray!" << std::endl;
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
        float slotSizeChest = 60.0f, paddingChest = 10.0f;
        int cols = 6, rows = 4;
        float chestStartX = (mWindow.getSize().x - ((cols * (slotSizeChest + paddingChest)) + paddingChest)) / 2.0f;
        float chestStartY = (mWindow.getSize().y - ((rows * (slotSizeChest + paddingChest)) + paddingChest)) / 2.0f - 100.0f;

        ChestData& currentChest = mActiveChests[mOpenChestPos];
        for (int i = 0; i < 24; ++i) {
            float sx = chestStartX + (i % cols) * (slotSizeChest + paddingChest);
            float sy = chestStartY + (i / cols) * (slotSizeChest + paddingChest);

            if (sf::FloatRect(sx, sy, slotSizeChest, slotSizeChest).contains(mx, my)) {
                InventorySlot& clickedSlot = currentChest.slots[i];
                if (clickedSlot.id == mDraggedItem.id) { // Stack
                    int spaceLeft = mItemDatabase[clickedSlot.id].maxStack - clickedSlot.count;
                    if (mDraggedItem.count <= spaceLeft) {
                        clickedSlot.count += mDraggedItem.count;
                        mDraggedItem.id = ItemID::AIR;
                        mDraggedItem.count = 0;
                    } else {
                        clickedSlot.count += spaceLeft;
                        mDraggedItem.count -= spaceLeft;
                    }
                } else { // Swap
                    InventorySlot temp = clickedSlot;
                    clickedSlot = mDraggedItem;
                    mDraggedItem = temp;
                }
                calculateTotalWeight();
                return;
            }
        }
    }

    // E. Drop in air (outside UI) -> Returns to backpack
    if (mDraggedItem.id != ItemID::AIR) {
        addItemToBackpack(mDraggedItem.id, mDraggedItem.count);
        mDraggedItem.id = ItemID::AIR;
        mDraggedItem.count = 0;
    }
}

void Game::spawnParticles(sf::Vector2f pos, int itemID, int count) {
    // 1. We choose the color according to the material
    sf::Color pColor = sf::Color::White;
    switch(itemID) {
        case ItemID::DIRT: pColor = sf::Color(139, 69, 19); break;     // Dirt brown
        case ItemID::GRASS: pColor = sf::Color(34, 139, 34); break;    // Grass green
        case ItemID::STONE: case ItemID::COAL:
        case ItemID::FURNACE: pColor = sf::Color(128, 128, 128); break;// Stone gray
        case ItemID::WOOD: case ItemID::CRAFTING_TABLE:
        case ItemID::CHEST: case ItemID::DOOR:
            pColor = sf::Color(101, 67, 33); break;                    // Dark wood brown
        case ItemID::LEAVES: pColor = sf::Color(50, 205, 50); break;   // Lime green
        case ItemID::MEAT: pColor = sf::Color(200, 0, 0); break;       // Red (Blood for enemies)
        default: pColor = sf::Color(200, 200, 200); break;             // Default gray
    }

    // 2. We generate the shrapnel
    for(int i = 0; i < count; ++i) {
        Particle p;
        p.position = pos;

        // Random velocity: X (left/right), Y (always jump up)
        float vx = (rand() % 300) - 150.0f; // -150 to 150
        float vy = -((rand() % 200) + 150.0f); // -150 to -350

        p.velocity = sf::Vector2f(vx, vy);
        p.color = pColor;

        // They live between 0.3 and 0.7 seconds
        p.maxLifetime = 0.3f + (rand() % 40) / 100.0f;
        p.lifetime = p.maxLifetime;

        // Size between 4 and 7 pixels
        p.size = 4.0f + (rand() % 4);

        mParticles.push_back(p);
    }
}