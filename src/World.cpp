#include "World.h"
#include <iostream>
#include <cmath>

World::World()
    : mTileSize(32.0f)
{
    // Load all block textures when the world is created.
    loadTextures();
}

// --- SMART GET BLOCK (INFINITE GENERATION) ---
int World::getBlock(int x, int y) {
    // 1. Boundary Check (Vertical is fixed)
    if (y < 0 || y >= WORLD_HEIGHT) {
        return 0; // Air outside vertical limits
    }

    // 2. Calculate Chunk Coordinate
    // Example: If x=20 and CHUNK_WIDTH=16, we are in Chunk 1.
    // We use std::floor() to handle negative coordinates correctly (e.g., x=-5 is in chunk -1).
    int chunkIndex = static_cast<int>(std::floor(x / (float)CHUNK_WIDTH));

    // 3. Check if Chunk exists in memory
    if (mChunks.find(chunkIndex) == mChunks.end()) {
        // If the chunk doesn't exist in our map, generate it on the fly.
        generateChunk(chunkIndex);
    }

    // 4. Retrieve Block from Chunk
    // We need to find the local X coordinate within that chunk (0 to CHUNK_WIDTH-1).
    // The modulo operator handles negative coordinates correctly to wrap them into the 0-15 range.
    int localX = (x % CHUNK_WIDTH + CHUNK_WIDTH) % CHUNK_WIDTH;
    int index = y * CHUNK_WIDTH + localX; // Convert 2D chunk coordinates to 1D vector index.

    return mChunks[chunkIndex][index];
}

void World::generateChunk(int chunkX) {
    std::vector<int> newChunk;
    std::vector<int> newBgChunk;

    int totalBlocks = CHUNK_WIDTH * WORLD_HEIGHT;
    newChunk.resize(totalBlocks, 0);
    newBgChunk.resize(totalBlocks, 0);

    float seed = 97.0f;
    int surfaceHeights[CHUNK_WIDTH];

    // ---------------------------------------------------------
    // PASO 1: TERRENO BASE (Solo Piedra, Tierra y Aire)
    // ---------------------------------------------------------
    for (int localX = 0; localX < CHUNK_WIDTH; ++localX) {
        int globalX = (chunkX * CHUNK_WIDTH) + localX;

        // Montañas suaves
        float n1 = std::sin((globalX + seed) / 50.0f);
        float n2 = std::sin((globalX + seed) / 25.0f) * 0.5f;
        float baseHeight = 80.0f;

        int surfaceY = static_cast<int>(baseHeight + ((n1 + n2) * 15.0f));
        surfaceHeights[localX] = surfaceY;

        for (int y = 0; y < WORLD_HEIGHT; ++y) {
            int index = y * CHUNK_WIDTH + localX;
            int blockID = 0;
            int bgID = 0;

            // Fondo
            if (y > surfaceY) {
                if (y < surfaceY + 10) bgID = 1; // Dirt wall
                else bgID = 3;                   // Stone wall
            }

            // Primer plano
            if (y >= WORLD_HEIGHT - 2) {
                blockID = 12; // Bedrock
            }
            else if (y >= surfaceY) {
                if (y == surfaceY) blockID = 2;       // Hierba
                else if (y < surfaceY + 5) blockID = 1; // Tierra
                else blockID = 3;                     // Piedra (AQUÍ NO PONEMOS MINERALES AÚN)
            }

            newChunk[index] = blockID;
            newBgChunk[index] = bgID;
        }
    }

    // ---------------------------------------------------------
    // PASO 2: GENERADOR DE MENAS (Vetas)
    // ---------------------------------------------------------
    // Función Lambda para "estampar" una veta
    auto spawnVein = [&](int count, int id, int minDepth, int maxDepth, int size) {
        for (int i = 0; i < count; ++i) {
            // 1. Elegir un punto central aleatorio dentro del chunk
            int cx = rand() % CHUNK_WIDTH;
            int cy = minDepth + (rand() % (maxDepth - minDepth));

            // 2. Crear una forma de "mancha" alrededor del centro
            // Hacemos un bucle pequeño alrededor del centro
            for (int vx = -1; vx <= 1; ++vx) {
                for (int vy = -1; vy <= 1; ++vy) {
                    // Probabilidad de rellenar para que no sea un cuadrado perfecto
                    if ((rand() % 100) > size) continue;

                    int nx = cx + vx;
                    int ny = cy + vy;

                    // Comprobar límites
                    if (nx >= 0 && nx < CHUNK_WIDTH && ny >= 0 && ny < WORLD_HEIGHT) {
                        int index = ny * CHUNK_WIDTH + nx;

                        // SOLO reemplazamos PIEDRA (ID 3).
                        // No rompemos tierra, ni aire, ni otros minerales.
                        if (newChunk[index] == 3) {
                            newChunk[index] = id;
                        }
                    }
                }
            }
        }
    };

    // --- CONFIGURACIÓN DE MINERALES ---
    // (Cantidad de intentos, ID, Profundidad Min, Profundidad Max, Probabilidad de relleno %)

    // CARBÓN (ID 7): Muy común, vetas grandes, cerca superficie
    spawnVein(6, 7, 20, 150, 80);

    // COBRE (ID 8): Común, vetas medianas
    spawnVein(4, 8, 30, 150, 70);

    // HIERRO (ID 9): Menos común, vetas medianas
    spawnVein(3, 9, 50, 150, 70);

    // COBALTO (ID 10): Raro, profundo, vetas pequeñas
    spawnVein(2, 10, 100, 150, 50);

    // TUNGSTENO (ID 11): Muy raro, muy profundo, vetas muy pequeñas
    spawnVein(1, 11, 130, 150, 40);


    // ---------------------------------------------------------
    // PASO 3: ÁRBOLES
    // ---------------------------------------------------------
    for (int localX = 0; localX < CHUNK_WIDTH; ++localX) {
        int globalX = (chunkX * CHUNK_WIDTH) + localX;
        int surfaceY = surfaceHeights[localX];

        if ((std::abs(globalX * 437) % 100) < 10 && localX > 1 && localX < CHUNK_WIDTH - 2) {
            int treeHeight = 4;
            int treeTopY = surfaceY - treeHeight;

            // Hojas
            for (int lx = -1; lx <= 1; ++lx) {
                for (int ly = -1; ly <= 1; ++ly) {
                    int leafY = treeTopY + ly;
                    int leafX = localX + lx;
                    if (leafY > 0 && leafY < WORLD_HEIGHT) {
                        int idx = leafY * CHUNK_WIDTH + leafX;
                        if (newChunk[idx] == 0) newChunk[idx] = 5;
                    }
                }
            }
            // Tronco
            for (int i = 1; i <= treeHeight; ++i) {
                int trunkY = surfaceY - i;
                if (trunkY > 0) newChunk[trunkY * CHUNK_WIDTH + localX] = 4;
            }
        }
    }

    mChunks[chunkX] = newChunk;
    mBackgroundChunks[chunkX] = newBgChunk;
}

