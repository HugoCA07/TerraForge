#include "World.h"
#include <iostream>
#include <cmath>
#include <algorithm>
#include "Game.h"

/**
 * @brief Constructor for the World class.
 * Initializes the tile size and triggers the loading of all block and item textures.
 */
World::World()
    : mTileSize(32.0f)
{
    loadTextures();
}

// ==========================================
// DYNAMIC CHUNK GENERATION
// ==========================================

/**
 * @brief Retrieves the block ID at the specified global grid coordinates.
 * If the requested chunk is not currently in memory, it is generated on the fly.
 * @param x Global X coordinate (in blocks).
 * @param y Global Y coordinate (in blocks).
 * @return The ItemID of the block, or 0 (Air) if out of vertical bounds.
 */
int World::getBlock(int x, int y) {
    // Boundary Check: Ensure Y is within the world height limits
    if (y < 0 || y >= WORLD_HEIGHT) {
        return 0; // Air outside vertical limits
    }

    // Calculate Chunk Index
    // std::floor ensures negative coordinates are grouped correctly (e.g., x=-5 belongs to chunk -1)
    int chunkIndex = static_cast<int>(std::floor(x / (float)CHUNK_WIDTH));

    // Check memory: generate the chunk if it doesn't exist yet
    if (mChunks.find(chunkIndex) == mChunks.end()) {
        generateChunk(chunkIndex);
    }

    // Retrieve the specific block from the chunk's 1D vector array.
    // The modulo operator wraps negative local coordinates into the 0-15 range safely.
    int localX = (x % CHUNK_WIDTH + CHUNK_WIDTH) % CHUNK_WIDTH;
    int index = y * CHUNK_WIDTH + localX;

    return mChunks[chunkIndex][index];
}

/**
 * @brief Procedurally generates a new chunk of terrain.
 * Uses a combination of Perlin-style noise and cellular automata to carve out biomes,
 * caves, ore veins, and surface decorations (trees).
 * @param chunkX The chunk coordinate (X index) to generate.
 */
