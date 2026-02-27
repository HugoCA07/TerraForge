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
    , mSoundTimer(0.0f) // Initialize the timer
    , mSpawnTimer(5.0f) // Start with 5 seconds so it spawns quickly the first time
{
    mWindow.setFramerateLimit(120);

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
    // Base text configuration
    mUiText.setFont(mFont);
    mUiText.setCharacterSize(20);
    mUiText.setFillColor(sf::Color::White);
    mUiText.setOutlineColor(sf::Color::Black);
    mUiText.setOutlineThickness(1.0f);

    // --- PREPARE NEW INVENTORY ---
    mBackpack.resize(30); // Create 30 empty slots
    // --- CRAFTING RECIPES ---
    // 1. Wood Pickaxe (ID 21): Requires 10 Wood (ID 4)
    mRecipes.push_back({21, 1, {{4, 10}}});

    // 2. Stone Pickaxe (ID 22): Requires 10 Stone (ID 3)
    mRecipes.push_back({22, 1, {{3, 10}}});

    // 3. Iron Pickaxe (ID 23): Requires 5 Iron (ID 9) and 5 Wood (ID 4)
    mRecipes.push_back({23, 1, {{9, 5}, {4, 5}}});

    // 4. Tungsten Pickaxe (ID 24): Requires 5 Tungsten (ID 11) and 5 Wood (ID 4)
    mRecipes.push_back({24, 1, {{11, 5}, {4, 5}}});

    // NUEVAS RECETAS:
    mRecipes.push_back({6, 5, {{4, 2},{5,2}}}); // Antorcha: 2 de Madera y 2 hojas = 4 Antorchas
    mRecipes.push_back({25, 1, {{4, 6}}}); // Puerta: 6 de Madera = 1 Puerta

    // Database: ID -> {Name, Weight, Max Stack}
    mItemDatabase[1] = {"Dirt", 1.0f, 99};
    mItemDatabase[2] = {"Grass", 1.0f, 99};
    mItemDatabase[3] = {"Stone", 2.0f, 99};
    mItemDatabase[4] = {"Log", 1.5f, 99};
    mItemDatabase[5] = {"Leaves", 0.1f, 99};
    mItemDatabase[6] = {"Torch", 0.2f, 99};
    mItemDatabase[25] = {"Wooden Door", 5.0f, 99}; // <--- AÑADIMOS LA PUERTA (ID 25)

    // --- NEW: MINERALS ---
    mItemDatabase[7] = {"Coal", 1.0f, 99};     // Coal weighs less
    mItemDatabase[8] = {"Copper", 2.0f, 99};      // (If you have copper)
    mItemDatabase[9] = {"Iron", 2.0f, 99};
    mItemDatabase[10] = {"Cobalt", 2.0f, 99};   // Cobalt weighs the same
    mItemDatabase[11] = {"Tungsten", 2.0f, 99};

    // Tools and Consumables
    mItemDatabase[21] = {"Wood Pickaxe", 5.0f, 1};
    mItemDatabase[22] = {"Stone Pickaxe", 5.0f, 1};
    mItemDatabase[23] = {"Iron Pickaxe", 5.0f, 1};
    mItemDatabase[24] = {"Tungsten Pickaxe", 5.0f, 1};

    mItemDatabase[30] = {"Meat", 0.5f, 20}; // Meat stacks up to 20

    // Give initial items for testing
    addItemToBackpack(1, 15); // 15 dirt
    addItemToBackpack(30, 5); // 5 meat
    addItemToBackpack(6,5);

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
        if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape)
            mWindow.close();

        // --- OPEN/CLOSE INVENTORY WITH 'E' ---
        if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::E) {
            mIsInventoryOpen = !mIsInventoryOpen;

            // SAFETY: If we close the menu while dragging something, return it to the backpack
            if (!mIsInventoryOpen && mDraggedItem.id != 0) {
                addItemToBackpack(mDraggedItem.id, mDraggedItem.count);
                mDraggedItem.id = 0;
                mDraggedItem.count = 0;
            }
        }

        // --- ADD THIS FOR RESIZE ---
            if (event.type == sf::Event::Resized) {
                // Update the view so graphics don't distort
                sf::FloatRect visibleArea(0.0f, 0.0f, event.size.width, event.size.height);
                mWindow.setView(sf::View(visibleArea));
            }

        // ==========================================================
        // DRAG & DROP SYSTEM
        // ==========================================================
        if (mIsInventoryOpen) {

            // Variables to calculate where the slots are (Same as in render)
            float slotSize = 48.0f;
            float padding = 8.0f;
            float startX = (mWindow.getSize().x / 2.0f) - ((10 * slotSize + 9 * padding) / 2.0f);
            float startY = mWindow.getSize().y - (3 * slotSize + 2 * padding) - 50.0f;

            float wheelCX = mWindow.getSize().x / 2.0f;
            float wheelCY = mWindow.getSize().y / 2.0f - 100.0f;
            float offset = 100.0f;
            sf::Vector2f wheelPositions[4] = {
                sf::Vector2f(0, -offset), sf::Vector2f(0, offset),
                sf::Vector2f(-offset, 0), sf::Vector2f(offset, 0)
            };
            InventorySlot* wheelSlots[4] = { &mEquippedConsumable, &mEquippedBlock, &mEquippedSecondary, &mEquippedPrimary };

            // ------------------------------------------------------
            // 1. ON MOUSE PRESS: PICK UP ITEM
            // ------------------------------------------------------
            if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {

                // --- MAGIC COORDINATE TRANSLATION (2K/4K Mode) ---
                sf::Vector2i pixelPos(event.mouseButton.x, event.mouseButton.y);

                // Create a virtual camera that measures EXACTLY what the physical screen measures right now
                sf::View uiView(sf::FloatRect(0.0f, 0.0f, mWindow.getSize().x, mWindow.getSize().y));
                sf::Vector2f worldPos = mWindow.mapPixelToCoords(pixelPos, uiView);

                float mx = worldPos.x;
                float my = worldPos.y;
                // ----------------------------------------------------
                // ----------------------------------------

                // A. Did we click on the Backpack?
                for (int row = 0; row < 3; ++row) {
                    for (int col = 0; col < 10; ++col) {
                        float sx = startX + col * (slotSize + padding);
                        float sy = startY + row * (slotSize + padding);

                        // Slot hitbox
                        if (mx >= sx && mx <= sx + slotSize && my >= sy && my <= sy + slotSize) {
                            int index = row * 10 + col;
                            if (mBackpack[index].id != 0) { // If there is something
                                mDraggedItem = mBackpack[index]; // Stick it to the mouse
                                mDragSourceType = 1; // Source: Backpack
                                mBackpack[index].id = 0; // Empty the slot
                                mBackpack[index].count = 0;
                            }
                        }
                    }
                }

                // B. Did we click on the Tactical Wheel?
                if (mDraggedItem.id == 0) {
                    for (int i = 0; i < 4; ++i) {
                        float sx = wheelCX + wheelPositions[i].x - 30.0f;
                        float sy = wheelCY + wheelPositions[i].y - 30.0f;

                        // 60x60 hitbox for wheel icons
                        if (mx >= sx && mx <= sx + 60.0f && my >= sy && my <= sy + 60.0f) {
                            if (wheelSlots[i]->id != 0) {
                                mDraggedItem = *wheelSlots[i];
                                mDragSourceType = 2; // Source: Wheel
                                wheelSlots[i]->id = 0;
                                wheelSlots[i]->count = 0;
                            }
                        }
                    }
                }

                // C. Did we click on the Crafting Menu?
                if (mDraggedItem.id == 0) { // Only allow crafting if mouse is empty
                    float craftX = 50.0f;
                    float craftY = 100.0f;
                    float rowHeight = 60.0f;   // Altura de cada fila
                    float panelWidth = 320.0f; // <--- ¡Panel mucho más ancho!

                    for (size_t i = 0; i < mRecipes.size(); ++i) {
                        float sx = craftX;
                        float sy = craftY + i * rowHeight;

                        // Si hacemos clic en cualquier parte de la fila ancha...
                        if (mx >= sx && mx <= sx + panelWidth && my >= sy && my <= sy + rowHeight - 5.0f) {
                            if (canCraft(mRecipes[i])) {
                                craftItem(mRecipes[i]);
                            }
                        }
                    }
                }
            }

            // ------------------------------------------------------
            // 2. ON MOUSE RELEASE: DROP ITEM
            // ------------------------------------------------------
            if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left) {
                if (mDraggedItem.id != 0) { // Only if we are holding something

                    // --- MAGIC COORDINATE TRANSLATION (2K/4K Mode) ---
                    sf::Vector2i pixelPos(event.mouseButton.x, event.mouseButton.y);

                    // Create a virtual camera that measures EXACTLY what the physical screen measures right now
                    sf::View uiView(sf::FloatRect(0.0f, 0.0f, mWindow.getSize().x, mWindow.getSize().y));
                    sf::Vector2f worldPos = mWindow.mapPixelToCoords(pixelPos, uiView);

                    float mx = worldPos.x;
                    float my = worldPos.y;
                    // ----------------------------------------------------
                    // ----------------------------------------

                    bool dropped = false;

                    // A. Did we drop it in the Backpack?
                    for (int row = 0; row < 3 && !dropped; ++row) {
                        for (int col = 0; col < 10 && !dropped; ++col) {
                            float sx = startX + col * (slotSize + padding);
                            float sy = startY + row * (slotSize + padding);
                            if (mx >= sx && mx <= sx + slotSize && my >= sy && my <= sy + slotSize) {
                                int index = row * 10 + col;

                                // MAGIC SWAP
                                InventorySlot temp = mBackpack[index];
                                mBackpack[index] = mDraggedItem;
                                mDraggedItem = temp;
                                dropped = true;
                            }
                        }
                    }

                    // B. Did we drop it in the Tactical Wheel?
                    if (!dropped) {
                        for (int i = 0; i < 4 && !dropped; ++i) {
                            float sx = wheelCX + wheelPositions[i].x - 30.0f;
                            float sy = wheelCY + wheelPositions[i].y - 30.0f;
                            if (mx >= sx && mx <= sx + 60.0f && my >= sy && my <= sy + 60.0f) {

                                // --- TACTICAL WHEEL RULES ---
                                int dragID = mDraggedItem.id;
                                bool allowed = false;

                                if (i == 0) { // UP (Usable): Meat or Torches
                                    if (dragID == 30 || dragID == 6) allowed = true;
                                }
                                // POR ESTO:
                                else if (i == 1) { // DOWN (Blocks): Everything except pickaxes, meat and torches
                                    if ((dragID >= 1 && dragID <= 5) || (dragID >= 7 && dragID <= 11) || dragID == 25) allowed = true;
                                }
                                else if (i == 2 || i == 3) { // SIDES (Weapons): Only Pickaxes
                                    if (dragID >= 21 && dragID <= 24) allowed = true;
                                }

                                if (allowed) {
                                    InventorySlot temp = *wheelSlots[i];
                                    *wheelSlots[i] = mDraggedItem;
                                    mDraggedItem = temp;
                                    dropped = true;
                                } else {
                                    std::cout << "You cannot equip that item in this slot!" << std::endl;
                                    // Will fall to case 'C'
                                }
                            }
                        }
                    }

                    // C. If we drop it in the air...
                    if (mDraggedItem.id != 0) {
                        addItemToBackpack(mDraggedItem.id, mDraggedItem.count);
                        mDraggedItem.id = 0;
                        mDraggedItem.count = 0;
                    }
                }
            }
        } // End if (mIsInventoryOpen)
    } // End while(pollEvent)
    mPlayer.handleInput(mIsInventoryOpen);
}