void World::render(sf::RenderWindow& window, sf::Color ambientColor) {
    sf::View view = window.getView();
    float left = view.getCenter().x - (view.getSize().x / 2.f);
    float right = view.getCenter().x + (view.getSize().x / 2.f);
    float top = view.getCenter().y - (view.getSize().y / 2.f);
    float bottom = view.getCenter().y + (view.getSize().y / 2.f);

    int startChunk = static_cast<int>(std::floor(left / (CHUNK_WIDTH * mTileSize))) - 1;
    int endChunk = static_cast<int>(std::floor(right / (CHUNK_WIDTH * mTileSize))) + 1;

    // -----------------------------------------------------------
    // [NUEVO] PASO 0: ASEGURAR QUE EL MUNDO EXISTE
    // -----------------------------------------------------------
    // Antes de calcular luces o dibujar, revisamos si faltan chunks.
    // Si faltan, los generamos AHORA MISMO.
    for (int cx = startChunk; cx <= endChunk; ++cx) {
        if (mChunks.find(cx) == mChunks.end()) {
            generateChunk(cx);
        }
    }

    // -----------------------------------------------------------
    // PASO 1: ENCONTRAR TODAS LAS LUCES VISIBLES
    // -----------------------------------------------------------
    std::vector<sf::Vector2f> lightSources;

    for (int cx = startChunk; cx <= endChunk; ++cx) {
        auto it = mChunks.find(cx);
        if (it != mChunks.end()) {
            const auto& blocks = it->second; // Usamos el iterador (Más seguro)
            for (int y = 0; y < WORLD_HEIGHT; ++y) {
                float blockY = y * mTileSize;
                if (blockY < top - 200 || blockY > bottom + 200) continue;

                for (int lx = 0; lx < CHUNK_WIDTH; ++lx) {
                    // ID 6 = Antorcha
                    if (blocks[y * CHUNK_WIDTH + lx] == 6) {
                         float wx = (cx * CHUNK_WIDTH + lx) * mTileSize + mTileSize/2.f;
                         float wy = y * mTileSize + mTileSize/2.f;
                         lightSources.push_back(sf::Vector2f(wx, wy));
                    }
                }
            }
        }
    }

    // -----------------------------------------------------------
    // PREPARACIÓN DE SPRITES Y LUZ
    // -----------------------------------------------------------
    sf::Sprite sprite;
    float lightRadius = 250.0f;

    // Lambda para calcular luz
    auto calculateLight = [&](sf::Vector2f blockPos, sf::Color baseColor) -> sf::Color {
        float r = baseColor.r / 255.f;
        float g = baseColor.g / 255.f;
        float b = baseColor.b / 255.f;

        float maxTorchLight = 0.0f;

        for (const auto& lightPos : lightSources) {
            float dx = blockPos.x - lightPos.x;
            float dy = blockPos.y - lightPos.y;
            float distSq = dx*dx + dy*dy;
            float radiusSq = lightRadius * lightRadius;

            if (distSq < radiusSq) {
                float dist = std::sqrt(distSq);
                float intensity = 1.0f - (dist / lightRadius);
                if (intensity > maxTorchLight) maxTorchLight = intensity;
            }
        }

        r = std::max(r, maxTorchLight);
        g = std::max(g, maxTorchLight);
        b = std::max(b, maxTorchLight);

        if (r > 1.0f) r = 1.0f;
        if (g > 1.0f) g = 1.0f;
        if (b > 1.0f) b = 1.0f;

        if (maxTorchLight > 0.1f) b *= 0.8f; // Tinte cálido

        return sf::Color(
            static_cast<sf::Uint8>(r * 255),
            static_cast<sf::Uint8>(g * 255),
            static_cast<sf::Uint8>(b * 255)
        );
    };

    // -----------------------------------------------------------
    // LOOP 1: PAREDES (FONDO) - [CORREGIDO PARA EVITAR CRASH]
    // -----------------------------------------------------------
    for (int cx = startChunk; cx <= endChunk; ++cx) {

        // CORRECCIÓN: Usamos find() para obtener el puntero al vector de forma segura
        auto itBg = mBackgroundChunks.find(cx);

        // Si no existe el chunk de fondo o está vacío, saltamos
        if (itBg == mBackgroundChunks.end() || itBg->second.empty()) continue;

        const auto& bgBlocks = itBg->second; // Referencia segura

        for (int y = 0; y < WORLD_HEIGHT; ++y) {
             float py = y * mTileSize;
             if (py < top || py > bottom) continue;

            for (int lx = 0; lx < CHUNK_WIDTH; ++lx) {
                int blockID = bgBlocks[y * CHUNK_WIDTH + lx];
                if (blockID != 0 && mTextures.count(blockID)) {
                    sprite.setTexture(mTextures[blockID]);

                    float px = (cx * CHUNK_WIDTH + lx) * mTileSize;
                    sprite.setPosition(px, py);

                    float scale = mTileSize / sprite.getLocalBounds().width;
                    sprite.setScale(scale, scale);

                    sf::Vector2f center(px + mTileSize/2, py + mTileSize/2);

                    sf::Color wallBase = ambientColor;
                    wallBase.r /= 2; wallBase.g /= 2; wallBase.b /= 2;

                    sprite.setColor(calculateLight(center, wallBase));
                    window.draw(sprite);
                }
            }
        }
    }

    // -----------------------------------------------------------
    // LOOP 2: PRIMER PLANO (BLOCKS)
    // -----------------------------------------------------------
    for (int cx = startChunk; cx <= endChunk; ++cx) {
        auto it = mChunks.find(cx);
        if (it == mChunks.end()) continue; // Seguridad extra

        const auto& blocks = it->second;

        for (int y = 0; y < WORLD_HEIGHT; ++y) {
             float py = y * mTileSize;
             if (py < top || py > bottom) continue;

            for (int lx = 0; lx < CHUNK_WIDTH; ++lx) {
                int blockID = blocks[y * CHUNK_WIDTH + lx];
                if (blockID != 0 && mTextures.count(blockID)) {
                    sprite.setTexture(mTextures[blockID]);

                    float px = (cx * CHUNK_WIDTH + lx) * mTileSize;
                    sprite.setPosition(px, py);

                    float scale = mTileSize / sprite.getLocalBounds().width;
                    sprite.setScale(scale, scale);

                    if (blockID == 6) { // Antorcha brilla siempre
                        sprite.setColor(sf::Color::White);
                    } else {
                        sf::Vector2f center(px + mTileSize/2, py + mTileSize/2);
                        sprite.setColor(calculateLight(center, ambientColor));
                    }

                    window.draw(sprite);
                }
            }
        }
    }

    // -----------------------------------------------------------
    // LOOP 3: ITEMS
    // -----------------------------------------------------------
    for (const auto& item : mItems) {
        if (mTextures.count(item.id)) {
             sprite.setTexture(mTextures[item.id]);
             sprite.setOrigin(sprite.getLocalBounds().width/2, sprite.getLocalBounds().height/2);
             sprite.setPosition(item.pos);
             float scale = (mTileSize / sprite.getLocalBounds().width) * 0.4f;
             sprite.setScale(scale, scale);

             sprite.setColor(calculateLight(item.pos, ambientColor));

             window.draw(sprite);
        }
    }
    sprite.setOrigin(0,0);
}