void World::generateChunk(int chunkX) {
    std::vector<int> newChunk;
    std::vector<int> newBgChunk;

    int totalBlocks = CHUNK_WIDTH * WORLD_HEIGHT;
    newChunk.resize(totalBlocks, 0);
    newBgChunk.resize(totalBlocks, 0);

    float seed = 97.0f; // Static seed for testing, make variable for random worlds
    int surfaceHeights[CHUNK_WIDTH];

    // ---------------------------------------------------------
    // STEP 1: BASE TERRAIN & BIOMES (Deserts and Snow)
    // ---------------------------------------------------------
    for (int localX = 0; localX < CHUNK_WIDTH; ++localX) {
        int globalX = (chunkX * CHUNK_WIDTH) + localX;

        // 1. CALCULATE SURFACE ELEVATION (Rolling hills)
        float n1 = std::sin((globalX + seed) / 50.0f);
        float n2 = std::sin((globalX + seed) / 25.0f) * 0.5f;
        float baseHeight = 80.0f;
        int surfaceY = static_cast<int>(baseHeight + ((n1 + n2) * 15.0f));
        surfaceHeights[localX] = surfaceY;

        // 2. CALCULATE BIOME "MOISTURE" (Temperature zones)
        float biomeValue = std::sin((globalX - 100) / 500.0f);

        bool isDesert = (biomeValue > 0.5f);
        bool isSnow = (biomeValue < -0.5f);

        // Calculate specific depth and shape properties for special biomes
        float biomeDepth = 0.0f;

        if (isDesert) {
            // Intensity from 0.0 (edge) to 1.0 (center)
            float intensity = (biomeValue - 0.5f) * 2.0f;
            // "Bag" shape: Wide and deep, drops sharply (exponent 0.5)
            biomeDepth = 45.0f * std::pow(intensity, 0.5f);
        }
        else if (isSnow) {
            float intensity = (std::abs(biomeValue) - 0.5f) * 2.0f;
            // "V" / "Diamond" shape: Drops down in a straight, pointy angle (exponent 1.2)
            biomeDepth = 45.0f * std::pow(intensity, 1.2f);
        }

        // Fill the vertical column for this X coordinate
        for (int y = 0; y < WORLD_HEIGHT; ++y) {
            int index = y * CHUNK_WIDTH + localX;
            int blockID = 0;
            int bgID = 0;

            // Background Walls (Generated based on depth)
            if (y > surfaceY) {
                if (y < surfaceY + 10) bgID = ItemID::BG_DIRT; // Dirt wall layer
                else bgID = ItemID::BG_STONE;                  // Deep Stone wall layer
            }

            // Foreground Solid Blocks
            if (y >= WORLD_HEIGHT - 2) {
                blockID = ItemID::BEDROCK; // Unbreakable Bedrock bottom layer
            }
            else if (y >= surfaceY) {
                int depthFromSurface = y - surfaceY;

                // Apply biome overlays overrides based on the calculated depth curve
                if (isDesert && depthFromSurface <= biomeDepth) {
                    blockID = ItemID::SAND; // Sand block filling the desert pocket
                }
                else if (isSnow && depthFromSurface <= biomeDepth) {
                    blockID = ItemID::SNOW; // Snow block filling the tundra pocket
                }
                else {
                    // Standard geology generation
                    if (depthFromSurface < 5) blockID = ItemID::DIRT;   // Dirt layer
                    else blockID = ItemID::STONE;                       // Deep Stone layer
                }
            }

            newChunk[index] = blockID;
            newBgChunk[index] = bgID;
        }
    }

    // ---------------------------------------------------------
    // STEP 1.5: CAVE SYSTEMS (Worm tunnels + Isolated pockets)
    // ---------------------------------------------------------
    // 1. Carve raw noise
    for (int localX = 0; localX < CHUNK_WIDTH; ++localX) {
        int globalX = (chunkX * CHUNK_WIDTH) + localX;
        int surfaceY = surfaceHeights[localX];

        // Start carving 10 blocks beneath the surface to prevent floating islands
        for (int y = surfaceY + 10; y < WORLD_HEIGHT - 5; ++y) {
            int index = y * CHUNK_WIDTH + localX;

            // Only carve through stone
            if (newChunk[index] == ItemID::STONE) {
                // A) SPAGHETTI CAVES (Interconnected tunnels)
                // Combine 3 different sine waves for a chaotic but connected pattern
                float n1 = std::sin((globalX + seed) / 20.0f);
                float n2 = std::cos((y + seed) / 15.0f);
                float n3 = std::sin((globalX - y) / 35.0f);
                float caveNoise = n1 + n2 + n3;

                // Create a tunnel if the wave values cancel out close to zero
                bool isWormCave = std::abs(caveNoise) < 0.4f;

                // B) POCKET CAVES (Isolated spherical rooms)
                bool isPocketCave = (rand() % 100 < 40);

                if (isWormCave || isPocketCave) {
                    newChunk[index] = 0; // Replace stone with air
                }
            }
        }
    }

    // 2. Cellular Automata Smoothing (Smooths out jagged edges and floating blocks)
    int smoothingPasses = 4;
    for (int p = 0; p < smoothingPasses; ++p) {
        std::vector<int> tempChunk = newChunk; // Read from previous state buffer

        for (int localX = 0; localX < CHUNK_WIDTH; ++localX) {
            int surfaceY = surfaceHeights[localX];

            for (int y = surfaceY + 10; y < WORLD_HEIGHT - 5; ++y) {
                int index = y * CHUNK_WIDTH + localX;

                // BIOME SHIELD: Do not smooth Sand or Snow
                if (tempChunk[index] == ItemID::SAND || tempChunk[index] == ItemID::SNOW) {
                    continue;
                }

                int neighborWalls = 0;

                // Count solid neighbor blocks within a 3x3 grid
                for (int nx = localX - 1; nx <= localX + 1; ++nx) {
                    for (int ny = y - 1; ny <= y + 1; ++ny) {
                        if (nx >= 0 && nx < CHUNK_WIDTH && ny >= 0 && ny < WORLD_HEIGHT) {
                            if (nx != localX || ny != y) {
                                if (tempChunk[ny * CHUNK_WIDTH + nx] != 0) {
                                    neighborWalls++;
                                }
                            }
                        } else {
                            // Assume edges outside chunk are solid to prevent infinite bleeding
                            neighborWalls++;
                        }
                    }
                }

                // Survival rules for smoothing
                if (neighborWalls > 4) {
                    newChunk[index] = ItemID::STONE; // Become/Stay solid stone
                } else if (neighborWalls < 4) {
                    newChunk[index] = ItemID::AIR; // Become/Stay air
                }
            }
        }
    }

    // ---------------------------------------------------------
    // STEP 2: ORE VEIN GENERATION
    // ---------------------------------------------------------
    // Lambda function to "stamp" a circular vein blob into the chunk
    auto spawnVein = [&](int count, int id, int minDepth, int maxDepth, int sizeProbability) {
        for (int i = 0; i < count; ++i) {
            // Choose a random center coordinate within the chunk depth bounds
            int cx = rand() % CHUNK_WIDTH;
            int cy = minDepth + (rand() % (maxDepth - minDepth));

            // Populate a 3x3 area around the center point based on probability
            for (int vx = -1; vx <= 1; ++vx) {
                for (int vy = -1; vy <= 1; ++vy) {
                    if ((rand() % 100) > sizeProbability) continue;

                    int nx = cx + vx;
                    int ny = cy + vy;

                    if (nx >= 0 && nx < CHUNK_WIDTH && ny >= 0 && ny < WORLD_HEIGHT) {
                        int index = ny * CHUNK_WIDTH + nx;
                        // ONLY overwrite Stone (Do not destroy caves or dirt)
                        if (newChunk[index] == ItemID::STONE) {
                            newChunk[index] = id;
                        }
                    }
                }
            }
        }
    };

    // --- ORE CONFIGURATION (Attempts, ID, Min Depth, Max Depth, Spread % ) ---
    spawnVein(6, ItemID::COAL, 20, 150, 80);
    spawnVein(4, ItemID::COPPER, 30, 150, 70);
    spawnVein(3, ItemID::IRON, 50, 150, 70);
    spawnVein(2, ItemID::COBALT, 100, 150, 50);
    spawnVein(1, ItemID::TUNGSTEN, 130, 150, 40);

    // ---------------------------------------------------------
    // STEP 3: SURFACE DECORATION (Trees)
    // ---------------------------------------------------------
    for (int localX = 0; localX < CHUNK_WIDTH; ++localX) {
        int globalX = (chunkX * CHUNK_WIDTH) + localX;
        int surfaceY = surfaceHeights[localX];

        int surfaceBlockID = newChunk[surfaceY * CHUNK_WIDTH + localX];

        // Trees only grow on DIRT (ID 1)
        if (surfaceBlockID == ItemID::DIRT && (std::abs(globalX * 437) % 100) < 10 && localX > 2 && localX < CHUNK_WIDTH - 3) {
            int treeHeight = 5 + (rand() % 11);
            int treeTopY = surfaceY - treeHeight;

            // Generate Leaves (Canopy)
            for (int lx = -2; lx <= 2; ++lx) {
                for (int ly = -2; ly <= 1; ++ly) {
                    // Shave corners to create an oval/rounded shape instead of a square
                    if (std::abs(lx) == 2 && std::abs(ly) == 2) continue;

                    int leafY = treeTopY + ly;
                    int leafX = localX + lx;
                    if (leafY > 0 && leafY < WORLD_HEIGHT) {
                        int idx = leafY * CHUNK_WIDTH + leafX;
                        // Don't overwrite existing blocks with leaves
                        if (newChunk[idx] == ItemID::AIR) newChunk[idx] = ItemID::LEAVES;
                    }
                }
            }
            // Generate Trunk (Wood)
            for (int i = 1; i <= treeHeight; ++i) {
                int trunkY = surfaceY - i;
                if (trunkY > 0) newChunk[trunkY * CHUNK_WIDTH + localX] = ItemID::WOOD;
            }
        }
    }

    // Save generated arrays into the chunk maps
    mChunks[chunkX] = newChunk;
    mBackgroundChunks[chunkX] = newBgChunk;
}

