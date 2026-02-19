#pragma once
#include <SFML/Graphics.hpp>
#include <map>
#include <vector>
#include <cmath>
#include <fstream>

// Define Chunk size (e.g., 16 blocks wide)
const int CHUNK_WIDTH = 16;
const int WORLD_HEIGHT = 150; // Fixed vertical height (Sky to Bedrock)

struct ItemDrop {
    int id; // Block ID
    sf::Vector2f pos; // Drop position
    sf::Vector2f vel; // Drop velocity
    float timeAlive;
};

class World {
public:
    World(); // Default constructor

    // Renders the visible portion of the world based on the camera view (culling)
    void render(sf::RenderWindow& window, sf::Color ambientColor);

    // Gets the block type at a specific coordinate, generating the chunk if it doesn't exist
    int getBlock(int x, int y);
    // Sets the block type at a specific coordinate
    void setBlock(int x, int y, int type);

    float getTileSize() const { return mTileSize; }
    // Gets the texture for a given block ID, used for the UI hotbar
    const sf::Texture* getTexture(int id) const;

    // Move and get items
    void update(sf::Time dt, sf::Vector2f playerPos, std::map<int, int>& inventory);
    // --- NUEVO: GUARDADO Y CARGA ---
    // Reciben el archivo abierto desde Game.cpp para escribir/leer sus datos
    void saveToStream(std::ofstream& file);
    void loadFromStream(std::ifstream& file);
    void spawnItem(int id, sf::Vector2f pos);

private:
    // --- GENERATION HELPERS ---
    void generateChunk(int chunkX);

    // --- DATA ---
    float mTileSize;

    // THE CHUNK MAP
    // Key: Chunk Coordinate (X)
    // Value: A vector of integers representing the blocks in that chunk
    std::map<int, std::vector<int>> mChunks;

    std::map<int, std::vector<int>> mBackgroundChunks;

    // --- GRAPHICS RESOURCES ---
    // A map to store textures by Block identification
    // ID 1 -> Dirt Texture
    // ID 2 -> Grass Texture
    // ID 3 -> Stone Texture
    std::map<int, sf::Texture> mTextures;

    // A single reusable sprite for drawing all world tiles to improve performance
    sf::Sprite mSprite;

    // Helper method to load all necessary block textures
    void loadTextures();

    // --- ITEMS ---
    std::vector<ItemDrop> mItems;
    // Helper to create and item
    void spawnItem(int x, int y, int id);
};