void World::spawnItem(int x, int y, int id) {
    ItemDrop item;
    item.id = id;

    // Position: In the center of the broken block
    item.pos = sf::Vector2f(x * mTileSize + mTileSize/2, y * mTileSize + mTileSize/2);

    // Random Velocity ("Pop" Effect)
    // rand() % 100 - 50 gives a number between -50 and 50
    float randX = (rand() % 100 - 50) * 2.0f;
    float randY = -150.0f; // Initial upward jump

    item.vel = sf::Vector2f(randX, randY);
    item.timeAlive = 0.0f;

    mItems.push_back(item);
}

// --- MODIFY WORLD (MINING & BUILDING) ---
void World::setBlock(int x, int y, int type) {
    if (y < 0 || y >= WORLD_HEIGHT) return;

    int chunkIndex = static_cast<int>(std::floor(x / (float)CHUNK_WIDTH));
    int localX = (x % CHUNK_WIDTH + CHUNK_WIDTH) % CHUNK_WIDTH;

    if (mChunks.find(chunkIndex) == mChunks.end()) {
        generateChunk(chunkIndex);
    }

    // 1. GET THE OLD BLOCK
    int oldType = mChunks[chunkIndex][y * CHUNK_WIDTH + localX];

    // 2. CHANGE THE BLOCK
    mChunks[chunkIndex][y * CHUNK_WIDTH + localX] = type;

    // 3. GENERATE DROP (If we break a solid block and place air)
    // Only if the old block was not Air (0) and the new one is Air (0)
    if (type == 0 && oldType != 0) {
        spawnItem(x, y, oldType);
    }
}