// ==========================================
// RENDERING
// ==========================================

/**
 * @brief Renders the visible sections of the world (Culling).
 * Processes dynamic lighting (torches + ambient depth) and draws
 * background walls, foreground blocks, and dropped items.
 * @param window The render window.
 * @param ambientColor The global daylight/nightlight color.
 */
void World::render(sf::RenderWindow& window, sf::Color ambientColor) {
    sf::View view = window.getView();

    // Calculate visible area boundaries
    float left = view.getCenter().x - (view.getSize().x / 2.f);
    float right = view.getCenter().x + (view.getSize().x / 2.f);
    float top = view.getCenter().y - (view.getSize().y / 2.f);
    float bottom = view.getCenter().y + (view.getSize().y / 2.f);

    // Calculate which chunks are visible on screen
    int startChunk = static_cast<int>(std::floor(left / (CHUNK_WIDTH * mTileSize))) - 1;
    int endChunk = static_cast<int>(std::floor(right / (CHUNK_WIDTH * mTileSize))) + 1;

    // STEP 0: ENSURE VISIBLE CHUNKS EXIST
    for (int cx = startChunk; cx <= endChunk; ++cx) {
        if (mChunks.find(cx) == mChunks.end()) generateChunk(cx);
    }

    // STEP 1: SCAN FOR LIGHT SOURCES (Torches)
    std::vector<sf::Vector2f> lightSources;
    for (int cx = startChunk; cx <= endChunk; ++cx) {
        auto it = mChunks.find(cx);
        if (it != mChunks.end()) {
            const auto& blocks = it->second;
            for (int y = 0; y < WORLD_HEIGHT; ++y) {
                float blockY = y * mTileSize;
                // Minor vertical culling for light checks
                if (blockY < top - 200 || blockY > bottom + 200) continue;

                for (int lx = 0; lx < CHUNK_WIDTH; ++lx) {
                    if (blocks[y * CHUNK_WIDTH + lx] == ItemID::TORCH) { // ¡Cambiado ID 6 a TORCH!
                        float wx = (cx * CHUNK_WIDTH + lx) * mTileSize + mTileSize/2.f;
                        float wy = y * mTileSize + mTileSize/2.f;
                        lightSources.push_back(sf::Vector2f(wx, wy));
                    }
                }
            }
        }
    }

    sf::Sprite sprite;
    float lightRadius = 250.0f;

    // Dynamic Lighting Calculation Lambda
    auto calculateLight = [&](sf::Vector2f blockPos, sf::Color baseColor) -> sf::Color {
        float r = baseColor.r;
        float g = baseColor.g;
        float b = baseColor.b;

        float torchR = 0.0f, torchG = 0.0f, torchB = 0.0f;

        for (const auto& lightPos : lightSources) {
            float dx = std::abs(blockPos.x - lightPos.x);
            float dy = std::abs(blockPos.y - lightPos.y);

            // Optimization: Skip distance calc if strictly outside AABB bounds
            if (dx > lightRadius || dy > lightRadius) continue;

            float dist = std::sqrt(dx*dx + dy*dy);

            if (dist < lightRadius) {
                // Linear intensity falloff
                float intensity = 1.0f - (dist / lightRadius);

                // Additive torch light (Warm fire colors: Max Red, High Green, Low Blue)
                torchR = std::max(torchR, intensity * 255.0f);
                torchG = std::max(torchG, intensity * 200.0f);
                torchB = std::max(torchB, intensity * 120.0f);
            }
        }

        // Screen blending: Choose the brightest value between ambient daylight and torchlight
        r = std::max(r, torchR);
        g = std::max(g, torchG);
        b = std::max(b, torchB);

        // Clamp to prevent visual overflow
        return sf::Color(static_cast<sf::Uint8>(std::min(r, 255.0f)),
                         static_cast<sf::Uint8>(std::min(g, 255.0f)),
                         static_cast<sf::Uint8>(std::min(b, 255.0f)));
    };

    // STEP 2: DRAW BACKGROUND WALLS
    for (int cx = startChunk; cx <= endChunk; ++cx) {
        auto itBg = mBackgroundChunks.find(cx);
        if (itBg == mBackgroundChunks.end() || itBg->second.empty()) continue;

        const auto& bgBlocks = itBg->second;

        for (int y = 0; y < WORLD_HEIGHT; ++y) {
            float py = y * mTileSize;
            if (py < top || py > bottom) continue; // Vertical culling

            for (int lx = 0; lx < CHUNK_WIDTH; ++lx) {
                int blockID = bgBlocks[y * CHUNK_WIDTH + lx];
                if (blockID != 0 && mTextures.count(blockID)) {
                    sprite.setTexture(mTextures[blockID]);

                    float px = (cx * CHUNK_WIDTH + lx) * mTileSize;
                    sprite.setPosition(px, py);

                    float scale = mTileSize / sprite.getLocalBounds().width;
                    sprite.setScale(scale, scale);

                    sf::Vector2f center(px + mTileSize/2, py + mTileSize/2);
                    sf::Color lightColor = calculateLight(center, ambientColor);

                    // Depth trick: Darken background walls by 50% so they visually sit "behind"
                    lightColor.r = static_cast<sf::Uint8>(lightColor.r * 0.5f);
                    lightColor.g = static_cast<sf::Uint8>(lightColor.g * 0.5f);
                    lightColor.b = static_cast<sf::Uint8>(lightColor.b * 0.5f);

                    sprite.setColor(lightColor);
                    window.draw(sprite);
                }
            }
        }
    }

    // STEP 3: DRAW FOREGROUND BLOCKS
    for (int cx = startChunk; cx <= endChunk; ++cx) {
        auto it = mChunks.find(cx);
        if (it == mChunks.end()) continue;

        const auto& blocks = it->second;

        for (int y = 0; y < WORLD_HEIGHT; ++y) {
             float py = y * mTileSize;
             if (py < top || py > bottom) continue;

            for (int lx = 0; lx < CHUNK_WIDTH; ++lx) {
                int blockID = blocks[y * CHUNK_WIDTH + lx];

                if (blockID != 0) {
                    float px = (cx * CHUNK_WIDTH + lx) * mTileSize;
                    sf::Vector2f center(px + mTileSize/2, py + mTileSize/2);

                    // ==========================================
                    // --- DIBUJADO DE AUTOTILING (TIERRA, PIEDRA, ETC) ---
                    // ==========================================
                    if (mAutotileTextures.count(blockID)) {
                        sf::Sprite autoSprite(mAutotileTextures[blockID]);

                        // Calculamos la X global real en el mundo
                        int globalX = cx * CHUNK_WIDTH + lx;

                        // Pasamos el propio blockID como target
                        int mask = getBitmask(globalX, y, blockID);

                        // Calculamos Fila y Columna
                        int col = mask % 4;
                        int row = mask / 4;

                        // Recortamos la textura y posicionamos
                        autoSprite.setTextureRect(sf::IntRect(col * mTileSize, row * mTileSize, mTileSize, mTileSize));
                        autoSprite.setPosition(px, py);

                        // Iluminación dinámica
                        autoSprite.setColor(calculateLight(center, ambientColor));

                        window.draw(autoSprite);
                    }
                    // ==========================================
                    // --- DIBUJADO NORMAL (MADERA, MINERALES, ETC) ---
                    // ==========================================
                    else if (mTextures.count(blockID)) {
                        sprite.setTexture(mTextures[blockID]);
                        sprite.setPosition(px, py);

                        float scale = mTileSize / sprite.getLocalBounds().width;
                        sprite.setScale(scale, scale);

                        if (blockID == ItemID::TORCH) { // Torches always render at max brightness
                            sprite.setColor(sf::Color::White);
                        } else {
                            sprite.setColor(calculateLight(center, ambientColor));
                        }

                        window.draw(sprite);
                    }
                }
            }
        }
    }

    // STEP 4: DRAW DROPPED ITEMS
    for (const auto& item : mItems) {
        const sf::Texture* tex = getTexture(item.id);
        if (tex) {
            sf::Sprite itemSprite(*tex);
            itemSprite.setPosition(item.pos);

            // Center origin for correct placement and potential rotation
            itemSprite.setOrigin(tex->getSize().x / 2.0f, tex->getSize().y / 2.0f);

            // Scale down standard blocks on the ground
            itemSprite.setScale(0.5f, 0.5f);

            itemSprite.setColor(calculateLight(item.pos, ambientColor));
            window.draw(itemSprite);
        }
    }
}

