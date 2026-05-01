#pragma once
#include <SFML/Graphics.hpp>
#include <map>
#include <vector>


// World generation constants
const int CHUNK_WIDTH = 16;
const int WORLD_HEIGHT = 150; // Fixed vertical height (Sky to Bedrock)

/**
 * @struct ItemDrop
 * @brief Represents an item physically dropped in the game world.
 */
struct ItemDrop {
    int id;           // Item/Block ID
    sf::Vector2f pos; // Drop position
    sf::Vector2f vel; // Physical velocity for tossing/bouncing
    float timeAlive;  // Time elapsed since dropped (for despawning/floating animation)
};

/**
 * @class World
 * @brief Manages procedural generation, block data, rendering, and dropped items.
 *
 * Uses an infinite horizontal chunk system. Chunks are generated on the fly
 * as the player requests blocks outside previously loaded areas.
 */
class World {
public:
    /**
     * @brief Constructs a new World object.
     * Sets tile sizes and loads textures.
     */
    World();

    /**
     * @brief Renders the visible portion of the world (culling) and dropped items.
     * @param window The render window.
     * @param ambientColor The global ambient light color for coloring blocks.
     */
    void render(sf::RenderWindow& window, sf::Color ambientColor);

    /**
     * @brief Gets the block type at a specific coordinate.
     * Generates the chunk automatically if it doesn't exist.
     * @param x Global grid X coordinate.
     * @param y Global grid Y coordinate.
     * @return The ID of the block at that location.
     */
    int getBlock(int x, int y);

    /**
     * @brief Sets the block type at a specific coordinate.
     * @param x Global grid X coordinate.
     * @param y Global grid Y coordinate.
     * @param type The new ItemID for the block (0 for Air).
     */
    void setBlock(int x, int y, int type);

    float getTileSize() const { return mTileSize; }

    /**
     * @brief Gets the UI icon texture for a specific ItemID.
     */
    const sf::Texture* getTexture(int id) const;

    /**
     * @brief Gets the specialized texture for rendering a tool held in the player's hand.
     */
    const sf::Texture* getHeldTexture(int id) const;

    /**
     * @brief Gets the spritesheet texture for animated armor layers.
     */
    const sf::Texture* getArmorAnimTexture(int id) const;

    /**
     * @brief Updates physics for dropped items and handles player pickup.
     * @param dt Time elapsed since the last frame.
     * @param playerPos The center position of the player for pickup radius detection.
     * @param inventory Reference to the temporary pickup map sent back to Game.cpp.
     */
    void update(sf::Time dt, sf::Vector2f playerPos, std::map<int, int>& inventory);

    // --- SAVE AND LOAD ---
    void saveToStream(std::ofstream& file);
    void loadFromStream(std::ifstream& file);

    /**
     * @brief Spawns an item drop at an exact pixel position.
     */
    void spawnItem(int id, sf::Vector2f pos);

    /**
     * @brief Spawns an item drop at a grid coordinate (centered).
     */
    void spawnItem(int x, int y, int id);

    /**
     * @brief Static helper to check if a given block ID has physical collision.
     * @param blockID The ID of the block to check.
     * @return True if the block is solid (collidable), false if passable (Air, Wood background, Open doors).
     */
    static bool isSolid(int blockID);

private:
    /**
     * @brief Procedurally generates a new chunk of terrain.
     * @param chunkX The chunk index to generate.
     */
    void generateChunk(int chunkX);

    /**
     * @brief Loads all textures for blocks, items, tools, and armor.
     */
    void loadTextures();

    // --- DATA ---
    float mTileSize;

    // THE CHUNK MAPS
    // Key: Chunk Coordinate (X)
    // Value: 1D Array representing the 2D grid of blocks in that chunk (Width * Height)
    std::map<int, std::vector<int>> mChunks;
    std::map<int, std::vector<int>> mBackgroundChunks; // Back wall layers

    // Graphics Resources
    std::map<int, sf::Texture> mTextures;          // Icons and block textures
    std::map<int, sf::Texture> mHeldTextures;      // Hand-held weapon/tool textures
    std::map<int, sf::Texture> mArmorAnimTextures; // Animated spritesheets for armor pieces

    sf::Sprite mSprite; // Shared sprite instance for high-performance rendering

    // Dynamic Entities
    std::vector<ItemDrop> mItems;
    // --- NUEVO: SISTEMA DE AUTOTILING ---
    std::map<int, sf::Texture> mAutotileTextures; // Guarda las texturas inteligentes
    int getBitmask(int x, int y, int targetID);
};