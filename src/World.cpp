#include "World.h"
#include <iostream>
#include <cmath>

/**
 * Constructor for the World class.
 * Initializes the tile size and loads block textures.
 */
World::World()
    : mTileSize(32.0f)
{
    // Load all block textures when the world is created.
    loadTextures();
}

// --- SMART GET BLOCK (INFINITE GENERATION) ---
/**
 * Retrieves the block ID at the specified global coordinates.
 * Generates the chunk if it doesn't exist.
 * @param x Global X coordinate (in blocks).
 * @param y Global Y coordinate (in blocks).
 * @return The block ID, or 0 (Air) if out of bounds.
 */

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

/**
 * Generates a new chunk at the specified chunk X coordinate.
 * Creates terrain, caves, ores, and trees using procedural generation.
 * @param chunkX The X coordinate of the chunk to generate.
 */
void World::generateChunk(int chunkX) {
    std::vector<int> newChunk;
    std::vector<int> newBgChunk;

    int totalBlocks = CHUNK_WIDTH * WORLD_HEIGHT;
    newChunk.resize(totalBlocks, 0);
    newBgChunk.resize(totalBlocks, 0);

    float seed = 97.0f;
    int surfaceHeights[CHUNK_WIDTH];

    // ---------------------------------------------------------
    // STEP 1: BASE TERRAIN (Stone, Dirt, and Air only)
    // ---------------------------------------------------------
    for (int localX = 0; localX < CHUNK_WIDTH; ++localX) {
        int globalX = (chunkX * CHUNK_WIDTH) + localX;

        // Smooth mountains
        float n1 = std::sin((globalX + seed) / 50.0f);
        float n2 = std::sin((globalX + seed) / 25.0f) * 0.5f;
        float baseHeight = 80.0f;

        int surfaceY = static_cast<int>(baseHeight + ((n1 + n2) * 15.0f));
        surfaceHeights[localX] = surfaceY;

        for (int y = 0; y < WORLD_HEIGHT; ++y) {
            int index = y * CHUNK_WIDTH + localX;
            int blockID = 0;
            int bgID = 0;

            // Background
            if (y > surfaceY) {
                if (y < surfaceY + 10) bgID = 1; // Dirt wall
                else bgID = 3;                   // Stone wall
            }

            // Foreground
            if (y >= WORLD_HEIGHT - 2) {
                blockID = 12; // Bedrock
            }
            else if (y >= surfaceY) {
                if (y == surfaceY) blockID = 2;       // Grass
                else if (y < surfaceY + 5) blockID = 1; // Dirt
                else blockID = 3;                     // Stone (NO ORES HERE YET)
            }

            newChunk[index] = blockID;
            newBgChunk[index] = bgID;
        }
    }

    // ---------------------------------------------------------
    // STEP 1.5: CAVES (Autómata Celular por Chunk)
    // ---------------------------------------------------------
    // 1. Añadimos ruido (agujeros aleatorios) solo bajo tierra
    for (int localX = 0; localX < CHUNK_WIDTH; ++localX) {
        int surfaceY = surfaceHeights[localX];

        // Empezamos 10 bloques por debajo de la superficie para no romper la hierba
        // y paramos 5 bloques antes del fondo para no romper la Bedrock
        for (int y = surfaceY + 10; y < WORLD_HEIGHT - 5; ++y) {
            int index = y * CHUNK_WIDTH + localX;

            // Solo rompemos la piedra
            if (newChunk[index] == 3) {
                if (rand() % 100 < 45) { // 45% de probabilidad de ser aire
                    newChunk[index] = 0; // Aire
                }
            }
        }
    }

    // 2. Suavizamos el ruido para crear túneles (4 pasadas)
    int smoothingPasses = 4;
    for (int p = 0; p < smoothingPasses; ++p) {
        std::vector<int> tempChunk = newChunk; // Mapa temporal para leer el estado anterior

        for (int localX = 0; localX < CHUNK_WIDTH; ++localX) {
            int surfaceY = surfaceHeights[localX];

            for (int y = surfaceY + 10; y < WORLD_HEIGHT - 5; ++y) {
                int index = y * CHUNK_WIDTH + localX;
                int neighborWalls = 0;

                // Contamos los muros vecinos en una cuadrícula de 3x3
                for (int nx = localX - 1; nx <= localX + 1; ++nx) {
                    for (int ny = y - 1; ny <= y + 1; ++ny) {
                        // Si está dentro de los límites de este Chunk
                        if (nx >= 0 && nx < CHUNK_WIDTH && ny >= 0 && ny < WORLD_HEIGHT) {
                            if (nx != localX || ny != y) { // No nos contamos a nosotros mismos
                                if (tempChunk[ny * CHUNK_WIDTH + nx] != 0) {
                                    neighborWalls++;
                                }
                            }
                        } else {
                            // Si mira fuera del Chunk, asumimos que hay piedra.
                            // Esto evitará que las cuevas se salgan del mapa por los lados rotos.
                            neighborWalls++;
                        }
                    }
                }

                // Aplicamos las reglas de supervivencia
                if (neighborWalls > 4) {
                    newChunk[index] = 3; // Se convierte/mantiene en Piedra
                } else if (neighborWalls < 4) {
                    newChunk[index] = 0; // Se deshace y se convierte en Aire
                }
            }
        }
    }

    // ---------------------------------------------------------
    // STEP 2: ORE GENERATOR (Veins)
    // ---------------------------------------------------------
    // Lambda function to "stamp" a vein
    auto spawnVein = [&](int count, int id, int minDepth, int maxDepth, int size) {
        for (int i = 0; i < count; ++i) {
            // 1. Choose a random center point within the chunk
            int cx = rand() % CHUNK_WIDTH;
            int cy = minDepth + (rand() % (maxDepth - minDepth));

            // 2. Create a "blob" shape around the center
            // We loop a small area around the center
            for (int vx = -1; vx <= 1; ++vx) {
                for (int vy = -1; vy <= 1; ++vy) {
                    // Probability to fill so it's not a perfect square
                    if ((rand() % 100) > size) continue;

                    int nx = cx + vx;
                    int ny = cy + vy;

                    // Check bounds
                    if (nx >= 0 && nx < CHUNK_WIDTH && ny >= 0 && ny < WORLD_HEIGHT) {
                        int index = ny * CHUNK_WIDTH + nx;

                        // ONLY replace STONE (ID 3).
                        // We don't break dirt, air, or other minerals.
                        if (newChunk[index] == 3) {
                            newChunk[index] = id;
                        }
                    }
                }
            }
        }
    };

    // --- ORE CONFIGURATION ---
    // (Number of attempts, ID, Min Depth, Max Depth, Fill Probability %)

    // COAL (ID 7): Very common, large veins, near surface
    spawnVein(6, 7, 20, 150, 80);

    // COPPER (ID 8): Common, medium veins
    spawnVein(4, 8, 30, 150, 70);

    // IRON (ID 9): Less common, medium veins
    spawnVein(3, 9, 50, 150, 70);

    // COBALT (ID 10): Rare, deep, small veins
    spawnVein(2, 10, 100, 150, 50);

    // TUNGSTEN (ID 11): Very rare, very deep, very small veins
    spawnVein(1, 11, 130, 150, 40);


    // ---------------------------------------------------------
    // STEP 3: TREES
    // ---------------------------------------------------------
    for (int localX = 0; localX < CHUNK_WIDTH; ++localX) {
        int globalX = (chunkX * CHUNK_WIDTH) + localX;
        int surfaceY = surfaceHeights[localX];

        if ((std::abs(globalX * 437) % 100) < 10 && localX > 1 && localX < CHUNK_WIDTH - 2) {
            int treeHeight = 4;
            int treeTopY = surfaceY - treeHeight;

            // Leaves
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
            // Trunk
            for (int i = 1; i <= treeHeight; ++i) {
                int trunkY = surfaceY - i;
                if (trunkY > 0) newChunk[trunkY * CHUNK_WIDTH + localX] = 4;
            }
        }
    }

    mChunks[chunkX] = newChunk;
    mBackgroundChunks[chunkX] = newBgChunk;
}