// ==========================================
// WORLD INTERACTION (Mining & Placing)
// ==========================================

/**
 * @brief Overwrites a block at the specified global coordinates.
 * Called when the player mines or places a block.
 */
void World::setBlock(int x, int y, int type) {
    if (y < 0 || y >= WORLD_HEIGHT) return;

    int chunkIndex = static_cast<int>(std::floor(x / (float)CHUNK_WIDTH));
    int localX = (x % CHUNK_WIDTH + CHUNK_WIDTH) % CHUNK_WIDTH;

    if (mChunks.find(chunkIndex) == mChunks.end()) {
        generateChunk(chunkIndex);
    }

    mChunks[chunkIndex][y * CHUNK_WIDTH + localX] = type;
}

// ==========================================
// DROPPED ITEMS (Loot)
// ==========================================

/**
 * @brief Spawns an item exactly at the center of a grid tile (used for mining).
 */
void World::spawnItem(int x, int y, int id) {
    ItemDrop item;
    item.id = id;

    // Centered position
    item.pos = sf::Vector2f(x * mTileSize + mTileSize/2, y * mTileSize + mTileSize/2);

    // Initial "Pop" velocity (Random slight horizontal, fixed upward jump)
    float randX = (rand() % 100 - 50) * 2.0f;
    float randY = -150.0f;

    item.vel = sf::Vector2f(randX, randY);
    item.timeAlive = 0.0f;

    mItems.push_back(item);
}

