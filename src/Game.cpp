#include "Game.h"
#include <cmath> // Necessary for std::sqrt
#include <iostream>

// Constructor
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
{
    mWindow.setFramerateLimit(120);

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

    // INITIAL INVENTORY
    // We start poor (0 blocks), you'll have to mine to build.
    mInventory[1] = 0; // Dirt
    mInventory[2] = 0; // Grass
    mInventory[3] = 0; // Stone
    mInventory[4] = 0; // Wood
    mInventory[5] = 0; // Leaves
    mInventory[6] = 50; // Torch, 50 just to try
    mInventory[7] = 0;
    mInventory[8] = 0;
    mInventory[9] = 0;
    mInventory[10] = 0;
    mInventory[11] = 0;
}

Game::~Game() {
}

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

void Game::processEvents() {
    sf::Event event;
    while (mWindow.pollEvent(event)) {
        if (event.type == sf::Event::Closed)
            mWindow.close();
        if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape)
            mWindow.close();
    }
    mPlayer.handleInput();
}

// Función auxiliar para saber la dureza
float getBlockHardness(int id) {
    switch (id) {
        case 0: return 0.0f;  // Aire
        case 1: return 0.2f;  // Tierra (Muy rápido)
        case 2: return 0.25f; // Hierba
        case 4: return 0.6f;  // Madera
        case 5: return 0.1f;  // Hojas (Instantáneo)
        case 6: return 0.1f;  // Antorcha

            // PIEDRA Y MINERALES
        case 3: return 1.0f;  // Piedra (Estándar)
        case 7: return 1.2f;  // Carbón
        case 8: return 1.5f;  // Cobre
        case 9: return 2.0f;  // Hierro
        case 10: return 3.0f; // Cobalto (Duro)
        case 11: return 5.0f; // Tungsteno (Muy duro)

        case 12: return -1.0f; // Bedrock (Indestructible)

        default: return 1.0f;
    }
}

