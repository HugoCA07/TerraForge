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
    , mSoundTimer(0.0f) // Inicializamos el temporizador
{
    mWindow.setFramerateLimit(120);

    // --- CARGAR SONIDOS ---
    // 1. HIT (Golpe)
    if (mBufHit.loadFromFile("assets/Hit.wav")) {
        mSndHit.setBuffer(mBufHit);
        mSndHit.setVolume(50.0f); // Volumen al 50%
        // Variación de tono aleatoria para que no suene robótico
        mSndHit.setPitch(1.0f);
    }

    // 2. BREAK (Rotura)
    if (mBufBreak.loadFromFile("assets/Break.wav")) {
        mSndBreak.setBuffer(mBufBreak);
        mSndBreak.setVolume(70.0f);
    }

    // 3. BUILD (Construir)
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

    if (!mDodoTexture.loadFromFile("assets/Dodo.png")) {
        std::cerr << "Error: Faltan los Dodos" << std::endl;
    }
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

    // --- UPDATE WORLD ITEMS ---
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

    // --- INVOCAR DODOS (BLINDADO) ---
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::O)) {
        // Le pedimos la posición explícitamente a la vista actual
        sf::Vector2i mousePos = sf::Mouse::getPosition(mWindow);
        sf::Vector2f worldPos = mWindow.mapPixelToCoords(mousePos, mWindow.getView());

        int gX = static_cast<int>(std::floor(worldPos.x / mWorld.getTileSize()));
        int gY = static_cast<int>(std::floor(worldPos.y / mWorld.getTileSize()));

        // Si apuntamos al aire, nace el Dodo
        if (mWorld.getBlock(gX, gY) == 0) {
            mDodos.push_back(Dodo(worldPos, mDodoTexture));
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

    // --------------------------------------------------
    // LEFT CLICK: COMBAT AND MINING
    // --------------------------------------------------
    if (sf::Mouse::isButtonPressed(sf::Mouse::Left)) {

        // 1. Calcular Daño de Herramienta
        int toolDamage = 1; // Puños
        if (mSelectedBlock == 21) toolDamage = 3;
        if (mSelectedBlock == 22) toolDamage = 5;
        if (mSelectedBlock == 23) toolDamage = 10;
        if (mSelectedBlock == 24) toolDamage = 20;

        bool hitEntity = false;

        // 2. Comprobar Combate con Dodos
        for (auto& dodo : mDodos) {
            // Si hacemos clic en su hitbox y estamos cerca
            if (dodo.getBounds().contains(worldPos) && distance <= maxRange) {
                float dir = (dodo.getPosition().x > playerCenter.x) ? 1.0f : -1.0f;

                if (dodo.takeDamage(toolDamage, dir)) {
                    mSndHit.setPitch(1.2f); // Tono agudo para golpe de carne
                    mSndHit.play();
                }
                hitEntity = true;
                break; // Solo pegamos a 1 Dodo por clic
            }
        }

        // 3. Si no le dimos a un Dodo, Minamos el bloque
        if (!hitEntity && distance <= maxRange) {

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

                if (mSelectedBlock == 21) miningSpeed = 3.0f;
                if (mSelectedBlock == 22) miningSpeed = 5.0f;
                if (mSelectedBlock == 23) miningSpeed = 10.0f;
                if (mSelectedBlock == 24) miningSpeed = 20.0f;

                int targetID = mWorld.getBlock(gridX, gridY);
                bool isHardBlock = (targetID == 3 || (targetID >= 7 && targetID <= 11));
                bool holdingPickaxe = (mSelectedBlock >= 21 && mSelectedBlock <= 24);

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
        }
    }
    else {
        // Soltamos clic izquierdo: Resetear minería
        mMiningTimer = 0.0f;
        mMiningPos = sf::Vector2i(-1, -1);
    }

    /// --------------------------------------------------
    // RIGHT CLICK: PLACE BLOCKS OR EAT!
    // --------------------------------------------------
    if (sf::Mouse::isButtonPressed(sf::Mouse::Right)) {

        // --- 1. ¿ES COMIDA? (ID 30 = Carne) ---
        if (mSelectedBlock == 30) {
            // Comprobamos si tenemos carne y si nos falta vida
            if (mInventory[30] > 0 && mPlayer.getHp() < mPlayer.getMaxHp()) {
                mInventory[30]--;         // Gastamos 1 carne
                mPlayer.heal(20);         // Curamos 20 de vida

                // Sonido de comer (reusamos un sonido bajándole el tono para que suene a "Ñam")
                mSndBuild.setPitch(0.5f);
                mSndBuild.play();

                std::cout << "¡Ñam! Vida actual: " << mPlayer.getHp() << std::endl;
                sf::sleep(sf::milliseconds(250)); // Pausa para no comernos 10 de golpe
            }
        }
        // --- 2. SI NO ES COMIDA, CONSTRUIMOS UN BLOQUE ---
        else if (distance <= maxRange) {
            if (mWorld.getBlock(gridX, gridY) == 0) {
                sf::FloatRect blockRect(gridX * tileSize, gridY * tileSize, tileSize, tileSize);

                if (!mPlayer.getGlobalBounds().intersects(blockRect)) {
                    if (mInventory[mSelectedBlock] > 0) {
                        mWorld.setBlock(gridX, gridY, mSelectedBlock);
                        mInventory[mSelectedBlock]--;

                        mSndBuild.setPitch(1.0f + (rand() % 20) / 100.0f);
                        mSndBuild.play();

                        // Pequeña pausa al construir también va bien
                        sf::sleep(sf::milliseconds(150));
                    }
                }
            }
        }
    }

    // --------------------------------------------------
    // DAY / NIGHT CYCLE LOGIC
    // --------------------------------------------------
    mGameTime += dt.asSeconds() * 0.5f;
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
    // SISTEMA DE GUARDADO (F5 / F6)
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
    // ACTUALIZAR DODOS Y ELIMINAR CADÁVERES
    // --------------------------------------------------
    for (auto it = mDodos.begin(); it != mDodos.end(); ) {
        it->update(dt, mPlayer.getPosition(), mWorld);

        if (it->isDead()) {
            // Soltar carne (ID 30)
            mWorld.spawnItem(30, it->getPosition());

            // Sonido al morir
            mSndBreak.setPitch(1.5f);
            mSndBreak.play();

            // Borrar de la lista
            it = mDodos.erase(it);
        } else {
            ++it;
        }
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
    for (auto& dodo : mDodos) {
        dodo.render(mWindow, mAmbientLight);
    }

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
    // DIBUJAR BARRA DE VIDA (HUD)
    // --------------------------------------------------
    // Calculamos el porcentaje de vida (0.0 a 1.0)
    float hpPercent = static_cast<float>(mPlayer.getHp()) / mPlayer.getMaxHp();

    // Posición: Debajo del inventario (X: 20, Y: 80)
    float barX = 20.0f;
    float barY = 80.0f;
    float barWidth = 200.0f;
    float barHeight = 20.0f;

    // 1. Fondo (Rojo oscuro)
    sf::RectangleShape hpBg(sf::Vector2f(barWidth, barHeight));
    hpBg.setPosition(barX, barY);
    hpBg.setFillColor(sf::Color(100, 0, 0, 200));
    hpBg.setOutlineThickness(2.0f);
    hpBg.setOutlineColor(sf::Color::Black);
    mWindow.draw(hpBg);

    // 2. Relleno de la barra (Rojo brillante o Verde)
    sf::RectangleShape hpFill(sf::Vector2f(barWidth * hpPercent, barHeight));
    hpFill.setPosition(barX, barY);
    hpFill.setFillColor(sf::Color(255, 50, 50, 255)); // Rojo brillante
    mWindow.draw(hpFill);

    // 3. (Opcional) Texto de Vida "50/100"
    sf::Text hpText = mUiText; // Copiamos la fuente
    hpText.setString(std::to_string(mPlayer.getHp()) + " / " + std::to_string(mPlayer.getMaxHp()));
    hpText.setCharacterSize(14);
    hpText.setPosition(barX + barWidth / 2.0f - 25.0f, barY); // Centrado a ojo
    mWindow.draw(hpText);


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