/**
 * @brief Spawns an item at an absolute world position (used for dead mobs).
 */
void World::spawnItem(int id, sf::Vector2f pos) {
    ItemDrop item;
    item.id = id;
    item.pos = pos;
    item.vel = sf::Vector2f((rand() % 100 - 50), -150.0f);
    item.timeAlive = 0.0f;

    mItems.push_back(item);
}

/**
 * @brief Updates physics (gravity) and magnetic attraction for dropped items.
 * @param dt Time elapsed.
 * @param playerPos Player's center coordinates for distance checks.
 * @param inventory A map passed by reference to add picked-up items to.
 */
void World::update(sf::Time dt, sf::Vector2f playerPos, std::map<int, int>& inventory) {
    // Iterate manually to safely delete items that are picked up
    for (auto it = mItems.begin(); it != mItems.end(); ) {
        ItemDrop& item = *it;
        item.timeAlive += dt.asSeconds();

        float dx = playerPos.x - item.pos.x;
        float dy = playerPos.y - item.pos.y;
        float dist = std::sqrt(dx*dx + dy*dy);

        float magnetRange = 125.0f;
        float pickupRange = 30.0f;
        float magnetForce = 40.0f;

        // 1. Magnetic Attraction towards player
        if (dist < magnetRange && item.timeAlive > 0.5f) { // 0.5s delay before magnetism activates
            float dirX = dx / dist;
            float dirY = dy / dist;
            item.vel.x += dirX * magnetForce;
            item.vel.y += dirY * magnetForce;
            item.vel *= 0.95f; // Dampen velocity to prevent orbiting
        }
        else {
            // 2. Normal Gravity Fall
            item.vel.y += 800.0f * dt.asSeconds();
            item.vel.x *= 0.95f; // Horizontal air friction
        }

        item.pos += item.vel * dt.asSeconds();

        // 3. Ground Collision (Only if not being magnetized)
        if (dist >= magnetRange) {
            int gridX = static_cast<int>(item.pos.x / mTileSize);
            int gridY = static_cast<int>(item.pos.y / mTileSize);

            // If inside a solid block, snap to grid top and stop falling
            if (getBlock(gridX, gridY) != 0) {
                item.pos.y = gridY * mTileSize;
                item.vel.y = 0;
                item.vel.x *= 0.5f; // Hard friction against ground
            }
        }

        // 4. Pickup Event
        if (dist < pickupRange && item.timeAlive > 0.2f) {
            inventory[item.id]++; // Add to temporary pickup map
            std::cout << "Picked up! ID: " << item.id << std::endl;

            // Erase from world and keep iterator valid
            it = mItems.erase(it);
        } else {
            ++it;
        }
    }
}