// ---------------------------------------------------------
// UPDATE (LOGIC)
// ---------------------------------------------------------
void Game::update(sf::Time dt) {
    mPlayer.update(dt, mWorld);

    // --- NEW: UPDATE WORLD ITEMS ---
    // We pass the player center to see if they pick something up
    mWorld.update(dt, mPlayer.getCenter(), mInventory);

    // Selection keys
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num1)) mSelectedBlock = 1;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num2)) mSelectedBlock = 2;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num3)) mSelectedBlock = 3;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num4)) mSelectedBlock = 4;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num5)) mSelectedBlock = 5;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num6)) mSelectedBlock = 6;
    // Teclas para herramientas
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num7)) mSelectedBlock = 21; // Madera
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num8)) mSelectedBlock = 22; // Piedra
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num9)) mSelectedBlock = 23; // Hierro
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num0)) mSelectedBlock = 24; // Tungsteno


    // CAMERA
    sf::View view = mWindow.getDefaultView();
    view.zoom(1.25f);
    view.setCenter(mPlayer.getPosition());
    mWindow.setView(view);

    // --- RANGE AND MINING LOGIC ---

    sf::Vector2i pixelPos = sf::Mouse::getPosition(mWindow);
    sf::Vector2f worldPos = mWindow.mapPixelToCoords(pixelPos);
    float tileSize = mWorld.getTileSize();

    // 1. DETERMINE WHICH BLOCK WE ARE LOOKING AT
    int gridX = static_cast<int>(std::floor(worldPos.x / tileSize));
    int gridY = static_cast<int>(std::floor(worldPos.y / tileSize));

    // 2. CALCULATE DISTANCE TO THE CENTER OF THAT BLOCK (NOT TO THE MOUSE)
    // Center coordinate of the target block
    float blockCenterX = (gridX * tileSize) + (tileSize / 2.0f);
    float blockCenterY = (gridY * tileSize) + (tileSize / 2.0f);

    sf::Vector2f playerCenter = mPlayer.getPosition(); // It is already the center thanks to setOrigin

    float dx = blockCenterX - playerCenter.x;
    float dy = blockCenterY - playerCenter.y;
    float distance = std::sqrt(dx*dx + dy*dy);

    float maxRange = 180.0f;

    // We only allow interaction if the BLOCK is close
    if (distance <= maxRange) {

        // Si NO estamos pulsando clic izquierdo, reseteamos el progreso
        if (!sf::Mouse::isButtonPressed(sf::Mouse::Left)) {
            mMiningTimer = 0.0f;
            mMiningPos = sf::Vector2i(-1, -1); // Posición inválida
        }
        else if (distance <= maxRange) {
            // Estamos pulsando clic izquierdo y estamos en rango

            // 1. ¿Hemos cambiado de bloque?
            // Si movemos el ratón a otro bloque, reiniciamos el contador
            if (mMiningPos.x != gridX || mMiningPos.y != gridY) {
                mMiningTimer = 0.0f;
                mMiningPos = sf::Vector2i(gridX, gridY);

                // Obtenemos la dureza del nuevo bloque
                int blockID = mWorld.getBlock(gridX, gridY);
                mCurrentHardness = getBlockHardness(blockID);
            }

            // 2. Minar (Solo si es un bloque válido y no es aire)
            if (mCurrentHardness > 0.0f) {
                // 1. CALCULAR VELOCIDAD DE LA HERRAMIENTA
                float miningSpeed = 1.0f; // Velocidad base (Mano)

                if (mSelectedBlock == 21) miningSpeed = 3.0f;  // Pico Madera (3x más rápido)
                if (mSelectedBlock == 22) miningSpeed = 5.0f;  // Pico Piedra
                if (mSelectedBlock == 23) miningSpeed = 10.0f; // Pico Hierro
                if (mSelectedBlock == 24) miningSpeed = 20.0f; // Pico Tungsteno (Insta-mine)

                /// --- NUEVO: RESTRICCIÓN DE MATERIALES ---
                int targetID = mWorld.getBlock(gridX, gridY);

                // Definimos qué es "Duro" (Piedra y todos los Minerales del 7 al 11)
                bool isHardBlock = (targetID == 3 || (targetID >= 7 && targetID <= 11));

                // Definimos si tenemos un Pico en la mano
                bool holdingPickaxe = (mSelectedBlock >= 21 && mSelectedBlock <= 24);

                // REGLA: Si es duro y NO tienes pico, velocidad es 0.
                if (isHardBlock && !holdingPickaxe) {
                    miningSpeed = 0.0f;
                    // Aquí podrías poner un sonido de "Clunk" (error) más adelante
                }

                // 2. APLICAR TIEMPO CON MULTIPLICADOR
                mMiningTimer += dt.asSeconds() * miningSpeed;

                // 3. ¿Hemos terminado? (IGUAL QUE ANTES)
                if (mMiningTimer >= mCurrentHardness) {
                    // ... (Romper bloque, igual que tenías) ...
                    mWorld.setBlock(gridX, gridY, 0);

                    // IMPORTANTE: REINICIAR
                    mMiningTimer = 0.0f;
                    mCurrentHardness = 0.0f;
                }
            }
            else if (mCurrentHardness == -1.0f) {
                // Es Bedrock, no hacemos nada (quizás un sonido de 'ding' metálico)
                mMiningTimer = 0.0f;
            }
        }

        // Right Click (Place)
        if (sf::Mouse::isButtonPressed(sf::Mouse::Right)) {
            if (mWorld.getBlock(gridX, gridY) == 0) {
                sf::FloatRect blockRect(gridX * tileSize, gridY * tileSize, tileSize, tileSize);

                if (!mPlayer.getGlobalBounds().intersects(blockRect)) {

                    // --- NEW: CHECK IF YOU HAVE MATERIAL ---
                    if (mInventory[mSelectedBlock] > 0) {

                        mWorld.setBlock(gridX, gridY, mSelectedBlock);

                        // SPEND 1 BLOCK
                        mInventory[mSelectedBlock]--;
                    }
                }
            }
        }
    }
    // --------------------------------------------------
    // DAY / NIGHT CYCLE LOGIC
    // --------------------------------------------------
    // Advance time. (Multiply by 0.5f to make the day last longer)
    mGameTime += dt.asSeconds() * 0.5f;

    // We use SINE to make a smooth wave between -1 and 1
    // A full day is 2*PI radians.
    float cycle = std::sin(mGameTime);

    // Convert range from (-1 to 1) -> (0.2 to 1.0)
    // 1.0 = Noon (Full brightness)
    // 0.2 = Midnight (Darkness, but not total so we can see something)
    float brightness = (cycle * 0.4f) + 0.6f;

    // Calculate RGB color components
    sf::Uint8 r = static_cast<sf::Uint8>(255 * brightness);
    sf::Uint8 g = static_cast<sf::Uint8>(255 * brightness);
    sf::Uint8 b = static_cast<sf::Uint8>(255 * brightness);

    // TRICK: At night (low brightness), we make the light slightly BLUISH
    // to give a moonlight sensation.
    if (brightness < 0.5f) {
        b = static_cast<sf::Uint8>(std::min(255, b + 40));
    }

    mAmbientLight = sf::Color(r, g, b);

    // --------------------------------------------------
    // SISTEMA DE GUARDADO (F5 / F6)
    // --------------------------------------------------

    // F5: GUARDAR
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::F5)) {
        saveGame();
        // Pequeña pausa para no guardar 60 veces por segundo
        sf::sleep(sf::milliseconds(300));
    }

    // F6: CARGAR
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::F6)) {
        loadGame();
        sf::sleep(sf::milliseconds(300));
    }
}