void World::loadTextures() {
    // Helper lambda to load a texture easily
    auto load = [&](int id, const std::string& filename) {
        sf::Texture tex;
        if (!tex.loadFromFile(filename)) {
            std::cerr << "Error loading: " << filename << std::endl;
            // Create a fallback magenta square if file is missing
            sf::Image img;
            img.create(32, 32, sf::Color::Magenta);
            tex.loadFromImage(img);
        }
        mTextures[id] = tex;
    };

    // --- LOAD YOUR ASSETS HERE ---
    // Make sure these files exist in your "assets" folder!
    load(1, "assets/Dirt.png");  // ID 1: Dirt
    load(2, "assets/Grass.png");  // ID 2: Grass
    load(3, "assets/Stone.png");  // ID 3: Stone
    load(4, "assets/Tree.png");   // ID 4: Tree
    load(5, "assets/Leaves.png");   // ID 5: Leaves
    load(6, "assets/Torch.png"); // ID 6: Torch
    load(7, "assets/Coal.png"); // ID 7: Coal
    load(8, "assets/Copper.png"); // ID 8: Copper
    load(9, "assets/Iron.png"); // ID 9: Iron
    load(10, "assets/Cobalt.png"); // ID 10: Cobalt
    load(11, "assets/Tungsten.png"); // ID 11: Tungsten
    load(12, "assets/Bedrock.png"); // ID 12: Bedrock

    // --- HERRAMIENTAS ---
    load(21, "assets/PickaxeWood.png");
    load(22, "assets/PickaxeStone.png");
    load(23, "assets/PickaxeIron.png");
    load(24, "assets/Pickaxetungsten.png");
}

const sf::Texture* World::getTexture(int id) const {
    // Search for the texture with the given ID.
    auto it = mTextures.find(id);
    if (it != mTextures.end()) {
        // If it exists, return a pointer to it.
        return &it->second;
    }
    // Otherwise, return a null pointer.
    return nullptr;
}