// ==========================================
// ASSET MANAGEMENT
// ==========================================

const sf::Texture* World::getTexture(int id) const {
    auto it = mTextures.find(id);
    if (it != mTextures.end()) return &it->second;
    return nullptr;
}

const sf::Texture* World::getHeldTexture(int id) const {
    auto it = mHeldTextures.find(id);
    if (it != mHeldTextures.end()) return &it->second;
    return nullptr;
}

const sf::Texture* World::getArmorAnimTexture(int id) const {
    auto it = mArmorAnimTextures.find(id);
    if (it != mArmorAnimTextures.end()) return &it->second;
    return nullptr;
}

/**
 * @brief Bulk loads all textures from disk into memory.
 */
void World::loadTextures() {
    auto load = [&](int id, const std::string& filename) {
        sf::Texture tex;
        if (!tex.loadFromFile(filename)) {
            std::cerr << "Error loading: " << filename << std::endl;
            sf::Image img;
            img.create(32, 32, sf::Color::Magenta);
            tex.loadFromImage(img);
        }
        mTextures[id] = tex;
    };

    auto loadHeld = [&](int id, const std::string& filename) {
        sf::Texture tex;
        if (tex.loadFromFile(filename)) mHeldTextures[id] = tex;
        else std::cerr << "Error loading held sprite: " << filename << std::endl;
    };

    auto loadArmorAnim = [&](int id, const std::string& filename) {
        sf::Texture tex;
        if (tex.loadFromFile(filename)) mArmorAnimTextures[id] = tex;
        else std::cerr << "Error loading armor anim: " << filename << std::endl;
    };

    // --- Load Held Tools ---
    loadHeld(ItemID::WOOD_PICKAXE, "assets/Pickaxewood_hands.png");
    loadHeld(ItemID::STONE_PICKAXE, "assets/Pickaxestone_hands.png");
    loadHeld(ItemID::IRON_PICKAXE, "assets/Pickaxeiron_hands.png");
    loadHeld(ItemID::TUNGSTEN_PICKAXE, "assets/Pickaxetungsten_hands.png");

    loadHeld(ItemID::WOOD_SWORD, "assets/Swordwood_hands.png");
    loadHeld(ItemID::STONE_SWORD, "assets/Swordstone_hands.png");
    loadHeld(ItemID::IRON_SWORD, "assets/Swordiron_hands.png");
    loadHeld(ItemID::TUNGSTEN_SWORD, "assets/Swordtungsten_hands.png");
    loadHeld(ItemID::BOW, "assets/Bow_hands.png");

    // --- Load Blocks & Items ---
    load(ItemID::DIRT, "assets/Dirt.png");
    load(ItemID::STONE, "assets/Stone.png");
    load(ItemID::WOOD, "assets/Tree.png");
    load(ItemID::LEAVES, "assets/Leaves.png");
    load(ItemID::TORCH, "assets/Torch.png");
    load(ItemID::SAND, "assets/Sand.png");
    load(ItemID::SNOW, "assets/Snow.png");
    load(ItemID::BEDROCK, "assets/Bedrock.png");

    // --- Background Walls ---
    load(ItemID::BG_DIRT, "assets/Dirt.png");
    load(ItemID::BG_STONE, "assets/Stone.png");

    // Ores
    load(ItemID::COAL, "assets/Coal.png");
    load(ItemID::COPPER, "assets/Copper.png");
    load(ItemID::IRON, "assets/Iron.png");
    load(ItemID::COBALT, "assets/Cobalt.png");
    load(ItemID::TUNGSTEN, "assets/Tungsten.png");

    // Doors
    load(ItemID::DOOR, "assets/DoorBottomClosed.png");
    load(ItemID::DOOR_MID, "assets/DoorMidClosed.png");
    load(ItemID::DOOR_TOP, "assets/DoorTopClosed.png");
    load(ItemID::DOOR_OPEN, "assets/DoorBottomOpen.png");
    load(ItemID::DOOR_OPEN_MID, "assets/DoorMidOpen.png");
    load(ItemID::DOOR_OPEN_TOP, "assets/DoorTopOpen.png");

    // Utilities
    load(ItemID::CRAFTING_TABLE, "assets/CraftingTable.png");
    load(ItemID::FURNACE, "assets/Furnace.png");
    load(ItemID::CHEST, "assets/Chest.png");

    // Inventory Tools
    load(ItemID::WOOD_PICKAXE, "assets/PickaxeWood.png");
    load(ItemID::STONE_PICKAXE, "assets/PickaxeStone.png");
    load(ItemID::IRON_PICKAXE, "assets/PickaxeIron.png");
    load(ItemID::TUNGSTEN_PICKAXE, "assets/Pickaxetungsten.png");

    load(ItemID::WOOD_SWORD, "assets/SwordWood.png");
    load(ItemID::STONE_SWORD, "assets/SwordStone.png");
    load(ItemID::IRON_SWORD, "assets/SwordIron.png");
    load(ItemID::TUNGSTEN_SWORD, "assets/SwordTungsten.png");
    load(ItemID::BOW, "assets/Bow.png");
    load(ItemID::ARROW, "assets/Arrow.png");

    // Consumables & Materials
    load(ItemID::MEAT, "assets/Meat.png");
    load(ItemID::MEAT_MEDALLION, "assets/MeatMedallion.png");
    load(ItemID::IRON_INGOT, "assets/IronIngot.png");
    load(ItemID::COPPER_INGOT, "assets/CopperIngot.png");
    load(ItemID::COBALT_INGOT, "assets/CobaltIngot.png");
    load(ItemID::TUNGSTEN_INGOT, "assets/TungstenIngot.png");

    // Inventory Armor
    load(ItemID::WOOD_HELMET, "assets/WoodHelmet.png");
    load(ItemID::WOOD_CHEST, "assets/WoodChest.png");
    load(ItemID::WOOD_LEGS, "assets/WoodLegs.png");
    load(ItemID::WOOD_BOOTS, "assets/WoodBoots.png");

    // --- Load Animated Armor ---
    loadArmorAnim(ItemID::WOOD_HELMET, "assets/WoodHelmet_Anim.png");
    loadArmorAnim(ItemID::WOOD_CHEST, "assets/WoodChest_Anim.png");
    loadArmorAnim(ItemID::WOOD_LEGS, "assets/WoodLegs_Anim.png");
    loadArmorAnim(ItemID::WOOD_BOOTS, "assets/WoodBoots_Anim.png");

    if (!mAutotileTextures[ItemID::DIRT].loadFromFile("assets/dirt_autotile.png")) {
        std::cerr << "Error: Faltan las texturas de autotiling de la Tierra" << std::endl;
    }
}