/**
 * Renders the visible part of the world.
 * Handles lighting, block rendering, background walls, and dropped items.
 * @param window The render window.
 * @param ambientColor The current ambient light color.
 */
void World::render(sf::RenderWindow& window, sf::Color ambientColor) {
    sf::View view = window.getView();
    float left = view.getCenter().x - (view.getSize().x / 2.f);
    float right = view.getCenter().x + (view.getSize().x / 2.f);
    float top = view.getCenter().y - (view.getSize().y / 2.f);
    float bottom = view.getCenter().y + (view.getSize().y / 2.f);

    int startChunk = static_cast<int>(std::floor(left / (CHUNK_WIDTH * mTileSize))) - 1;
    int endChunk = static_cast<int>(std::floor(right / (CHUNK_WIDTH * mTileSize))) + 1;

    // -----------------------------------------------------------
    // [NEW] STEP 0: ENSURE WORLD EXISTS
    // -----------------------------------------------------------
    // Before calculating lights or drawing, check if chunks are missing.
    // If missing, generate them RIGHT NOW.
    for (int cx = startChunk; cx <= endChunk; ++cx) {
        if (mChunks.find(cx) == mChunks.end()) {
            generateChunk(cx);
        }
    }

    // -----------------------------------------------------------
    // STEP 1: FIND ALL VISIBLE LIGHTS
    // -----------------------------------------------------------
    std::vector<sf::Vector2f> lightSources;

    for (int cx = startChunk; cx <= endChunk; ++cx) {
        auto it = mChunks.find(cx);
        if (it != mChunks.end()) {
            const auto& blocks = it->second; // Use iterator (Safer)
            for (int y = 0; y < WORLD_HEIGHT; ++y) {
                float blockY = y * mTileSize;
                if (blockY < top - 200 || blockY > bottom + 200) continue;

                for (int lx = 0; lx < CHUNK_WIDTH; ++lx) {
                    // ID 6 = Torch
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
    // SPRITE AND LIGHT PREPARATION
    // -----------------------------------------------------------
    sf::Sprite sprite;
    float lightRadius = 250.0f;

    // Lambda to calculate light
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

        if (maxTorchLight > 0.1f) b *= 0.8f; // Warm tint

        return sf::Color(
            static_cast<sf::Uint8>(r * 255),
            static_cast<sf::Uint8>(g * 255),
            static_cast<sf::Uint8>(b * 255)
        );
    };

    // -----------------------------------------------------------
    // LOOP 1: WALLS (BACKGROUND) - [FIXED TO AVOID CRASH]
    // -----------------------------------------------------------
    for (int cx = startChunk; cx <= endChunk; ++cx) {

        // CORRECTION: We use find() to get the pointer to the vector safely
        auto itBg = mBackgroundChunks.find(cx);

        // If the background chunk doesn't exist or is empty, skip
        if (itBg == mBackgroundChunks.end() || itBg->second.empty()) continue;

        const auto& bgBlocks = itBg->second; // Safe reference

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
    // LOOP 2: FOREGROUND (BLOCKS)
    // -----------------------------------------------------------
    for (int cx = startChunk; cx <= endChunk; ++cx) {
        auto it = mChunks.find(cx);
        if (it == mChunks.end()) continue; // Extra safety

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

                    if (blockID == 6) { // Torch always shines
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
    // --- DRAW ITEMS ON THE GROUND ---
    for (const auto& item : mItems) {
        const sf::Texture* tex = getTexture(item.id);
        if (tex) {
            sf::Sprite sprite(*tex);
            sprite.setPosition(item.pos);

            // Set origin to center for nice rotation
            sprite.setOrigin(tex->getSize().x / 2.0f, tex->getSize().y / 2.0f);

            // Other blocks (dirt, wood, etc) at 0.5
            sprite.setScale(0.5f, 0.5f);

            sprite.setColor(calculateLight(item.pos, ambientColor));
            window.draw(sprite);
        }
    }
}

/**
 * Spawns an item drop at the specified coordinates.
 * @param x Global X coordinate (in blocks).
 * @param y Global Y coordinate (in blocks).
 * @param id The ID of the item to spawn.
 */
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
/**
 * Sets a block at the specified coordinates.
 * Handles chunk generation if necessary and spawns drops when breaking blocks.
 * @param x Global X coordinate (in blocks).
 * @param y Global Y coordinate (in blocks).
 * @param type The new block ID.
 */
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

/**
 * Loads all block textures from files.
 * Uses a helper lambda to handle errors and create fallback textures.
 */
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
    load(25, "assets/Door.png"); // ID 25: Door

    // --- TOOLS ---
    load(21, "assets/PickaxeWood.png");
    load(22, "assets/PickaxeStone.png");
    load(23, "assets/PickaxeIron.png");
    load(24, "assets/Pickaxetungsten.png");

    // --- MEAT ---
    load(30, "assets/Meat.png");
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

/**
 * Updates the world state.
 * Handles item physics (gravity, magnetism) and pickup logic.
 * @param dt Delta time (time elapsed since last frame).
 * @param playerPos The player's current position.
 * @param inventory Reference to the player's inventory map.
 */
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

/**
 * Saves the world state to a file stream.
 * Serializes chunks (blocks and background walls).
 * @param file The output file stream.
 */
void World::saveToStream(std::ofstream& file) {
    // 1. Save chunk count
    size_t count = mChunks.size();
    file.write(reinterpret_cast<const char*>(&count), sizeof(count));

    // 2. Iterate through each Chunk and save it
    for (const auto& pair : mChunks) {
        int chunkX = pair.first;
        const std::vector<int>& blocks = pair.second;
        const std::vector<int>& walls = mBackgroundChunks[chunkX]; // Don't forget walls

        // A) Save Chunk X coordinate
        file.write(reinterpret_cast<const char*>(&chunkX), sizeof(chunkX));

        // B) Save blocks (Full vector at once)
        // size() * sizeof(int) calculates the size in bytes of the entire vector
        file.write(reinterpret_cast<const char*>(blocks.data()), blocks.size() * sizeof(int));

        // C) Save walls
        file.write(reinterpret_cast<const char*>(walls.data()), walls.size() * sizeof(int));
    }
}

/**
 * Loads the world state from a file stream.
 * Deserializes chunks (blocks and background walls).
 * @param file The input file stream.
 */
void World::loadFromStream(std::ifstream& file) {
    // 1. Clear current world (Important to not mix maps)
    mChunks.clear();
    mBackgroundChunks.clear();
    mItems.clear(); // Clear ground items too to avoid bugs

    // 2. Read how many chunks are saved
    size_t count = 0;
    file.read(reinterpret_cast<char*>(&count), sizeof(count));

    // 3. Reconstruct each Chunk
    for (size_t i = 0; i < count; ++i) {
        int chunkX = 0;

        // A) Read X coordinate
        file.read(reinterpret_cast<char*>(&chunkX), sizeof(chunkX));

        // Prepare empty vectors of correct size
        std::vector<int> blocks(CHUNK_WIDTH * WORLD_HEIGHT);
        std::vector<int> walls(CHUNK_WIDTH * WORLD_HEIGHT);

        // B) Read Block data
        file.read(reinterpret_cast<char*>(blocks.data()), blocks.size() * sizeof(int));

        // C) Read Wall data
        file.read(reinterpret_cast<char*>(walls.data()), walls.size() * sizeof(int));

        // Insert into map
        mChunks[chunkX] = blocks;
        mBackgroundChunks[chunkX] = walls;
    }
}

/**
 * Spawns an item drop at a specific position (e.g., from a mob).
 * @param id The ID of the item to spawn.
 * @param pos The position to spawn the item at.
 */
void World::spawnItem(int id, sf::Vector2f pos) {
    ItemDrop item;
    item.id = id;
    item.pos = pos;

    // Give it a small "jump" upwards so the drop feels satisfying
    item.vel = sf::Vector2f((rand() % 100 - 50), -150.0f);
    item.timeAlive = 0.0f;

    mItems.push_back(item);
}