// ---------------------------------------------------------
// RENDER (DRAWING)
// ---------------------------------------------------------
void Game::render() {
    mWindow.clear(sf::Color(135, 206, 235));

    // 1. DRAW SKY
    mSkySprite.setColor(mAmbientLight);
    mWindow.draw(mSkySprite);

    // 2. DRAW WORLD AND PLAYER
    // Aquí la cámara está enfocando al mundo correctamente
    mWorld.render(mWindow, mAmbientLight);
    mPlayer.render(mWindow, mAmbientLight);

    // --------------------------------------------------
    // DRAW MOUSE SELECTOR
    // --------------------------------------------------
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
    // DIBUJAR BARRA DE PROGRESO DE MINADO (AQUÍ ES EL LUGAR CORRECTO)
    // --------------------------------------------------
    // Lo ponemos aquí porque la "Vista del Juego" sigue activa.
    // No necesitamos hacer 'setView(view)' porque ya estamos en esa vista.

    if (mMiningTimer > 0.0f && mCurrentHardness > 0.0f) {

        // Calcular porcentaje (0.0 a 1.0)
        float percent = mMiningTimer / mCurrentHardness;
        if (percent > 1.0f) percent = 1.0f;

        // Fondo de la barra (Negro)
        sf::RectangleShape barBg(sf::Vector2f(tileSize * 0.8f, 6.0f));
        barBg.setPosition(
            mMiningPos.x * tileSize + tileSize * 0.1f,
            mMiningPos.y * tileSize + tileSize * 0.1f // Un poco arriba del bloque
        );
        barBg.setFillColor(sf::Color::Black);
        mWindow.draw(barBg);

        // Relleno de la barra (Blanco)
        sf::RectangleShape barFill(sf::Vector2f((tileSize * 0.8f) * percent, 4.0f));
        barFill.setPosition(
            barBg.getPosition().x + 1.0f, // Margen de 1px
            barBg.getPosition().y + 1.0f
        );
        barFill.setFillColor(sf::Color::White);
        mWindow.draw(barFill);
    }

    // --------------------------------------------------
    // UI / HUD SYSTEM (HOTBAR)
    // --------------------------------------------------
    // AHORA cambiamos a la vista de pantalla para dibujar el inventario encima de todo
    mWindow.setView(mWindow.getDefaultView());

    float uiX = 20.0f;
    float uiY = 20.0f;
    float slotSize = 40.0f;
    float padding = 10.0f;

    for (int i = 1; i <= 10; ++i) {
        int realID = i;
        if (i == 7) realID = 21; // Slot 7 = Pico Madera
        if (i == 8) realID = 22; // Slot 8 = Pico Piedra
        if (i == 9) realID = 23; // Slot 9 = Pico Hierro
        if (i == 10) realID = 24; // Slot 10 = Pico Tungsteno
        // 1. DRAW SLOT BOX
        sf::RectangleShape slot(sf::Vector2f(slotSize, slotSize));
        slot.setPosition(uiX + (i-1) * (slotSize + padding), uiY);
        slot.setFillColor(sf::Color(0, 0, 0, 150));
        slot.setOutlineThickness(2.0f);

        if (mSelectedBlock == realID) {
            slot.setOutlineColor(sf::Color::White);
            slot.setScale(1.1f, 1.1f);
        } else {
            slot.setOutlineColor(sf::Color(100, 100, 100));
        }
        mWindow.draw(slot);

        // 2. DRAW BLOCK ICON
        const sf::Texture* tex = mWorld.getTexture(realID);
        if (tex != nullptr) {
            sf::Sprite icon;
            icon.setTexture(*tex);
            float iconMargin = 5.0f;
            float scale = (slotSize - iconMargin*2) / tex->getSize().x;
            icon.setScale(scale, scale);
            icon.setPosition(
                slot.getPosition().x + iconMargin,
                slot.getPosition().y + iconMargin
            );

            if (mInventory[realID] > 0) {
                icon.setColor(sf::Color(255, 255, 255, 255));
            } else {
                icon.setColor(sf::Color(255, 255, 255, 80));
            }
            mWindow.draw(icon);
        }

        // 3. DRAW QUANTITY
        if (mInventory[realID] > 0) {
            mUiText.setString(std::to_string(mInventory[realID]));
            sf::FloatRect textBounds = mUiText.getLocalBounds();
            mUiText.setPosition(
                slot.getPosition().x + slotSize - textBounds.width - 5.0f,
                slot.getPosition().y + slotSize - textBounds.height - 8.0f
            );
            mWindow.draw(mUiText);
        }

        // 4. DRAW KEY (NÚMERO PEQUEÑO ARRIBA IZQ)
        sf::Text keyText = mUiText;

        // --- CORRECCIÓN AQUÍ ---
        // En lugar de usar 'realID' (que es 21, 22...), usamos 'i' (que es 7, 8...).
        // Además, si es el slot 10, escribimos "0" porque esa es la tecla en el teclado.
        std::string keyString = std::to_string(i);
        if (i == 10) keyString = "0";

        keyText.setString(keyString);

        keyText.setCharacterSize(12);
        keyText.setFillColor(sf::Color(200, 200, 200));
        keyText.setPosition(slot.getPosition().x + 3.0f, slot.getPosition().y + 1.0f);
        mWindow.draw(keyText);
    }

    // --------------------------------------------------
    // CRAFTING SYSTEM (Teclas F1 - F4)
    // --------------------------------------------------

    // F1: PICO DE MADERA (ID 21) - Requiere 10 Madera (ID 4)
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::F1)) {
        if (mInventory[4] >= 10) { // ¿Tienes materiales?
            mInventory[4] -= 10;   // Restar coste
            mInventory[21]++;      // Dar premio
            std::cout << "Crafteado: Pico de Madera!" << std::endl;
            sf::sleep(sf::milliseconds(200)); // Pequeña pausa para no craftear 100 de golpe
        }
    }

    // F2: PICO DE PIEDRA (ID 22) - Requiere 10 Piedra (ID 3)
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::F2)) {
        if (mInventory[3] >= 10) {
            mInventory[3] -= 10;
            mInventory[22]++;
            std::cout << "Crafteado: Pico de Piedra!" << std::endl;
            sf::sleep(sf::milliseconds(200));
        }
    }

    // F3: PICO DE HIERRO (ID 23) - Requiere 5 Hierro (ID 9) + 5 Madera (ID 4)
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::F3)) {
        if (mInventory[9] >= 5 && mInventory[4] >= 5) {
            mInventory[9] -= 5;
            mInventory[4] -= 5;
            mInventory[23]++;
            std::cout << "Crafteado: Pico de Hierro!" << std::endl;
            sf::sleep(sf::milliseconds(200));
        }
    }

    // F4: PICO DE TUNGSTENO (ID 24) - Requiere 5 Tungsteno (ID 11) + 5 Madera (ID 4)
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::F4)) {
        if (mInventory[11] >= 5 && mInventory[4] >= 5) {
            mInventory[11] -= 5;
            mInventory[4] -= 5;
            mInventory[24]++;
            std::cout << "Crafteado: EL DESTRUCTOR DE MUNDOS!" << std::endl;
            sf::sleep(sf::milliseconds(200));
        }
    }

    // SIEMPRE LO ÚLTIMO
    mWindow.display();
}