void World::update(sf::Time dt, sf::Vector2f playerPos, std::map<int, int>& inventory) {

    // WE USE ITERATORS (auto it) INSTEAD OF INDICES (i)
    // This prevents "memory corruption" errors when deleting.
    for (auto it = mItems.begin(); it != mItems.end(); ) {

        ItemDrop& item = *it; // Access the current item
        item.timeAlive += dt.asSeconds();

        // --- PHYSICS CALCULATION ---
        float dx = playerPos.x - item.pos.x;
        float dy = playerPos.y - item.pos.y;
        float dist = std::sqrt(dx*dx + dy*dy);

        float magnetRange = 125.0f;
        float pickupRange = 30.0f;
        float magnetForce = 40.0f;

        // 1. MAGNET LOGIC
        if (dist < magnetRange && item.timeAlive > 0.5f) {
            float dirX = dx / dist;
            float dirY = dy / dist;
            item.vel.x += dirX * magnetForce;
            item.vel.y += dirY * magnetForce;
            item.vel *= 0.95f;
        }
        else {
            // 2. NORMAL GRAVITY
            item.vel.y += 800.0f * dt.asSeconds();
            item.vel.x *= 0.95f;
        }

        // Apply movement
        item.pos += item.vel * dt.asSeconds();

        // 3. GROUND COLLISION (If far away)
        if (dist >= magnetRange) {
            int gridX = static_cast<int>(item.pos.x / mTileSize);
            int gridY = static_cast<int>(item.pos.y / mTileSize);

            if (getBlock(gridX, gridY) != 0) {
                item.pos.y = gridY * mTileSize;
                item.vel.y = 0;
                item.vel.x *= 0.5f;
            }
        }

        // 4. PICKUP ZONE (HERE WAS THE ERROR: NOW THERE IS ONLY ONE BLOCK)
        if (dist < pickupRange && item.timeAlive > 0.2f) {

            // A) Save ID
            int id = item.id;

            // B) Add to inventory
            inventory[id]++;

            // C) Debug
            std::cout << "Picked up! You have: " << inventory[id] << " of block ID " << id << std::endl;

            // D) SAFELY DELETE
            // erase returns the iterator to the NEXT valid element.
            // So we don't lose the reference.
            it = mItems.erase(it);

        } else {
            // Only advance if we have NOT deleted anything
            ++it;
        }
    }
}

void World::saveToStream(std::ofstream& file) {
    // 1. Guardar cantidad de chunks
    size_t count = mChunks.size();
    file.write(reinterpret_cast<const char*>(&count), sizeof(count));

    // 2. Recorrer cada Chunk y guardarlo
    for (const auto& pair : mChunks) {
        int chunkX = pair.first;
        const std::vector<int>& blocks = pair.second;
        const std::vector<int>& walls = mBackgroundChunks[chunkX]; // No olvides las paredes

        // A) Guardar coordenada X del Chunk
        file.write(reinterpret_cast<const char*>(&chunkX), sizeof(chunkX));

        // B) Guardar los bloques (Vector completo de golpe)
        // size() * sizeof(int) calcula el tamaño en bytes de todo el vector
        file.write(reinterpret_cast<const char*>(blocks.data()), blocks.size() * sizeof(int));

        // C) Guardar las paredes
        file.write(reinterpret_cast<const char*>(walls.data()), walls.size() * sizeof(int));
    }
}

void World::loadFromStream(std::ifstream& file) {
    // 1. Limpiar el mundo actual (Importante para no mezclar mapas)
    mChunks.clear();
    mBackgroundChunks.clear();
    mItems.clear(); // Borramos los items del suelo también para evitar bugs

    // 2. Leer cuántos chunks hay guardados
    size_t count = 0;
    file.read(reinterpret_cast<char*>(&count), sizeof(count));

    // 3. Reconstruir cada Chunk
    for (size_t i = 0; i < count; ++i) {
        int chunkX = 0;

        // A) Leer coordenada X
        file.read(reinterpret_cast<char*>(&chunkX), sizeof(chunkX));

        // Preparar vectores vacíos del tamaño correcto
        std::vector<int> blocks(CHUNK_WIDTH * WORLD_HEIGHT);
        std::vector<int> walls(CHUNK_WIDTH * WORLD_HEIGHT);

        // B) Leer datos de Bloques
        file.read(reinterpret_cast<char*>(blocks.data()), blocks.size() * sizeof(int));

        // C) Leer datos de Paredes
        file.read(reinterpret_cast<char*>(walls.data()), walls.size() * sizeof(int));

        // Insertar en el mapa
        mChunks[chunkX] = blocks;
        mBackgroundChunks[chunkX] = walls;
    }
}