// Helper function to determine hardness
float getBlockHardness(int id) {
    switch (id) {
        case 0: return 0.0f;  // Air
        case 1: return 0.2f;  // Dirt (Very fast)
        case 2: return 0.25f; // Grass
        case 4: return 0.6f;  // Wood
        case 5: return 0.1f;  // Leaves (Instant)
        case 6: return 0.1f;  // Torch
        case 25: return 0.5f; // <--- ¡AÑADE LA PUERTA AQUÍ PARA PODER ROMPERLA!

            // STONE AND MINERALS
        case 3: return 1.0f;  // Stone (Standard)
        case 7: return 1.2f;  // Coal
        case 8: return 1.5f;  // Copper
        case 9: return 2.0f;  // Iron
        case 10: return 3.0f; // Cobalt (Hard)
        case 11: return 5.0f; // Tungsten (Very hard)

        case 12: return -1.0f; // Bedrock (Indestructible)

        default: return 1.0f;
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
    mPlayer.update(dt, mWorld);
    mPlayer.setOverweight(mCurrentWeight > mMaxWeight);
    // Le pasamos al jugador el ID del objeto que tiene equipado en la mano derecha
    mPlayer.setEquippedWeapon(mEquippedPrimary.id);

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
    // 1. SISTEMA DE COMBATE (Depende de la Animación)
    // ==================================================
    // Se ejecuta automáticamente en el fotograma 'Active' del ataque,
    // ¡independientemente de si el ratón sigue pulsado o no!
    if (mPlayer.canDealDamage()) {

        // 1. Calcular Daño del Arma (Según tu inventario)
        int toolDamage = 1; // Puños
        int equippedID = mEquippedPrimary.id; // Suponiendo que el arma está en la primaria
        if (equippedID == 21) toolDamage = 3;
        if (equippedID == 22) toolDamage = 5;
        if (equippedID == 23) toolDamage = 10;
        if (equippedID == 24) toolDamage = 20;

        sf::FloatRect attackHitbox = mPlayer.getWeaponHitbox();

        for (auto& mob : mMobs) {
            // Si la hitbox del hachazo toca al monstruo
            if (attackHitbox.intersects(mob->getBounds())) {

                float dir = (mPlayer.getPosition().x < mob->getPosition().x) ? 1.0f : -1.0f;

                // Aplicamos daño. Si el bicho recibe el golpe, suena
                if (mob->takeDamage(toolDamage, dir)) {
                    mSndHit.setPitch(1.0f + (rand() % 40) / 100.0f);
                    mSndHit.play();
                }

                // Ponemos el seguro para no darle 60 veces en un segundo
                mPlayer.registerHit();
                break; // Rompemos el bucle para dar solo a 1 enemigo por hachazo
            }
        }
    }

    // ==================================================
    // 2. SISTEMA DE MINERÍA (Depende de mantener el Clic)
    // ==================================================
    if (!mIsInventoryOpen) {
        if (sf::Mouse::isButtonPressed(sf::Mouse::Left)) {

            // --- ¡NUEVO! EL SEGURO DE DISTANCIA DE VUELTA ---
            // Solo empezamos a picar si el ratón está lo suficientemente cerca
            if (distance <= maxRange) {

                // ¿Hemos cambiado de bloque?
                if (mMiningPos.x != gridX || mMiningPos.y != gridY) {
                    mMiningTimer = 0.0f;
                    mMiningPos = sf::Vector2i(gridX, gridY);

                    int blockID = mWorld.getBlock(gridX, gridY);
                    mCurrentHardness = getBlockHardness(blockID);
                }

                // Minar
                if (mCurrentHardness > 0.0f) {
                    float miningSpeed = 1.0f;
                    int equippedID = mEquippedPrimary.id;

                    if (equippedID == 21) miningSpeed = 3.0f;
                    if (equippedID == 22) miningSpeed = 5.0f;
                    if (equippedID == 23) miningSpeed = 10.0f;
                    if (equippedID == 24) miningSpeed = 20.0f;

                    int targetID = mWorld.getBlock(gridX, gridY);
                    bool isHardBlock = (targetID == 3 || (targetID >= 7 && targetID <= 11));
                    bool holdingPickaxe = (equippedID >= 21 && equippedID <= 24);

                    if (isHardBlock && !holdingPickaxe) {
                        miningSpeed = 0.0f;
                    }

                    mMiningTimer += dt.asSeconds() * miningSpeed;

                    if (mMiningTimer >= mCurrentHardness) {
                        mWorld.setBlock(gridX, gridY, 0);

                        mSndBreak.setPitch(0.8f + (rand() % 40) / 100.0f);
                        mSndBreak.play();

                        mMiningTimer = 0.0f;
                        mCurrentHardness = 0.0f;
                    }
                    else {
                        mSoundTimer += dt.asSeconds();
                        if (mSoundTimer >= 0.25f && miningSpeed > 0.0f) {
                            mSndHit.setPitch(0.9f + (rand() % 20) / 100.0f);
                            mSndHit.play();
                            mSoundTimer = 0.0f;
                        }
                    }
                }
                else if (mCurrentHardness == -1.0f) {
                    mMiningTimer = 0.0f;
                }

            } else {
                // Si mantenemos pulsado pero el ratón está MUY LEJOS, reseteamos la minería
                mMiningTimer = 0.0f;
                mMiningPos = sf::Vector2i(-1, -1);
            }

        }
        else {
            // Botón izquierdo soltado: Resetear minería
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

            // --- 1. IS IT FOOD? (ID 30 = Meat) ---
            if (mSelectedBlock == 30) {
                if (mPlayer.getHp() < mPlayer.getMaxHp() && wheel[mActiveWheelSlot]->count > 0) {

                    wheel[mActiveWheelSlot]->count--; // Spend from the wheel
                    if (wheel[mActiveWheelSlot]->count == 0) wheel[mActiveWheelSlot]->id = 0; // If it runs out, clear the slot

                    mPlayer.heal(20);
                    mSndBuild.setPitch(0.5f);
                    mSndBuild.play();
                    mActionTimer = 0.25f;
                }
            }
            // --- 2. IF NOT FOOD, BUILD A BLOCK ---
            else if (distance <= maxRange) {

                bool isPlaceableBlock = (mSelectedBlock >= 1 && mSelectedBlock <= 11 || mSelectedBlock == 25);

                if (isPlaceableBlock && mWorld.getBlock(gridX, gridY) == 0) {
                    sf::FloatRect blockRect(gridX * tileSize, gridY * tileSize, tileSize, tileSize);

                    if (!mPlayer.getGlobalBounds().intersects(blockRect)) {
                        bool isMobInWay = false;
                        for (auto& mob : mMobs) {
                            if (mob->getBounds().intersects(blockRect)) {
                                isMobInWay = true;
                                break;
                            }
                        }

                        if (!isMobInWay && wheel[mActiveWheelSlot]->count > 0) {
                            mWorld.setBlock(gridX, gridY, mSelectedBlock);

                            wheel[mActiveWheelSlot]->count--; // Spend the block from the wheel
                            if (wheel[mActiveWheelSlot]->count == 0) wheel[mActiveWheelSlot]->id = 0;

                            mSndBuild.setPitch(1.0f + (rand() % 20) / 100.0f);
                            mSndBuild.play();
                            mActionTimer = 0.15f;

                            calculateTotalWeight();
                        }
                    }
                }
            }
        }// End of timer check
    }

    // --------------------------------------------------
    // DAY / NIGHT CYCLE LOGIC
    // --------------------------------------------------
    mGameTime += dt.asSeconds() * 0.5f;
    // ==========================================
    // DAY AND NIGHT CYCLE SYSTEM
    // ==========================================
    float dayLength = 120.0f; // A full day lasts 2 real minutes
    if (mGameTime > dayLength) {
        mGameTime = 0.0f; // Restart the cycle when reaching the end
    }

    int lightLevel = 255; // Default, full light (Day)

    // We are going to divide the 120 seconds into phases:
    if (mGameTime > 60.0f && mGameTime < 80.0f) {
        // SUNSET (From 60 to 80 sec): Gradually darken
        float progress = (mGameTime - 60.0f) / 20.0f; // Goes from 0.0 to 1.0
        lightLevel = 255 - static_cast<int>(progress * 200); // Drops from 255 to 55
    }
    else if (mGameTime >= 80.0f && mGameTime < 100.0f) {
        // DARK NIGHT (From 80 to 100 sec)
        lightLevel = 55;
    }
    else if (mGameTime >= 100.0f && mGameTime <= 120.0f) {
        // SUNRISE (From 100 to 120 sec): Gradually lighten
        float progress = (mGameTime - 100.0f) / 20.0f; // Goes from 0.0 to 1.0
        lightLevel = 55 + static_cast<int>(progress * 200); // Rises from 55 to 255
    }

    // Give the night a slightly bluish tint to make it look nicer
    int blueLevel = std::min(255, lightLevel + 40);
    mAmbientLight = sf::Color(lightLevel, lightLevel, blueLevel);

    float cycle = std::sin(mGameTime);
    float brightness = (cycle * 0.4f) + 0.6f;

    sf::Uint8 r = static_cast<sf::Uint8>(255 * brightness);
    sf::Uint8 g = static_cast<sf::Uint8>(255 * brightness);
    sf::Uint8 b = static_cast<sf::Uint8>(255 * brightness);

    if (brightness < 0.5f) {
        b = static_cast<sf::Uint8>(std::min(255, b + 40));
    }
    mAmbientLight = sf::Color(r, g, b);

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
    // AUTOMATIC CREATURE SPAWNER
    // ==========================================
    mSpawnTimer -= dt.asSeconds();

    if (mSpawnTimer <= 0.0f) {
        // Reset the clock: between 10 and 20 seconds until the next spawn
        mSpawnTimer = 10.0f + (rand() % 10);

        // Only spawn if we haven't reached the population limit
        if (mMobs.size() < MAX_MOBS) {

            // 1. DISTANCE: Much further (Between 1500 and 2500 pixels away)
            float playerX = mPlayer.getPosition().x;
            float spawnDistance = 1500.0f + (rand() % 1000);

            float spawnX;
            if (rand() % 2 == 0) {
                spawnX = playerX - spawnDistance; // Left
            } else {
                spawnX = playerX + spawnDistance; // Right
            }

            // 2. Find the ground (Y) at that X coordinate
            float tileSize = mWorld.getTileSize();
            int gridX = std::max(0, static_cast<int>(spawnX / tileSize));

            float spawnY = 0.0f;
            bool groundFound = false;

            // Raycast from the sky downwards
            for (int y = 0; y < 200; ++y) {
                if (mWorld.getBlock(gridX, y) != 0) {
                    // WE FOUND THE GROUND!
                    // But before confirming, check that it is a safe place (no walls on the sides)
                    // Check if the space above (y-1, y-2) and to the sides (x-1, x+1) is pure air (0)
                    if (y >= 3 &&
                        mWorld.getBlock(gridX, y - 1) == 0 &&       // Center free
                        mWorld.getBlock(gridX, y - 2) == 0 &&       // High center free
                        mWorld.getBlock(gridX - 1, y - 1) == 0 &&   // Left free
                        mWorld.getBlock(gridX + 1, y - 1) == 0) {   // Right free

                        spawnY = (y - 2) * tileSize; // Born a couple of blocks above, safe
                        groundFound = true;
                        }

                    // Whether it is a valid place or not, break the loop because we already touched the surface
                    // (This prevents them from continuing to search and spawning inside dark caves)
                    break;
                }
            }

            // 3. LET THERE BE LIFE!
            if (groundFound) {
                // If the ambient light has little red (less than 100), consider it night
                bool isNight = (mAmbientLight.r < 100);
                sf::Vector2f spawnPos(spawnX, spawnY);

                if (isNight) {
                    mMobs.push_back(std::make_unique<Troodon>(spawnPos, mTroodonTexture));
                    std::cout << "A Troodon lurks in the darkness..." << std::endl;
                } else {
                    mMobs.push_back(std::make_unique<Dodo>(spawnPos, mDodoTexture));
                    std::cout << "A wild Dodo has appeared." << std::endl;
                }
            }
        }
    }
    for (auto it = mMobs.begin(); it != mMobs.end(); ) {

        auto& mob = **it; // Extract the object to make it easier to read

        mob.update(dt, mPlayer.getPosition(), mWorld, brightness);

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
            mWorld.spawnItem(30, mob.getPosition()); // Drops meat
            mSndBreak.setPitch(1.5f);
            mSndBreak.play();
            it = mMobs.erase(it); // Automatically deleted from memory
        } else {
            ++it;
        }
    }

    // --------------------------------------------------
    // DEATH AND RESPAWN SYSTEM
    // --------------------------------------------------
    if (mPlayer.getHp() <= 0) {
        std::cout << "YOU DIED! Respawning..." << std::endl;

        // 1. Heal to max
        mPlayer.heal(mPlayer.getMaxHp());

        // 2. Teleport to start (High in the sky to fall safely to the ground)
        mPlayer.setPosition(sf::Vector2f(100.0f, -500.0f));

        // 3. Classic penalty: You lose half your food (ID 30)!
        int meatCount = getItemCount(30); // Count how much meat you have in the entire backpack
        if (meatCount > 0) {
            int meatToLose = meatCount / 2;
            if (meatToLose > 0) {
                consumeItem(30, meatToLose); // Delete that half from the backpack
                std::cout << "You lost " << meatToLose << " pieces of meat..." << std::endl;
            }
        }

        // Dramatic death sound
        mSndBreak.setPitch(0.4f);
        mSndBreak.play();
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
    mWindow.clear(sf::Color(135, 206, 235));

    // 1. DRAW SKY
    mSkySprite.setColor(mAmbientLight);
    mWindow.draw(mSkySprite);

    // 2. DRAW WORLD AND PLAYER
    // Here the camera is focusing on the world correctly
    mWorld.render(mWindow, mAmbientLight);
    mPlayer.render(mWindow, mAmbientLight);

    // DRAW ALL CREATURES
    for (auto& mob : mMobs) {
        mob->render(mWindow, mAmbientLight);
    }

    // ==========================================
    // PIXELATED LIGHT HALOS (DIAMONDS)
    // ==========================================
    float px = mPlayer.getPosition().x;
    float py = mPlayer.getPosition().y;
    float tileSize = mWorld.getTileSize();

    int centerGridX = static_cast<int>(px / tileSize);
    int centerGridY = static_cast<int>(py / tileSize);

    // --- DIAMOND CONFIGURATION ---
    // We use squares instead of circles for a more "pixelated" look

    // 1. Outer Halo (Large, soft orange)
    float outerSize = 300.0f;
    sf::RectangleShape lightHalo(sf::Vector2f(outerSize, outerSize));
    lightHalo.setOrigin(outerSize / 2.0f, outerSize / 2.0f); // Pivot to center
    lightHalo.setRotation(45.0f); // The trick! Rotate it to be a diamond
    lightHalo.setFillColor(sf::Color(255, 100, 20, 15)); // Warm color, very transparent

    // 2. Middle Halo (Medium, fire orange)
    float midSize = 180.0f;
    sf::RectangleShape midHalo(sf::Vector2f(midSize, midSize));
    midHalo.setOrigin(midSize / 2.0f, midSize / 2.0f);
    midHalo.setRotation(45.0f);
    midHalo.setFillColor(sf::Color(255, 160, 40, 25));

    // 3. Core Halo (Small, bright yellow, the "flame")
    float coreSize = 60.0f;
    sf::RectangleShape coreHalo(sf::Vector2f(coreSize, coreSize));
    coreHalo.setOrigin(coreSize / 2.0f, coreSize / 2.0f);
    coreHalo.setRotation(45.0f);
    coreHalo.setFillColor(sf::Color(255, 255, 150, 40));

    // --- DRAW LOOP ---
    for (int x = centerGridX - 20; x <= centerGridX + 20; ++x) {
        for (int y = centerGridY - 20; y <= centerGridY + 20; ++y) {

            if (mWorld.getBlock(x, y) == 6) {
                // Center position of the torch block
                sf::Vector2f lightPos((x * tileSize) + (tileSize / 2.0f),
                                      (y * tileSize) + (tileSize / 2.0f));

                // Draw the 3 layers with BlendAdd
                lightHalo.setPosition(lightPos);
                mWindow.draw(lightHalo, sf::BlendAdd);

                midHalo.setPosition(lightPos);
                mWindow.draw(midHalo, sf::BlendAdd);

                coreHalo.setPosition(lightPos);
                mWindow.draw(coreHalo, sf::BlendAdd);
            }
        }
    }

    // --------------------------------------------------
    // DRAW MOUSE SELECTOR
    // --------------------------------------------------
    sf::Vector2i pixelPos = sf::Mouse::getPosition(mWindow);
    sf::Vector2f worldPos = mWindow.mapPixelToCoords(pixelPos);
    tileSize = mWorld.getTileSize();

    int gridX = static_cast<int>(std::floor(worldPos.x / tileSize));
    int gridY = static_cast<int>(std::floor(worldPos.y / tileSize));

    float blockCenterX = (gridX * tileSize) + (tileSize / 2.0f);
    float blockCenterY = (gridY * tileSize) + (tileSize / 2.0f);

    sf::Vector2f playerCenter = mPlayer.getPosition();
    float dx = blockCenterX - playerCenter.x;
    float dy = blockCenterY - playerCenter.y;
    float dist = std::sqrt(dx*dx + dy*dy);
    float maxRange = 180.0f;

    sf::RectangleShape selector(sf::Vector2f(tileSize, tileSize));
    selector.setPosition(gridX * tileSize, gridY * tileSize);
    selector.setFillColor(sf::Color::Transparent);
    selector.setOutlineThickness(2.0f);

    if (dist <= maxRange) {
        selector.setOutlineColor(sf::Color(255, 255, 255, 150));
    } else {
        selector.setOutlineColor(sf::Color(255, 0, 0, 150));
    }
    mWindow.draw(selector);

    // --------------------------------------------------
    // DRAW MINING PROGRESS BAR (THIS IS THE CORRECT PLACE)
    // --------------------------------------------------
    // We put it here because the 'Game View' is still active.
    // We don't need to do 'setView(view)' because we are already in that view.

    if (mMiningTimer > 0.0f && mCurrentHardness > 0.0f) {

        // Calculate percentage (0.0 to 1.0)
        float percent = mMiningTimer / mCurrentHardness;
        if (percent > 1.0f) percent = 1.0f;

        // Bar background (Black)
        sf::RectangleShape barBg(sf::Vector2f(tileSize * 0.8f, 6.0f));
        barBg.setPosition(
            mMiningPos.x * tileSize + tileSize * 0.1f,
            mMiningPos.y * tileSize + tileSize * 0.1f // A little above the block
        );
        barBg.setFillColor(sf::Color::Black);
        mWindow.draw(barBg);

        // Bar fill (White)
        sf::RectangleShape barFill(sf::Vector2f((tileSize * 0.8f) * percent, 4.0f));
        barFill.setPosition(
            barBg.getPosition().x + 1.0f, // 1px margin
            barBg.getPosition().y + 1.0f
        );
        barFill.setFillColor(sf::Color::White);
        mWindow.draw(barFill);
    }

    // --------------------------------------------------
    // UI / HUD SYSTEM (ITEM IN HAND)
    // --------------------------------------------------
    mWindow.setView(mWindow.getDefaultView());

    // Only draw the active item slot, to the right of the health bar
    float uiX = 240.0f; // To the right of the red bar
    float uiY = 70.0f;
    float slotSize = 40.0f;

    sf::RectangleShape activeSlotBg(sf::Vector2f(slotSize, slotSize));
    activeSlotBg.setPosition(uiX, uiY);
    activeSlotBg.setFillColor(sf::Color(0, 0, 0, 150));
    activeSlotBg.setOutlineThickness(2.0f);
    activeSlotBg.setOutlineColor(sf::Color::White); // Always white because it's active
    mWindow.draw(activeSlotBg);

    if (mSelectedBlock != 0) {
        const sf::Texture* tex = mWorld.getTexture(mSelectedBlock);
        if (tex != nullptr) {
            sf::Sprite icon(*tex);
            float scale = (slotSize - 10.0f) / tex->getSize().x;
            icon.setScale(scale, scale);
            icon.setPosition(uiX + 5.0f, uiY + 5.0f);
            mWindow.draw(icon);

            // Quantity (If it's a block or consumable)
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

    // --------------------------------------------------
    // DRAW HEALTH BAR (HUD)
    // --------------------------------------------------
    // Calculate health percentage (0.0 to 1.0)
    float hpPercent = static_cast<float>(mPlayer.getHp()) / mPlayer.getMaxHp();

    // Position: Below inventory (X: 20, Y: 80)
    float barX = 20.0f;
    float barY = 80.0f;
    float barWidth = 200.0f;
    float barHeight = 20.0f;

    // 1. Background (Dark red)
    sf::RectangleShape hpBg(sf::Vector2f(barWidth, barHeight));
    hpBg.setPosition(barX, barY);
    hpBg.setFillColor(sf::Color(100, 0, 0, 200));
    hpBg.setOutlineThickness(2.0f);
    hpBg.setOutlineColor(sf::Color::Black);
    mWindow.draw(hpBg);

    // 2. Bar fill (Bright red or Green)
    sf::RectangleShape hpFill(sf::Vector2f(barWidth * hpPercent, barHeight));
    hpFill.setPosition(barX, barY);
    hpFill.setFillColor(sf::Color(255, 50, 50, 255)); // Bright red
    mWindow.draw(hpFill);

    // 3. (Optional) Health Text '50/100'
    sf::Text hpText = mUiText; // Copy the font
    hpText.setString(std::to_string(mPlayer.getHp()) + " / " + std::to_string(mPlayer.getMaxHp()));
    hpText.setCharacterSize(14);
    hpText.setPosition(barX + barWidth / 2.0f - 25.0f, barY); // Centered by eye
    mWindow.draw(hpText);

    // --------------------------------------------------
    // FULL INVENTORY MENU (KEY 'E')
    // --------------------------------------------------
    // Use the all-terrain camera so the UI anchors perfectly to any monitor
    sf::View uiView(sf::FloatRect(0.0f, 0.0f, mWindow.getSize().x, mWindow.getSize().y));
    mWindow.setView(uiView);
    if (mIsInventoryOpen) {
        // 1. Darken the background to highlight the menu
        sf::RectangleShape overlay(sf::Vector2f(mWindow.getSize().x, mWindow.getSize().y));
        overlay.setFillColor(sf::Color(0, 0, 0, 150)); // Semi-transparent black
        mWindow.draw(overlay);

        float slotSize = 48.0f;
        float padding = 8.0f;

        // ==========================================
        // A) THE BACKPACK 3x10 (Terraria Style)
        // ==========================================
        // Calculate the bottom center of the screen
        float startX = (mWindow.getSize().x / 2.0f) - ((10 * slotSize + 9 * padding) / 2.0f);
        float startY = mWindow.getSize().y - (3 * slotSize + 2 * padding) - 50.0f;

        for (int row = 0; row < 3; ++row) {
            for (int col = 0; col < 10; ++col) {
                int index = row * 10 + col; // Index from 0 to 29

                sf::RectangleShape slotBg(sf::Vector2f(slotSize, slotSize));
                slotBg.setPosition(startX + col * (slotSize + padding), startY + row * (slotSize + padding));
                slotBg.setFillColor(sf::Color(40, 40, 40, 200));
                slotBg.setOutlineThickness(2.0f);
                slotBg.setOutlineColor(sf::Color(100, 100, 100));
                mWindow.draw(slotBg);

                // Draw the item if there is something in this slot
                if (mBackpack[index].id != 0) {
                    const sf::Texture* tex = mWorld.getTexture(mBackpack[index].id);
                    if (tex) {
                        sf::Sprite icon(*tex);
                        float scale = (slotSize - 10.0f) / tex->getSize().x;
                        icon.setScale(scale, scale);
                        icon.setPosition(slotBg.getPosition().x + 5.0f, slotBg.getPosition().y + 5.0f);
                        mWindow.draw(icon);

                        // Draw quantity
                        mUiText.setString(std::to_string(mBackpack[index].count));
                        mUiText.setCharacterSize(14);
                        sf::FloatRect textBounds = mUiText.getLocalBounds();
                        mUiText.setPosition(
                            slotBg.getPosition().x + slotSize - textBounds.width - 4.0f,
                            slotBg.getPosition().y + slotSize - textBounds.height - 6.0f
                        );
                        mWindow.draw(mUiText);
                    }
                }
            }
        }

        // ==========================================
        // B) THE TACTICAL WHEEL (With your Sprite)
        // ==========================================
        float wheelCX = mWindow.getSize().x / 2.0f;
        float wheelCY = mWindow.getSize().y / 2.0f - 100.0f;

        // Draw the ring
        mWheelSprite.setPosition(wheelCX, wheelCY);
        mWindow.draw(mWheelSprite);

        // Create a quick list with the 4 tactical slots to draw them easily
        // Order: 0=Up(Usable), 1=Down(Block), 2=Left(Weapon2), 3=Right(Weapon1)
        InventorySlot* wheelSlots[4] = { &mEquippedConsumable, &mEquippedBlock, &mEquippedSecondary, &mEquippedPrimary };

        // Relative positions where the ICONS will go with respect to the center of the wheel
        float offset = 100.0f; // The center of the blue ring
        sf::Vector2f wheelPositions[4] = {
            sf::Vector2f(0, -offset),  // Up
            sf::Vector2f(0, offset),   // Down
            sf::Vector2f(-offset, 0),  // Left
            sf::Vector2f(offset, 0)    // Right
        };
        std::string wheelLabels[4] = {"Usable", "Block", "Weapon 2", "Weapon 1"};

        // NEW: Fine adjustments to push TEXT outside the blue ring
        sf::Vector2f textOffsets[4] = {
            sf::Vector2f(0.0f, -45.0f),  // "Usable": Higher
            sf::Vector2f(0.0f, 25.0f),   // "Block": Lower
            sf::Vector2f(-45.0f, -10.0f),// "Weapon 2": More to the left
            sf::Vector2f(45.0f, -10.0f)  // "Weapon 1": More to the right
        };

        for (int i = 0; i < 4; ++i) {
            float slotX = wheelCX + wheelPositions[i].x;
            float slotY = wheelCY + wheelPositions[i].y;

            // Draw the slot name (with its own offset)
            mUiText.setString(wheelLabels[i]);
            mUiText.setCharacterSize(18); // A bit larger to be readable
            mUiText.setOutlineThickness(2.0f); // Thick black border
            sf::FloatRect textBounds = mUiText.getLocalBounds();

            // Apply custom text offset
            mUiText.setPosition(
                slotX + textOffsets[i].x - (textBounds.width / 2.0f),
                slotY + textOffsets[i].y
            );
            mWindow.draw(mUiText);

            // If there is an item equipped there, draw it
            if (wheelSlots[i]->id != 0) {
                const sf::Texture* tex = mWorld.getTexture(wheelSlots[i]->id);
                if (tex) {
                    sf::Sprite icon(*tex);
                    float scale = 40.0f / tex->getSize().x; // Icon size
                    icon.setScale(scale, scale);
                    icon.setOrigin(tex->getSize().x / 2.0f, tex->getSize().y / 2.0f);
                    icon.setPosition(slotX, slotY);
                    mWindow.draw(icon);

                    // Draw quantity
                    sf::Text qtyText = mUiText;
                    qtyText.setString(std::to_string(wheelSlots[i]->count));
                    qtyText.setCharacterSize(14);
                    qtyText.setPosition(slotX + 15.0f, slotY + 10.0f);
                    mWindow.draw(qtyText);
                }
            }
        }

        // ==========================================
        // C) DRAW DRAGGED ITEM (DRAG & DROP)
        // ==========================================
        if (mDraggedItem.id != 0) {
            const sf::Texture* tex = mWorld.getTexture(mDraggedItem.id);
            if (tex) {
                sf::Vector2i mousePos = sf::Mouse::getPosition(mWindow);

                sf::Sprite dragIcon(*tex);
                float scale = 40.0f / tex->getSize().x;
                dragIcon.setScale(scale, scale);
                // Center the icon on the mouse tip
                dragIcon.setOrigin(tex->getSize().x / 2.0f, tex->getSize().y / 2.0f);
                dragIcon.setPosition(static_cast<float>(mousePos.x), static_cast<float>(mousePos.y));
                mWindow.draw(dragIcon);

                // Quantity of dragged item
                mUiText.setString(std::to_string(mDraggedItem.count));
                mUiText.setCharacterSize(16);
                mUiText.setPosition(mousePos.x + 10.0f, mousePos.y + 10.0f);
                mWindow.draw(mUiText);
            }
        }
        // ==========================================
        // D) CRAFTING MENU (Diseño Mejorado)
        // ==========================================
        float craftX = 50.0f;
        float craftY = 100.0f;
        float rowHeight = 60.0f;
        float panelWidth = 320.0f; // Panel ancho para mostrar ingredientes

        // Menu title
        mUiText.setString("CRAFTING");
        mUiText.setCharacterSize(24);
        mUiText.setOutlineThickness(2.0f);
        mUiText.setPosition(craftX, craftY - 40.0f);
        mWindow.draw(mUiText);

        for (size_t i = 0; i < mRecipes.size(); ++i) {
            const Recipe& recipe = mRecipes[i];
            bool possible = canCraft(recipe);

            // 1. DIBUJAR EL FONDO DE LA FILA
            sf::RectangleShape rowBg(sf::Vector2f(panelWidth, rowHeight - 5.0f));
            rowBg.setPosition(craftX, craftY + i * rowHeight);
            rowBg.setFillColor(sf::Color(40, 40, 40, 200));

            if (possible) {
                rowBg.setOutlineColor(sf::Color(50, 200, 50, 200)); // Borde verde
                rowBg.setOutlineThickness(2.0f);
            } else {
                rowBg.setOutlineColor(sf::Color(100, 100, 100, 150)); // Borde gris
                rowBg.setOutlineThickness(1.0f);
            }
            mWindow.draw(rowBg);

            // 2. DIBUJAR EL ICONO DEL RESULTADO (A la izquierda)
            const sf::Texture* resTex = mWorld.getTexture(recipe.resultId);
            if (resTex) {
                sf::Sprite resIcon(*resTex);
                float scale = 40.0f / resTex->getSize().x;
                resIcon.setScale(scale, scale);
                resIcon.setPosition(craftX + 10.0f, craftY + i * rowHeight + 7.0f);

                if (!possible) resIcon.setColor(sf::Color(255, 255, 255, 100)); // Transparente si no puedes
                mWindow.draw(resIcon);

                // Si fabrica más de 1 (ej: 4 Antorchas), mostrar el número
                if (recipe.resultCount > 1) {
                    mUiText.setString(std::to_string(recipe.resultCount));
                    mUiText.setCharacterSize(16);
                    mUiText.setFillColor(sf::Color::Yellow); // Amarillo para destacar
                    mUiText.setPosition(craftX + 35.0f, craftY + i * rowHeight + 32.0f);
                    mWindow.draw(mUiText);
                    mUiText.setFillColor(sf::Color::White); // Restaurar color
                }
            }

            // 3. DIBUJAR LOS INGREDIENTES REQUERIDOS (A la derecha del resultado)
            float ingX = craftX + 80.0f; // Empezamos a dibujar más a la derecha

            for (const auto& ing : recipe.ingredients) {
                int ingID = ing.first;
                int reqAmount = ing.second;
                int hasAmount = getItemCount(ingID);

                const sf::Texture* ingTex = mWorld.getTexture(ingID);
                if (ingTex) {
                    // Icono del ingrediente en chiquito
                    sf::Sprite ingIcon(*ingTex);
                    float scale = 24.0f / ingTex->getSize().x;
                    ingIcon.setScale(scale, scale);
                    ingIcon.setPosition(ingX, craftY + i * rowHeight + 15.0f);
                    if (!possible) ingIcon.setColor(sf::Color(255, 255, 255, 150));
                    mWindow.draw(ingIcon);

                    // Texto: Tienes / Necesitas (ej: "2/5")
                    mUiText.setString(std::to_string(hasAmount) + "/" + std::to_string(reqAmount));
                    mUiText.setCharacterSize(14);

                    // Si tienes menos de lo necesario, el texto se pone ROJO
                    if (hasAmount >= reqAmount) {
                        mUiText.setFillColor(sf::Color::White);
                    } else {
                        mUiText.setFillColor(sf::Color(255, 80, 80));
                    }

                    mUiText.setPosition(ingX + 30.0f, craftY + i * rowHeight + 18.0f);
                    mWindow.draw(mUiText);

                    mUiText.setFillColor(sf::Color::White); // Restaurar a blanco siempre
                    ingX += 75.0f; // Espacio para el siguiente ingrediente (Pico de hierro necesita 2 cosas)
                }
            }
        }
    }

    // ALWAYS THE LAST THING
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

    // 2.5 SAVE EQUIPPED ITEMS (Rueda Táctica)
    InventorySlot equipped[4] = { mEquippedPrimary, mEquippedSecondary, mEquippedBlock, mEquippedConsumable };
    for (int i = 0; i < 4; ++i) {
        file.write(reinterpret_cast<const char*>(&equipped[i].id), sizeof(equipped[i].id));
        file.write(reinterpret_cast<const char*>(&equipped[i].count), sizeof(equipped[i].count));
    }

    // 3. SAVE WORLD
    mWorld.saveToStream(file);

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

    // 2.5 LOAD EQUIPPED ITEMS (Rueda Táctica)
    InventorySlot* equippedPointers[4] = { &mEquippedPrimary, &mEquippedSecondary, &mEquippedBlock, &mEquippedConsumable };
    for (int i = 0; i < 4; ++i) {
        file.read(reinterpret_cast<char*>(&equippedPointers[i]->id), sizeof(equippedPointers[i]->id));
        file.read(reinterpret_cast<char*>(&equippedPointers[i]->count), sizeof(equippedPointers[i]->count));
    }

    // Le avisamos al jugador de qué arma acabamos de cargar en su mano principal
    mPlayer.setEquippedWeapon(mEquippedPrimary.id);

    // SUPER IMPORTANT! After loading everything, recalculate weight
    calculateTotalWeight();

    // 3. LOAD WORLD
    mWorld.loadFromStream(file);

    file.close();
    std::cout << "--- GAME LOADED ---" << std::endl;
}

void Game::calculateTotalWeight() {
    mCurrentWeight = 0.0f;
    for (const auto& slot : mBackpack) {
        if (slot.id != 0) {
            mCurrentWeight += mItemDatabase[slot.id].weight * slot.count;
        }
    }
    std::cout << "Current weight: " << mCurrentWeight << " / " << mMaxWeight << std::endl;
}

bool Game::addItemToBackpack(int id, int amount) {
    // 1. Try to stack in a slot that already has this item
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
    // 2. If leftover, find an empty slot
    for (auto& slot : mBackpack) {
        if (slot.id == 0) {
            slot.id = id;
            slot.count = amount;
            calculateTotalWeight();
            return true;
        }
    }
    return false; // Backpack full!
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