// ==========================================
// FILE I/O (SAVE AND LOAD)
// ==========================================

/**
 * @brief Serializes the map structure to a binary file stream.
 */
void World::saveToStream(std::ofstream& file) {
    size_t count = mChunks.size();
    file.write(reinterpret_cast<const char*>(&count), sizeof(count));

    for (const auto& pair : mChunks) {
        int chunkX = pair.first;
        const std::vector<int>& blocks = pair.second;
        const std::vector<int>& walls = mBackgroundChunks[chunkX];

        file.write(reinterpret_cast<const char*>(&chunkX), sizeof(chunkX));
        file.write(reinterpret_cast<const char*>(blocks.data()), blocks.size() * sizeof(int));
        file.write(reinterpret_cast<const char*>(walls.data()), walls.size() * sizeof(int));
    }
}

/**
 * @brief Deserializes the map structure from a binary file stream.
 */
void World::loadFromStream(std::ifstream& file) {
    mChunks.clear();
    mBackgroundChunks.clear();
    mItems.clear(); // Clear dropped items to prevent load-duplication

    size_t count = 0;
    file.read(reinterpret_cast<char*>(&count), sizeof(count));

    for (size_t i = 0; i < count; ++i) {
        int chunkX = 0;
        file.read(reinterpret_cast<char*>(&chunkX), sizeof(chunkX));

        std::vector<int> blocks(CHUNK_WIDTH * WORLD_HEIGHT);
        std::vector<int> walls(CHUNK_WIDTH * WORLD_HEIGHT);

        file.read(reinterpret_cast<char*>(blocks.data()), blocks.size() * sizeof(int));
        file.read(reinterpret_cast<char*>(walls.data()), walls.size() * sizeof(int));

        mChunks[chunkX] = blocks;
        mBackgroundChunks[chunkX] = walls;
    }
}