void Game::saveGame() {
    std::ofstream file("savegame.dat", std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Error: No se pudo crear el archivo de guardado." << std::endl;
        return;
    }

    // 1. GUARDAR JUGADOR (Posición X, Y)
    sf::Vector2f pos = mPlayer.getPosition();
    file.write(reinterpret_cast<const char*>(&pos), sizeof(pos));

    // 2. GUARDAR INVENTARIO
    // Primero guardamos cuántos tipos de items tenemos
    size_t invSize = mInventory.size();
    file.write(reinterpret_cast<const char*>(&invSize), sizeof(invSize));

    // Guardamos cada par {ID, Cantidad}
    for (const auto& pair : mInventory) {
        int id = pair.first;
        int count = pair.second;
        file.write(reinterpret_cast<const char*>(&id), sizeof(id));
        file.write(reinterpret_cast<const char*>(&count), sizeof(count));
    }

    // 3. GUARDAR MUNDO
    mWorld.saveToStream(file);

    file.close();
    std::cout << "--- PARTIDA GUARDADA EXITOSAMENTE ---" << std::endl;
}

void Game::loadGame() {
    std::ifstream file("savegame.dat", std::ios::binary);
    if (!file.is_open()) {
        std::cout << "No se encontro partida guardada." << std::endl;
        return;
    }

    // 1. CARGAR JUGADOR
    sf::Vector2f pos;
    file.read(reinterpret_cast<char*>(&pos), sizeof(pos));
    mPlayer.setPosition(pos); // Necesitas asegurarte de que Player tiene este método (normalmente lo hereda de Sprite)

    // 2. CARGAR INVENTARIO
    mInventory.clear();
    size_t invSize = 0;
    file.read(reinterpret_cast<char*>(&invSize), sizeof(invSize));

    for (size_t i = 0; i < invSize; ++i) {
        int id = 0, count = 0;
        file.read(reinterpret_cast<char*>(&id), sizeof(id));
        file.read(reinterpret_cast<char*>(&count), sizeof(count));
        mInventory[id] = count;
    }

    // 3. CARGAR MUNDO
    mWorld.loadFromStream(file);

    file.close();
    std::cout << "--- PARTIDA CARGADA ---" << std::endl;
}