// ==========================================
// ESCÁNER DE VECINOS (BITMASKING UNIVERSAL Y FUSIÓN)
// ==========================================
int World::getBitmask(int x, int y, int targetID) {
    int mask = 0;

    int top    = getBlock(x, y - 1);
    int right  = getBlock(x + 1, y);
    int bottom = getBlock(x, y + 1);
    int left   = getBlock(x - 1, y);

    // --- NUEVO: REGLAS DE FUSIÓN ENTRE DISTINTOS BLOQUES ---
    auto connects = [&](int neighbor) {
        // 1. Siempre nos conectamos perfectamente con nosotros mismos
        if (neighbor == targetID) return true;

        // 2. Regla de la Tierra: Se fusiona con la Piedra, Bedrock y los Minerales
        if (targetID == ItemID::DIRT) {
            if (neighbor == ItemID::STONE || neighbor == ItemID::BEDROCK ||
               (neighbor >= ItemID::COAL && neighbor <= ItemID::TUNGSTEN)) {
                return true;
               }
        }

        // (En el futuro, si haces Autotile para la Piedra, añadirás su regla aquí)
        // if (targetID == ItemID::STONE) { ... }

        return false; // Si no cumple nada, no nos fusionamos
    };

    // Evaluamos a los 4 vecinos con nuestras nuevas reglas
    if (connects(top))    mask += 1;
    if (connects(right))  mask += 2;
    if (connects(bottom)) mask += 4;
    if (connects(left))   mask += 8;

    return mask;
}

// ==========================================
// FÍSICAS DE BLOQUES (COLISIONES)
// ==========================================
bool World::isSolid(int blockID) {
    return (blockID != ItemID::AIR && blockID != ItemID::WOOD &&
            blockID != ItemID::LEAVES && blockID != ItemID::TORCH &&
            blockID != ItemID::DOOR_OPEN && blockID != ItemID::DOOR_OPEN_MID &&
            blockID != ItemID::DOOR_OPEN_TOP);
}