#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <SFML/System.hpp>
#include <SFML/Audio.hpp>
#include <iostream>
#include <string>
#include <stdexcept>
#include <algorithm>
#include <limits>
#include <utility>
#include <memory>
#include <vector>
#include <map>
#include <sstream>
#include <cmath>
#include <iomanip>
#include <functional>
#include <deque>
#include <random>


#ifdef _WIN32
#include <windows.h>
#endif

#if __cplusplus < 201402L
// Provide make_unique for C++11
namespace std {
    template<typename T, typename... Args>
    std::unique_ptr<T> make_unique(Args&&... args) {
        return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
    }
}
#endif

// =================================================================
// 0. GAME CONFIGURATION & GLOBALS
// =================================================================

namespace GameConfig {
    const unsigned int WINDOW_WIDTH = 1280;
    const unsigned int WINDOW_HEIGHT = 720;
    const unsigned int FRAMERATE_LIMIT = 60;
    const std::string FONT_PATH_ARIBLK = "ariblk.ttf";
    const std::string MENU_BG_PATH_PREFIX = "assets/"; // MODIFIED: Path prefix for assets
    const int MENU_BG_FRAME_COUNT = 20;
    const float BG_ANIMATION_DELAY = 0.08f;
    const float TRANSITION_DURATION = 0.7f;
    const float GAMEPLAY_TRANSITION_DURATION = 0.4f;
    const unsigned int MAX_NAME_LENGTH = 15;

    // --- Color Palette ---
    const sf::Color GOLD_COLOR = sf::Color(255, 215, 0);
    const sf::Color ALERT_RED_COLOR = sf::Color(220, 20, 60);
    const sf::Color LIGHT_RED_FLASH = sf::Color(255, 80, 80);
    const sf::Color OFF_WHITE_COLOR = sf::Color(245, 245, 245);
    const sf::Color WIN_GREEN_COLOR = sf::Color(60, 220, 60);
    const sf::Color LOG_BLUE_COLOR = sf::Color(173, 216, 230);
}

// --- Game State Identifiers ---
enum class GameStateID {
    NONE,
    MENU,
    NAME_INPUT,
    GAMEPLAY,
    GAME_OVER
};

// --- Transition States ---
enum class TransitionState {
    NONE,
    FADING_OUT,
    FADING_IN
};


// =================================================================
// 1. GAME LOGIC 
// =================================================================

class Player;
class Dungeon;

class Entity {
public:
    virtual ~Entity() = default;
    virtual std::string getDescription() const = 0;
    virtual void interact(Player& player, std::string& interactionResult) = 0;
    virtual std::string getName() const = 0;
};

class Item : public Entity {
protected:
    std::string name;
public:
    explicit Item(std::string n) : name(std::move(n)) {}
    std::string getName() const override { return name; }
    void interact(Player& player, std::string& interactionResult) override;
    std::string getDescription() const override { return "You see a " + name + "."; }
};

class Weapon : public Item {
public:
    explicit Weapon(std::string n) : Item(std::move(n)) {}
    std::string getDescription() const override { return "A powerful " + name + " rests here."; }
};

class Potion : public Item {
public:
    explicit Potion(std::string n) : Item(std::move(n)) {}
    std::string getDescription() const override { return "A bubbling " + name + " is on a pedestal.";}
};

class Key : public Item {
public:
    explicit Key(std::string n) : Item(std::move(n)) {}
    std::string getDescription() const override { return "A shiny " + name + " catches your eye.";}
};

class Enemy : public Entity {
protected:
    std::string name;
    int damage;
public:
    Enemy(std::string n, int d) : name(std::move(n)), damage(d) {}
    std::string getName() const override { return name; }
    int getDamage() const { return damage; }
    std::string getDescription() const override { return "DANGER! A " + name + " blocks your path."; }
    void interact(Player& player, std::string& interactionResult) override;
};

class MinionEnemy : public Enemy {
public:
    MinionEnemy(std::string n, int d) : Enemy(std::move(n), d) {}
};

class BossEnemy : public Enemy {
public:
    BossEnemy(std::string n, int d) : Enemy(std::move(n), d) {}
    void interact(Player& player, std::string& interactionResult) override;
};

template <typename T>
class Stack {
private:
    struct Node {
        T data;
        Node* next;
        Node(T d, Node* n) : data(d), next(n) {}
    };
    Node* topNode = nullptr;
    size_t count = 0;
public:
    ~Stack() { while (!isEmpty()) pop(); }
    Stack(const Stack&) = delete;
    Stack& operator=(const Stack&) = delete;
    Stack(Stack&&) = delete;
    Stack& operator=(Stack&&) = delete;
    Stack() = default;

    void push(const T& value) { topNode = new Node(value, topNode); count++; }
    void pop() {
        if (isEmpty()) return;
        Node* temp = topNode;
        topNode = topNode->next;
        delete temp;
        count--;
    }
    T& top() const { if (isEmpty()) throw std::runtime_error("Stack is empty."); return topNode->data; }
    bool isEmpty() const { return topNode == nullptr; }
    size_t size() const { return count; }
};

template <typename T>
class Inventory {
private:
    T* items = nullptr;
    size_t current_size = 0;
    size_t capacity = 0;
    void resize() {
        capacity = (capacity == 0) ? 2 : capacity * 2;
        T* newItems = new T[capacity];
        for (size_t i = 0; i < current_size; ++i) newItems[i] = items[i];
        delete[] items;
        items = newItems;
    }
public:
    Inventory() = default;
    ~Inventory() { delete[] items; }
    Inventory(const Inventory& other) : current_size(other.current_size), capacity(other.capacity) {
        items = new T[capacity];
        for (size_t i = 0; i < current_size; ++i) items[i] = other.items[i];
    }
    Inventory& operator=(const Inventory& other) {
        if (this == &other) return *this;
        delete[] items;
        current_size = other.current_size;
        capacity = other.capacity;
        items = new T[capacity];
        for (size_t i = 0; i < current_size; ++i) items[i] = other.items[i];
        return *this;
    }

    void add(const T& item) { if (current_size == capacity) resize(); items[current_size++] = item; }
    bool has(const T& item) const {
        for (size_t i = 0; i < current_size; ++i) if (items[i] == item) return true;
        return false;
    }
    size_t size() const { return current_size; }
    std::string getSortedString() const {
        if (current_size == 0) return "Empty";
        T* sorted = new T[current_size];
        for (size_t i = 0; i < current_size; ++i) sorted[i] = items[i];
        std::sort(sorted, sorted + current_size);
        std::stringstream ss;
        for (size_t i = 0; i < current_size; ++i) ss << sorted[i] << (i == current_size - 1 ? "" : ", ");
        delete[] sorted;
        return ss.str();
    }
};

class Player {
private:
    std::string name;
    int health;
    int moves;
    Inventory<std::string> inventory;
    bool finalBossDefeated = false;
public:
    Player(std::string n, int h, int m) : name(std::move(n)), health(h), moves(m) {}

    std::string getName() const { return name; }
    int getHealth() const { return health; }
    int getMoves() const { return moves; }
    bool isFinalBossDefeated() const { return finalBossDefeated; }
    const Inventory<std::string>& getInventory() const { return inventory; }

    std::string takeDamage(int damage) {
        health -= damage;
        if (health < 0) health = 0;
        std::stringstream ss;
        ss << "You took " << damage << " damage!";
        return ss.str();
    }
    void heal(int amount) { health = std::min(100, health + amount); }
    void useMove() { moves--; } // Allow moves to go into negative to detect game over.
    void collectItem(const std::string& item) { inventory.add(item); }
    bool hasItem(const std::string& item) const { return inventory.has(item); }
    void setBossDefeated(bool status) { finalBossDefeated = status; }
    // Copy Constructor for the Player class
Player(const Player& other)
    : name(other.name),
      health(other.health),
      moves(other.moves),
      inventory(other.inventory), // This correctly calls the Inventory's copy constructor
      finalBossDefeated(other.finalBossDefeated)
{
    // We add this message for demonstration, to see when the copy happens.
    std::cout 
    << "[DEBUG] Player copy constructor was called to create a copy of: " << other.name << std::endl;
}
// Copy Assignment Operator for the Player class
Player& operator=(const Player& other) {
    // We add this message for demonstration.
    std::cout << "[DEBUG] Player copy assignment operator called to assign from: " << other.name << std::endl;

    // Step 1: Handle self-assignment. This is a critical check!
    if (this == &other) {
        return *this;
    }

    // Step 2: Copy the data members from the other object to this one.
    this->name = other.name;
    this->health = other.health;
    this->moves = other.moves;
    this->inventory = other.inventory; // This calls the Inventory's assignment operator
    this->finalBossDefeated = other.finalBossDefeated;

    // Step 3: Return a reference to the current object to allow for chaining (e.g., a = b = c).
    return *this;
}
};

class Room {
public:
    std::string name;
    std::string description;
    std::unique_ptr<Entity> entity;
    Room* next;
    bool isFinalDoor = false;
    bool isChoiceRoom = false;
    std::string backgroundID;

    Room(std::string n, std::string d, std::string bgID)
    : name(std::move(n)), description(std::move(d)), entity(nullptr), next(nullptr), backgroundID(std::move(bgID)) {}
};

class Dungeon {
private:
    Room* head = nullptr;
    Room* currentRoom = nullptr;
    Player& player;
    Stack<Room*> path_tracker;
public:
    explicit Dungeon(Player& p) : player(p) {}
    ~Dungeon() {
        Room* current = head;
        while (current != nullptr) {
            Room* to_delete = current;
            current = current->next;
            delete to_delete;
        }
    }
    Dungeon(const Dungeon&) = delete;
    Dungeon& operator=(const Dungeon&) = delete;

    void addRoom(Room* newRoom) {
        if (!head) { head = currentRoom = newRoom; }
        else {
            Room* temp = head;
            while (temp->next) temp = temp->next;
            temp->next = newRoom;
        }
    }

    Room* getCurrentRoom() const { return currentRoom; }
    bool canMoveBack() const { return !path_tracker.isEmpty(); }
    bool canMoveForward() const { return currentRoom && currentRoom->next; }

    void moveForward() {
        if (canMoveForward()) {
            player.useMove();
            path_tracker.push(currentRoom);
            currentRoom = currentRoom->next;
        }
    }

    void moveBack() {
        if (canMoveBack()) {
            player.useMove();
            currentRoom = path_tracker.top();
            path_tracker.pop();
        }
    }
};

void Item::interact(Player& player, std::string& interactionResult) {
    if (dynamic_cast<Potion*>(this)) {
        player.heal(100);
        interactionResult = "You drank the potion and feel fully restored!";
    } else {
        player.collectItem(this->getName());
        interactionResult = "You picked up the " + this->getName() + ".";
    }
}

void Enemy::interact(Player& player, std::string& interactionResult) {
    interactionResult = "";
}

void BossEnemy::interact(Player& player, std::string& interactionResult) {
    interactionResult = "";
}


// =================================================================
// 2. GUI FRAMEWORK & SCREENS
// =================================================================
class Game;

namespace Utils {
    void centerOrigin(sf::Text& text) {
        sf::FloatRect bounds = text.getLocalBounds();
        text.setOrigin(bounds.left + bounds.width / 2.0f, bounds.top + bounds.height / 2.0f);
    }

    void centerOrigin(sf::Sprite& sprite) {
        sf::FloatRect bounds = sprite.getLocalBounds();
        sprite.setOrigin(bounds.width / 2.0f, bounds.height / 2.0f);
    }

    void centerOrigin(sf::RectangleShape& rect) {
        sf::Vector2f size = rect.getSize();
        rect.setOrigin(size.x / 2.0f, size.y / 2.0f);
    }

    void wrapText(sf::Text& text, float maxWidth) {
        std::string string = text.getString();
        if (string.empty()) return;

        std::string wrappedString;
        std::string currentLine;
        std::stringstream ss(string);
        std::string word;

        while (ss >> word) {
            sf::Text tempText = text;
            std::string testLine = currentLine.empty() ? word : currentLine + " " + word;
            tempText.setString(testLine);

            if (tempText.getGlobalBounds().width > maxWidth && !currentLine.empty()) {
                wrappedString += currentLine + "\n";
                currentLine = word;
            } else {
                if (!currentLine.empty()) currentLine += " ";
                currentLine += word;
            }
        }
        wrappedString += currentLine;
        text.setString(wrappedString);
    }

    float lerp(float a, float b, float t) { return a + t * (b - a); }
    float clamp(float value, float min, float max) { return std::max(min, std::min(value, max)); }
}

class ResourceManager {
private:
    static std::map<std::string, sf::Font> fonts;
    static std::map<std::string, sf::Texture> textures;
    static std::map<std::string, std::vector<sf::Texture>> texture_frames;
public:
    static sf::Font& getFont(const std::string& id) {
        if (fonts.find(id) == fonts.end()) {
            sf::Font font;
            // MODIFIED: Prepend assets/ path
            if (!font.loadFromFile("assets/" + id)) { std::cerr << "Error loading font: " << id << std::endl; }
            fonts[id] = font;
        }
        return fonts[id];
    }
    static sf::Texture& getTexture(const std::string& id) {
        if (textures.find(id) == textures.end()) {
            sf::Texture tex;
             // Prepend assets/ path
            if (!tex.loadFromFile("assets/" + id)) {
                std::cerr << "\n\n======================================================\n";
                std::cerr << "FATAL ERROR: Could not load texture: 'assets/" << id << "'\n";
                std::cerr << "Please ensure the file exists in the executable's\n";
                std::cerr << "'assets' sub-directory and the name is correct.\n";
                std::cerr << "======================================================\n\n";
            }
            textures[id] = tex;
        }
        return textures[id];
    }
    static std::vector<sf::Texture>& getBackgroundFrames(const std::string& id, const std::string& prefix, int frameCount) {
        if (texture_frames.find(id) == texture_frames.end()) {
            std::vector<sf::Texture> frames;
            frames.reserve(frameCount);
            for (int i = 1; i <= frameCount; ++i) {
                sf::Texture tex;
                std::stringstream ss;
                ss << prefix << std::setw(6) << std::setfill('0') << i << ".png";
                if (!tex.loadFromFile(ss.str())) {
                    std::cerr << "Error loading background frame: " << ss.str() << std::endl;
                    break;
                }
                frames.push_back(tex);
            }
            texture_frames[id] = frames;
        }
        return texture_frames[id];
    }
};
std::map<std::string, sf::Font> ResourceManager::fonts;
std::map<std::string, sf::Texture> ResourceManager::textures;
std::map<std::string, std::vector<sf::Texture>> ResourceManager::texture_frames;

class Screen {
public:
    virtual ~Screen() = default;
    virtual void handleEvent(sf::Event& event, Game& game) = 0;
    virtual void update(sf::Time dt, Game& game) = 0;
    virtual void draw(sf::RenderWindow& window) = 0;
    virtual void onEnter(Game& game) {}
    virtual void onResize(unsigned int width, unsigned int height) = 0;
};

class Game {
public:
    sf::RenderWindow window;
    sf::View mainView;
    std::map<GameStateID, std::unique_ptr<Screen>> screens;
    GameStateID currentStateID = GameStateID::NONE;
    GameStateID nextStateID = GameStateID::NONE;
    TransitionState currentTransition = TransitionState::NONE;
    sf::Clock transitionClock;
    sf::RectangleShape transitionRect;
    sf::Clock gameClock;
    std::string playerName;
    std::unique_ptr<Player> playerLogic;
    std::unique_ptr<Dungeon> dungeonLogic;

    // --- Screen Shake Members ---
    bool isShaking = false;
    float shakeDuration = 0.f;
    float shakeMagnitude = 0.f;
    sf::Clock shakeClock;
    std::mt19937 rng{std::random_device{}()};

    Game();
    void run();
    void changeScreen(GameStateID newStateID);
    void startGameplay();
    void handleResize(unsigned int width, unsigned int height);
    void triggerScreenShake(float duration, float magnitude);
private:
    void processEvents();
    void update(sf::Time dt);
    void render();
    void handleScreenTransition(sf::Time dt);
    void updateScreenShake();
};

class AnimatedScreen : public Screen {
public:
    sf::Sprite backgroundSprite;
    std::vector<sf::Texture>& bgFrames;
    int currentBgFrame = 0;
    float bgFrameTimer = 0.f;

    AnimatedScreen(const std::string& frame_id, const std::string& prefix, int frameCount)
    : bgFrames(ResourceManager::getBackgroundFrames(frame_id, prefix, frameCount))
    {
        if (!bgFrames.empty()) {
            backgroundSprite.setTexture(bgFrames[0]);
        }
    }

    void updateAnimation(sf::Time dt) {
        if (bgFrames.empty() || bgFrames.size() <= 1) return;

        bgFrameTimer += dt.asSeconds();
        if (bgFrameTimer >= GameConfig::BG_ANIMATION_DELAY) {
            bgFrameTimer -= GameConfig::BG_ANIMATION_DELAY;
            currentBgFrame = (currentBgFrame + 1) % bgFrames.size();
            backgroundSprite.setTexture(bgFrames[currentBgFrame]);
        }
    }

    void onEnter(Game& game) override {
        bgFrameTimer = 0.f;
        currentBgFrame = 0;
        if (!bgFrames.empty()) {
            backgroundSprite.setTexture(bgFrames[0]);
        }
    }

    void drawBackground(sf::RenderWindow& window) {
        const sf::Texture* tex = backgroundSprite.getTexture();
        if (tex && tex->getSize().x > 0) {
            sf::Vector2f targetSize(GameConfig::WINDOW_WIDTH, GameConfig::WINDOW_HEIGHT);
            sf::Vector2u texSize = tex->getSize();
            float scaleX = targetSize.x / texSize.x;
            float scaleY = targetSize.y / texSize.y;
            float scale = std::min(scaleX, scaleY); 

            backgroundSprite.setScale(scale, scale);
            Utils::centerOrigin(backgroundSprite);
            backgroundSprite.setPosition(targetSize.x / 2.0f, targetSize.y / 2.0f);

            window.draw(backgroundSprite);
        } else {
            window.clear(sf::Color(10, 0, 10)); 
        }
    }
};

class MenuScreen : public AnimatedScreen {
    sf::Text titleText, pressEnterText;
public:
    MenuScreen() : AnimatedScreen("menu_bg", GameConfig::MENU_BG_PATH_PREFIX, GameConfig::MENU_BG_FRAME_COUNT) {
        titleText.setFont(ResourceManager::getFont(GameConfig::FONT_PATH_ARIBLK));
        titleText.setString("DUNGEON ESCAPE");
        titleText.setCharacterSize(108);
        titleText.setFillColor(GameConfig::GOLD_COLOR);
        titleText.setOutlineColor(sf::Color::Black);
        titleText.setOutlineThickness(8); 
        Utils::centerOrigin(titleText);

        pressEnterText.setFont(ResourceManager::getFont(GameConfig::FONT_PATH_ARIBLK));
        pressEnterText.setString("PRESS ENTER");
        pressEnterText.setCharacterSize(55);
        pressEnterText.setFillColor(GameConfig::GOLD_COLOR);
        pressEnterText.setOutlineColor(sf::Color::Black);
        pressEnterText.setOutlineThickness(5); 
        Utils::centerOrigin(pressEnterText);
    }

    void onResize(unsigned int width, unsigned int height) override {
        titleText.setPosition(width / 2.0f, height * 0.4f);
        pressEnterText.setPosition(width / 2.0f, height * 0.65f);
    }

    void handleEvent(sf::Event& event, Game& game) override {
        if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Enter) {
            game.changeScreen(GameStateID::NAME_INPUT);
        }
    }

    void update(sf::Time dt, Game& game) override {
        updateAnimation(dt);
    }

    void draw(sf::RenderWindow& window) override {
        drawBackground(window);
        window.draw(titleText);
        window.draw(pressEnterText);
    }
};

class NameInputScreen : public AnimatedScreen {
    sf::Text promptText, nameDisplay, cursorText, continueText;
    sf::RectangleShape inputBox;
    std::string& playerNameRef;
    bool isActive = true, showCursor = true;
    sf::Clock cursorBlinkClock;
public:
    NameInputScreen(std::string& playerNameOutput)
    : AnimatedScreen("menu_bg", GameConfig::MENU_BG_PATH_PREFIX, GameConfig::MENU_BG_FRAME_COUNT), playerNameRef(playerNameOutput) {
        sf::Font& font = ResourceManager::getFont(GameConfig::FONT_PATH_ARIBLK);

        promptText.setFont(font);
        promptText.setString("ENTER YOUR NAME:");
        promptText.setCharacterSize(60);
        promptText.setFillColor(GameConfig::GOLD_COLOR);
        promptText.setOutlineColor(sf::Color::Black);
        promptText.setOutlineThickness(5);
        Utils::centerOrigin(promptText);

        inputBox.setSize({600, 75});
        inputBox.setOutlineThickness(4);
        inputBox.setOutlineColor(GameConfig::GOLD_COLOR);
        inputBox.setFillColor({10, 10, 10, 200});
        Utils::centerOrigin(inputBox);

        nameDisplay.setFont(font);
        nameDisplay.setCharacterSize(45);
        nameDisplay.setFillColor(GameConfig::OFF_WHITE_COLOR);

        cursorText.setFont(font);
        cursorText.setString("|");
        cursorText.setCharacterSize(45);
        cursorText.setFillColor(GameConfig::OFF_WHITE_COLOR);

        continueText.setFont(font);
        continueText.setString("PRESS ENTER TO BEGIN");
        continueText.setCharacterSize(35);
        continueText.setFillColor(GameConfig::GOLD_COLOR);
        continueText.setOutlineColor(sf::Color::Black);
        continueText.setOutlineThickness(4);
        Utils::centerOrigin(continueText);
    }

    void onEnter(Game& game) override {
        AnimatedScreen::onEnter(game);
        playerNameRef.clear();
        nameDisplay.setString("");
        isActive = true;
        showCursor = true;
        cursorBlinkClock.restart();
    }

    void onResize(unsigned int width, unsigned int height) override {
        promptText.setPosition(width / 2.0f, height * 0.3f);
        inputBox.setPosition(width / 2.0f, height * 0.5f);
        nameDisplay.setPosition(inputBox.getPosition().x - inputBox.getSize().x / 2.f + 15, inputBox.getPosition().y - inputBox.getSize().y / 2.f + 10);
        continueText.setPosition(width / 2.0f, height * 0.75f);
    }

    void handleEvent(sf::Event& event, Game& game) override {
        if (!isActive) return;

        if (event.type == sf::Event::TextEntered) {
            if (event.text.unicode == '\b') { // Backspace
                if (!playerNameRef.empty()) playerNameRef.pop_back();
            } else if (event.text.unicode >= 32 && event.text.unicode < 127) { // Printable chars
                if (playerNameRef.length() < GameConfig::MAX_NAME_LENGTH) {
                    playerNameRef += static_cast<char>(event.text.unicode);
                }
            }
            nameDisplay.setString(playerNameRef);
            showCursor = true;
            cursorBlinkClock.restart();
        }

        if (event.type == sf::Event::KeyPressed) {
            if (event.key.code == sf::Keyboard::Enter && !playerNameRef.empty()) {
                isActive = false;
                game.startGameplay();
            } else if (event.key.code == sf::Keyboard::Escape) {
                game.changeScreen(GameStateID::MENU);
            }
        }
    }

    void update(sf::Time dt, Game& game) override {
        updateAnimation(dt);
        
        if (isActive && cursorBlinkClock.getElapsedTime().asSeconds() > 0.5f) {
            showCursor = !showCursor;
            cursorBlinkClock.restart();
        }
    }

    void draw(sf::RenderWindow& window) override {
        drawBackground(window);
        window.draw(promptText);
        window.draw(inputBox);
        window.draw(nameDisplay);
        if (isActive && showCursor) {
            cursorText.setPosition(nameDisplay.getPosition().x + nameDisplay.getGlobalBounds().width + 5, nameDisplay.getPosition().y);
            window.draw(cursorText);
        }
        if (!playerNameRef.empty()) {
            window.draw(continueText);
        }
    }
};

class GameOverScreen : public Screen {
    sf::Text gameOverText, reasonText, continueText;
    sf::Sprite background;
    sf::RectangleShape overlay;
public:
    GameOverScreen(const std::string& reason, std::string finalBgId) {
        sf::Font& font = ResourceManager::getFont(GameConfig::FONT_PATH_ARIBLK);

        gameOverText.setFont(font);
        gameOverText.setString("GAME OVER");
        gameOverText.setCharacterSize(120);
        gameOverText.setOutlineColor(sf::Color::Black);
        gameOverText.setOutlineThickness(8);
        Utils::centerOrigin(gameOverText);

        reasonText.setFont(font);
        reasonText.setString(reason);
        reasonText.setCharacterSize(50);
        reasonText.setFillColor(GameConfig::OFF_WHITE_COLOR);
        reasonText.setOutlineColor(sf::Color::Black);
        reasonText.setOutlineThickness(5);
        Utils::centerOrigin(reasonText);

        continueText.setFont(font);
        continueText.setString("PRESS ENTER TO RETURN TO MENU");
        continueText.setCharacterSize(35);
        continueText.setFillColor(GameConfig::GOLD_COLOR);
        continueText.setOutlineColor(sf::Color::Black);
        continueText.setOutlineThickness(4);
        Utils::centerOrigin(continueText);

        background.setTexture(ResourceManager::getTexture(finalBgId));
        overlay.setFillColor(sf::Color(0, 0, 0, 180));

        bool win = reason.find("VICTORIOUS") != std::string::npos;
        gameOverText.setFillColor(win ? GameConfig::WIN_GREEN_COLOR : GameConfig::ALERT_RED_COLOR);
        if(win) gameOverText.setString("VICTORY!");
        Utils::centerOrigin(gameOverText);
    }

    void onResize(unsigned int width, unsigned int height) override {
        gameOverText.setPosition(width / 2.0f, height * 0.35f);
        reasonText.setPosition(width / 2.0f, height * 0.55f);
        continueText.setPosition(width / 2.0f, height * 0.8f);

        Utils::wrapText(reasonText, width * 0.8f);
        Utils::centerOrigin(reasonText); 
        reasonText.setPosition(width / 2.0f, height * 0.55f);


        overlay.setSize({(float)width, (float)height});
    }

    void handleEvent(sf::Event& event, Game& game) override {
        if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Enter) {
            game.changeScreen(GameStateID::MENU);
        }
    }

    void update(sf::Time dt, Game& game) override {
    }

    void draw(sf::RenderWindow& window) override {
        const sf::Texture* tex = background.getTexture();
        if (tex && tex->getSize().x > 0) {
            sf::Vector2f targetSize((float)GameConfig::WINDOW_WIDTH, (float)GameConfig::WINDOW_HEIGHT);
            sf::Vector2u texSize = tex->getSize();
            float scaleX = targetSize.x / texSize.x;
            float scaleY = targetSize.y / texSize.y;
            float scale = std::min(scaleX, scaleY);
            background.setScale(scale, scale);
            Utils::centerOrigin(background);
            background.setPosition(targetSize.x / 2.0f, targetSize.y / 2.0f);
            window.draw(background);
        } else {
            window.clear(sf::Color(10, 0, 10));
        }

        window.draw(overlay);
        window.draw(gameOverText);
        window.draw(reasonText);
        window.draw(continueText);
    }
};

// =================================================================
// 3. GAMEPLAY SCREEN (The Bridge)
// =================================================================

class GamePlayScreen : public Screen {
private:
    enum class InteractionState { EXPLORING, COMBAT, CHOICE, MESSAGE };
    InteractionState currentState = InteractionState::EXPLORING;

    enum class GpTransitionState { NONE, FADING_OUT, FADING_IN };
    GpTransitionState transitionState = GpTransitionState::NONE;
    sf::RectangleShape transitionOverlay;
    sf::Clock transitionClock;
    std::function<void()> onTransitionComplete;

    Game& game;
    sf::Sprite background;
    sf::RectangleShape uiPanel, messagePanel;
    sf::Text roomNameText, roomDescText, entityDescText, playerStatsText, actionPromptsText, interactionText, recentActionsTitle, recentActionsText;

    sf::RectangleShape damageFlash;
    sf::Clock flashClock;
    float flashDuration = 0.f;

    std::string message;
    std::deque<std::string> actionsLog;
    const size_t MAX_LOG_SIZE = 4;
    
    bool m_isNewRoomEntry = true;


    void drawBackground(sf::RenderWindow& window) {
        const sf::Texture* tex = background.getTexture();
        if (tex && tex->getSize().x > 0) {
            sf::Vector2f targetSize((float)GameConfig::WINDOW_WIDTH, (float)GameConfig::WINDOW_HEIGHT);
            sf::Vector2u texSize = tex->getSize();
            float scaleX = targetSize.x / texSize.x;
            float scaleY = targetSize.y / texSize.y;
            float scale = std::min(scaleX, scaleY);
            background.setScale(scale, scale);
            Utils::centerOrigin(background);
            background.setPosition(targetSize.x / 2.0f, targetSize.y / 2.0f);
            window.draw(background);
        } else {
            window.clear(sf::Color(10, 0, 10));
        }
    }

public:
    explicit GamePlayScreen(Game& g) : game(g) {
        sf::Font& font = ResourceManager::getFont(GameConfig::FONT_PATH_ARIBLK);

        auto styleText = [&](sf::Text& text, const sf::Color& color, int size, int outline) {
            text.setFont(font);
            text.setFillColor(color);
            text.setCharacterSize(size);
            text.setOutlineColor(sf::Color::Black);
            text.setOutlineThickness(outline);
        };

        styleText(roomNameText, GameConfig::GOLD_COLOR, 48, 5);
        styleText(roomDescText, GameConfig::OFF_WHITE_COLOR, 28, 4);
        roomDescText.setLineSpacing(1.2f);
        styleText(entityDescText, GameConfig::ALERT_RED_COLOR, 30, 4);
        entityDescText.setLineSpacing(1.2f);

        styleText(playerStatsText, GameConfig::OFF_WHITE_COLOR, 20, 2);
        playerStatsText.setLineSpacing(1.3f);

        styleText(recentActionsTitle, GameConfig::GOLD_COLOR, 24, 3);
        recentActionsTitle.setString("Recent Actions");
        Utils::centerOrigin(recentActionsTitle);
        styleText(recentActionsText, GameConfig::LOG_BLUE_COLOR, 17, 2);
        recentActionsText.setLineSpacing(1.3f);

        styleText(actionPromptsText, GameConfig::GOLD_COLOR, 26, 3);
        actionPromptsText.setLineSpacing(1.3f);

        styleText(interactionText, GameConfig::OFF_WHITE_COLOR, 32, 4);
        interactionText.setLineSpacing(1.2f);

        uiPanel.setFillColor({0, 0, 0, 200});
        uiPanel.setOutlineColor(GameConfig::GOLD_COLOR);
        uiPanel.setOutlineThickness(3.f);

        messagePanel.setFillColor({20, 0, 0, 230});
        messagePanel.setOutlineColor(GameConfig::ALERT_RED_COLOR);
        messagePanel.setOutlineThickness(3.f);

        transitionOverlay.setFillColor(sf::Color(0,0,0,0));

        damageFlash.setFillColor(sf::Color::Transparent);
        damageFlash.setOutlineColor(sf::Color::Transparent);
        damageFlash.setOutlineThickness(15);
    }

    void onEnter(Game& gameRef) override {
        actionsLog.clear();
        m_isNewRoomEntry = true;
        addAction("Your adventure begins...");
        checkRoomState();
        onResize(GameConfig::WINDOW_WIDTH, GameConfig::WINDOW_HEIGHT);
    }

    void onResize(unsigned int width, unsigned int height) override {
        uiPanel.setSize({(float)width, 200.f});
        uiPanel.setPosition(0, height - 200.f);
        
        roomNameText.setPosition(30.f, 30.f);
        roomDescText.setPosition(30.f, 100.f);

        float panelX = uiPanel.getPosition().x;
        float panelY = uiPanel.getPosition().y;
        float panelW = uiPanel.getSize().x;
        
        playerStatsText.setPosition(panelX + 30, panelY + 20);
        recentActionsTitle.setPosition(panelX + panelW / 2.f, panelY + 25);
        recentActionsText.setPosition(panelX + panelW / 2.f - 200, panelY + 60);

        messagePanel.setSize({width * 0.7f, height * 0.5f});
        Utils::centerOrigin(messagePanel);
        messagePanel.setPosition(width/2.f, height/2.f);
        interactionText.setPosition(messagePanel.getPosition().x - messagePanel.getOrigin().x + 20, messagePanel.getPosition().y - messagePanel.getOrigin().y + 20);

        transitionOverlay.setSize({(float)width, (float)height});
        damageFlash.setSize({(float)width, (float)height});
        updateUI();
    }

    void startTransition(std::function<void()> callback) {
        if (transitionState == GpTransitionState::NONE) {
            transitionState = GpTransitionState::FADING_OUT;
            onTransitionComplete = std::move(callback);
            transitionClock.restart();
        }
    }

    void addAction(const std::string& action) {
        if (action.empty() || (!actionsLog.empty() && action == actionsLog.front())) {
            return;
        }
        actionsLog.push_front(action);
        if (actionsLog.size() > MAX_LOG_SIZE) {
            actionsLog.pop_back();
        }
    }

    void handleEvent(sf::Event& event, Game& gameRef) override {
        if (event.type != sf::Event::KeyPressed) return;
        if (transitionState != GpTransitionState::NONE) return;

        switch (currentState) {
            case InteractionState::EXPLORING:
                if (event.key.code == sf::Keyboard::F) {
                    if (game.playerLogic->getMoves() <= 0) {
                        triggerGameOver("You have run out of moves.");
                        break;
                    }
                    if (game.dungeonLogic->canMoveForward()){
                        startTransition([this](){ m_isNewRoomEntry = true; game.dungeonLogic->moveForward(); });
                    }
                }
                else if (event.key.code == sf::Keyboard::B) {
                    if (game.playerLogic->getMoves() <= 0) {
                        triggerGameOver("You have run out of moves.");
                        break;
                    }
                    if(game.dungeonLogic->canMoveBack()) {
                       startTransition([this](){ m_isNewRoomEntry = true; game.dungeonLogic->moveBack(); });
                    }
                }
                else if (event.key.code == sf::Keyboard::C) {
                    Room* room = game.dungeonLogic->getCurrentRoom();
                    if (room && room->entity && dynamic_cast<Weapon*>(room->entity.get()) && room->name == "Chamber of the Cursed Blades") {
                        startTransition([this, room]() {
                            game.playerLogic->collectItem("Sword");
                            addAction("You collected the Sword.");
                            room->description = "You grasp the sword. A surge of ultimate power floods your veins.";
                            room->entity.reset(nullptr); 
                            currentState = InteractionState::EXPLORING;
                        });
                    }
                }
                else if (event.key.code == sf::Keyboard::Q) {
                    Room* room = game.dungeonLogic->getCurrentRoom();
                    bool inSwordRoomWithSword = room && room->entity && dynamic_cast<Weapon*>(room->entity.get()) && room->name == "Chamber of the Cursed Blades";
                    if (!inSwordRoomWithSword) {
                         game.changeScreen(GameStateID::MENU);
                    }
                }
                break;
            case InteractionState::COMBAT:
                if (event.key.code == sf::Keyboard::F) { startTransition([this](){ handleCombat(); }); }
                else if (event.key.code == sf::Keyboard::R) { addAction("Fled in terror!"); triggerGameOver("You fled in terror!"); }
                break;
            case InteractionState::CHOICE:
                if (event.key.code == sf::Keyboard::Num1 || event.key.code == sf::Keyboard::Numpad1) { startTransition([this](){ handleChoice(1); }); }
                else if (event.key.code == sf::Keyboard::Num2 || event.key.code == sf::Keyboard::Numpad2) { startTransition([this](){ handleChoice(2); }); }
                break;
            case InteractionState::MESSAGE:
                if (event.key.code == sf::Keyboard::Enter) { startTransition([this](){ currentState = InteractionState::EXPLORING; }); }
                break;
        }
    }

    void triggerGameOver(const std::string& reason) {
        std::string bg = game.dungeonLogic->getCurrentRoom() ? game.dungeonLogic->getCurrentRoom()->backgroundID : "dungeon.png";
        game.screens[GameStateID::GAME_OVER] = std::make_unique<GameOverScreen>(reason, bg);
        game.changeScreen(GameStateID::GAME_OVER);
    }

    void checkRoomState() {
        Room* room = game.dungeonLogic->getCurrentRoom();
        if (!room) return;
        
        if (m_isNewRoomEntry) {
            addAction("Entered The " + room->name);
            m_isNewRoomEntry = false;
        }


        if (room->isFinalDoor) {
             if (game.playerLogic->hasItem("Golden Key") && game.playerLogic->isFinalBossDefeated()) {
                 triggerGameOver("You used the Golden key and escaped. You are VICTORIOUS!");
                 return;
             } else if (game.playerLogic->hasItem("Golden Key")) {
                 triggerGameOver("The final door is locked tight. The boss must be defeated!");
             }
             else {
                 triggerGameOver("The final door is locked. You needed the Golden Key.");
             }
        }
        else if (room->entity) {
            if (dynamic_cast<Weapon*>(room->entity.get()) && room->name == "Chamber of the Cursed Blades") {
                currentState = InteractionState::EXPLORING;
            }
            else if (dynamic_cast<Enemy*>(room->entity.get())) {
                currentState = InteractionState::COMBAT;
            }
            else { 
                handleItemInteraction();
            }
        }
        else if (room->isChoiceRoom) { currentState = InteractionState::CHOICE; }
        else { currentState = InteractionState::EXPLORING; }
        updateUI();
    }

    void handleItemInteraction() {
        Room* room = game.dungeonLogic->getCurrentRoom();
        if (!room || !room->entity) return;
        Item* item = dynamic_cast<Item*>(room->entity.get());
        if (!item) return;

        std::string result;
        item->interact(*game.playerLogic, result);
        addAction(result);
        room->description = result;
        room->entity.reset(nullptr);

        currentState = InteractionState::EXPLORING;
        updateUI();
    }

    void handleCombat() {
        Room* room = game.dungeonLogic->getCurrentRoom();
        if (!room || !room->entity) return;
        Enemy* enemy = dynamic_cast<Enemy*>(room->entity.get());
        if(!enemy) return;

        addAction("Fought the " + enemy->getName());
        int effectiveDamage = enemy->getDamage();
        message = "";

        if (dynamic_cast<BossEnemy*>(enemy)) {
            if(game.playerLogic->hasItem("Sword")) {
                 message = "Your Sword glows, weakening the boss!\n";
                 effectiveDamage = 50;
            } else {
                 message = "You are unarmed against the mighty boss!\n";
            }
        }

        std::string damageResult = game.playerLogic->takeDamage(effectiveDamage);
        message += damageResult;

        game.triggerScreenShake(0.3f, 20.f);
        flashDuration = 0.25f;
        flashClock.restart();

        if (game.playerLogic->getHealth() <= 0) {
            addAction("You were slain by the " + enemy->getName());
            triggerGameOver("You have died in combat.");
            return;
        }
        
        std::string enemyName = enemy->getName();
        addAction("Defeated the " + enemyName);
        if (dynamic_cast<BossEnemy*>(enemy)) {
            game.playerLogic->setBossDefeated(true);
            addAction("The final boss is defeated!");
        }
        
        room->description = "You defeated the " + enemyName + ". The way is clear. (You took " + std::to_string(effectiveDamage) + " damage)";

        room->entity.reset(nullptr);
        currentState = InteractionState::MESSAGE;
    }

    void handleChoice(int choice) {
        Room* room = game.dungeonLogic->getCurrentRoom();
        if (!room || !room->isChoiceRoom) return;
        
        if (choice == 1) {
            game.playerLogic->collectItem("Golden Key");
            addAction("You took the Golden Key.");
            room->description = "You took the Golden Key.";
        } else {
            game.playerLogic->heal(100);
            addAction("You drank the Health Potion.");
            room->description = "You drank the Health Potion.";
        }
        room->isChoiceRoom = false;
        
        currentState = InteractionState::EXPLORING;
    }

    void updateUI() {
        Room* room = game.dungeonLogic->getCurrentRoom();
        if (!room) return;

        sf::Texture& bgTex = ResourceManager::getTexture(room->backgroundID);
        if (bgTex.getSize().x > 0) background.setTexture(bgTex, true);

        roomNameText.setString(room->name);
        
        roomDescText.setString(room->description);

        Utils::wrapText(roomDescText, GameConfig::WINDOW_WIDTH - 60);
        roomDescText.setPosition(30.f, 100.f);

        entityDescText.setString("");
        if(room->entity) {
            entityDescText.setString(room->entity->getDescription());
            if(dynamic_cast<Enemy*>(room->entity.get())) {
                entityDescText.setFillColor(GameConfig::ALERT_RED_COLOR);
            } else {
                entityDescText.setFillColor(GameConfig::GOLD_COLOR);
            }
        }
        entityDescText.setPosition(30.f, roomDescText.getPosition().y + roomDescText.getGlobalBounds().height + 40.f);
        
        std::string fullStats = "Player: " + game.playerLogic->getName() + "\n" +
                              "Health: " + std::to_string(game.playerLogic->getHealth()) + " / 100\n" +
                              "Moves Left: " + std::to_string(game.playerLogic->getMoves()) + "\n" +
                              "Inventory: " + game.playerLogic->getInventory().getSortedString();
        playerStatsText.setString(fullStats);

        std::stringstream log_ss;
        for(const auto& action : actionsLog) {
            log_ss << "- " << action << "\n";
        }
        recentActionsText.setString(log_ss.str());

        std::string prompt;
        switch (currentState) {
            case InteractionState::EXPLORING:
                {
                    std::vector<std::string> prompts;
                    Room* currentRoom = game.dungeonLogic->getCurrentRoom();
                    bool inSwordRoomWithSword = currentRoom && currentRoom->entity && dynamic_cast<Weapon*>(currentRoom->entity.get()) && currentRoom->name == "Chamber of the Cursed Blades";

                    if (inSwordRoomWithSword) {
                        prompts.push_back("[C] Collect Sword");
                    }
                    
                    if (game.dungeonLogic->canMoveForward()) prompts.push_back("[F] Forward");
                    if (game.dungeonLogic->canMoveBack()) prompts.push_back("[B] Backtrack");
                    if (!inSwordRoomWithSword) {
                        prompts.push_back("[Q] Quit");
                    }

                    std::stringstream ss;
                    for(size_t i = 0; i < prompts.size(); ++i) {
                        ss << prompts[i] << (i == prompts.size() - 1 ? "" : "\n");
                    }
                    prompt = ss.str();
                }
                break;
            case InteractionState::COMBAT: prompt = "[F] Fight!\n[R] Attempt to Run"; break;
            case InteractionState::CHOICE: prompt = "[1] Take Golden Key\n[2] Take Health Potion"; break;
            case InteractionState::MESSAGE:
                interactionText.setString(message);
                interactionText.setFillColor(GameConfig::OFF_WHITE_COLOR);
                Utils::wrapText(interactionText, messagePanel.getSize().x - 40);
                prompt = "[Enter] Continue";
                break;
        }
        actionPromptsText.setString(prompt);

        sf::FloatRect promptBounds = actionPromptsText.getLocalBounds();
        actionPromptsText.setOrigin(promptBounds.left + promptBounds.width, promptBounds.top);
        float panelX = uiPanel.getPosition().x;
        float panelY = uiPanel.getPosition().y;
        float panelW = uiPanel.getSize().x;
        actionPromptsText.setPosition(panelX + panelW - 30, panelY + 25);
    }

    void update(sf::Time dt, Game& gameRef) override {
        if (flashDuration > 0) {
            float elapsed = flashClock.getElapsedTime().asSeconds();
            if (elapsed < flashDuration) {
                float progress = elapsed / flashDuration;
                sf::Uint8 alpha = static_cast<sf::Uint8>(Utils::lerp(180.f, 0.f, progress));
                damageFlash.setFillColor(sf::Color(GameConfig::LIGHT_RED_FLASH.r, GameConfig::LIGHT_RED_FLASH.g, GameConfig::LIGHT_RED_FLASH.b, alpha));
                damageFlash.setOutlineColor(sf::Color(255, 255, 255, alpha));
            } else {
                flashDuration = 0;
                damageFlash.setFillColor(sf::Color::Transparent);
                damageFlash.setOutlineColor(sf::Color::Transparent);
            }
        }

        if (transitionState != GpTransitionState::NONE) {
            float t = Utils::clamp(transitionClock.getElapsedTime().asSeconds() / GameConfig::GAMEPLAY_TRANSITION_DURATION, 0.f, 1.f);
            sf::Uint8 alpha = 0;

            if (transitionState == GpTransitionState::FADING_OUT) {
                alpha = static_cast<sf::Uint8>(Utils::lerp(0.f, 255.f, t));
                if (t >= 1.0f) {
                    if(onTransitionComplete) {
                        onTransitionComplete();
                        onTransitionComplete = nullptr;
                    }
                    
                    // MODIFIED: Check for game over due to running out of moves.
                    if (game.playerLogic->getMoves() < 0) {
                        triggerGameOver("You have run out of moves.");
                        return;
                    }

                    checkRoomState();
                    transitionState = GpTransitionState::FADING_IN;
                    transitionClock.restart();
                }
            } else { // FADING_IN
                alpha = static_cast<sf::Uint8>(Utils::lerp(255.f, 0.f, t));
                if (t >= 1.0f) {
                    transitionState = GpTransitionState::NONE;
                }
            }
            transitionOverlay.setFillColor(sf::Color(0, 0, 0, alpha));
        }

        updateUI();
    }

    void draw(sf::RenderWindow& window) override {
        drawBackground(window);

        window.draw(roomNameText);
        window.draw(roomDescText);
        if(!entityDescText.getString().isEmpty()) window.draw(entityDescText);

        window.draw(uiPanel);
        window.draw(playerStatsText);
        window.draw(recentActionsTitle);
        window.draw(recentActionsText);
        window.draw(actionPromptsText);

        if (currentState == InteractionState::MESSAGE) {
            window.draw(messagePanel);
            window.draw(interactionText);
        }
        if (flashDuration > 0) {
            window.draw(damageFlash);
        }
        if (transitionState != GpTransitionState::NONE) {
            window.draw(transitionOverlay);
        }
    }
};

// =================================================================
// 4. MAIN GAME & SETUP
// =================================================================

void setupDungeon(Dungeon& dungeon) {
    auto* entrance = new Room("Dungeon Entrance", "The heavy stone door slams shut behind you. Your only way is forward.", "dungeon.png");
    auto* wizardStudy = new Room("Sanctum of Fire and Frost", "You dare enter my domain, mortal? The fire and frost bend to my will. If you wish to pass, you must defeat me first.", "wizard.png");
    auto* dragonLair = new Room("Dragon's Lair", "The air is hot and smells of sulfur. A scaly beast awakens from its slumber.", "dragon.png");
    auto* zombieCrypt = new Room("Zombie's Crypt", "Dust swirls through shafts of cold light. From the gloom, a corpse lurches forward with dead, hungry eyes.", "zombie.png");
    auto* weaponRoom = new Room("Chamber of the Cursed Blades", "Dark swords float mid-air, glowing with runes. A red sigil burns behind them, pulsing with power.", "sword.png");
    auto* choiceRoom = new Room("Room of Choice", "The hooded figure looks up from his book. 'You can only take one,' he says. 'The golden key... or the potion that gives you health'", "potion.png");
    auto* monsterDen = new Room("Giant Monster's Den", "Huge claw marks scar the walls. A hulking creature guards the path ahead.", "monster.png");
    auto* bossChamber = new Room("Final Boss Chamber", "This is it. The final guardian.", "finalboss.png");
    auto* finalDoor = new Room("The Final Door", "You see a massive, ornate door with a single large keyhole. This must be the exit.", "finaldoor.png");

    wizardStudy->entity = std::make_unique<MinionEnemy>("Wizard", 10);
    dragonLair->entity = std::make_unique<MinionEnemy>("Dragon", 15);
    zombieCrypt->entity = std::make_unique<MinionEnemy>("Zombie", 5);
    weaponRoom->entity = std::make_unique<Weapon>("Sword");
    choiceRoom->isChoiceRoom = true;
    monsterDen->entity = std::make_unique<MinionEnemy>("Giant Monster", 10);
    bossChamber->entity = std::make_unique<BossEnemy>("Final Boss", 75);
    finalDoor->isFinalDoor = true;

    dungeon.addRoom(entrance);
    dungeon.addRoom(wizardStudy);
    dungeon.addRoom(dragonLair);
    dungeon.addRoom(zombieCrypt);
    dungeon.addRoom(weaponRoom);
    dungeon.addRoom(choiceRoom);
    dungeon.addRoom(monsterDen);
    dungeon.addRoom(bossChamber);
    dungeon.addRoom(finalDoor);
}

Game::Game()
    : window(sf::VideoMode(GameConfig::WINDOW_WIDTH, GameConfig::WINDOW_HEIGHT), "Dungeon Escape"),
      mainView(sf::FloatRect(0, 0, GameConfig::WINDOW_WIDTH, GameConfig::WINDOW_HEIGHT))
{
    window.setFramerateLimit(GameConfig::FRAMERATE_LIMIT);

    ResourceManager::getFont(GameConfig::FONT_PATH_ARIBLK);
    ResourceManager::getTexture("dungeon.png");
    ResourceManager::getTexture("wizard.png");
    ResourceManager::getTexture("dragon.png");
    ResourceManager::getTexture("zombie.png");
    ResourceManager::getTexture("sword.png");
    ResourceManager::getTexture("potion.png");
    ResourceManager::getTexture("monster.png");
    ResourceManager::getTexture("finalboss.png");
    ResourceManager::getTexture("finaldoor.png");
    ResourceManager::getBackgroundFrames("menu_bg", GameConfig::MENU_BG_PATH_PREFIX, GameConfig::MENU_BG_FRAME_COUNT);

    screens[GameStateID::MENU] = std::make_unique<MenuScreen>();
    screens[GameStateID::NAME_INPUT] = std::make_unique<NameInputScreen>(playerName);

    currentStateID = GameStateID::MENU;
    screens[currentStateID]->onEnter(*this);
    handleResize(window.getSize().x, window.getSize().y);
}

void Game::startGameplay() {
    playerLogic = std::make_unique<Player>(playerName, 100, 10);
    dungeonLogic = std::make_unique<Dungeon>(*playerLogic);
    setupDungeon(*dungeonLogic);
    screens[GameStateID::GAMEPLAY] = std::make_unique<GamePlayScreen>(*this);
    changeScreen(GameStateID::GAMEPLAY);
}

void Game::run() {
    while (window.isOpen()) {
        sf::Time dt = gameClock.restart();
        if (dt.asSeconds() > (1.f/20.f)) dt = sf::seconds(1.f/60.f);
        processEvents();
        update(dt);
        render();
    }
}

void Game::processEvents() {
    sf::Event event;
    while (window.pollEvent(event)) {
        if (event.type == sf::Event::Closed) window.close();
        if (event.type == sf::Event::Resized) handleResize(event.size.width, event.size.height);

        if (currentTransition == TransitionState::NONE && screens.count(currentStateID)) {
            screens.at(currentStateID)->handleEvent(event, *this);
        }
    }
}

void Game::update(sf::Time dt) {
    if (currentTransition == TransitionState::NONE) {
        if (screens.count(currentStateID)) screens.at(currentStateID)->update(dt, *this);
    }
    handleScreenTransition(dt);
    updateScreenShake();
}

void Game::render() {
    window.clear(sf::Color::Black);
    window.setView(mainView);
    if (screens.count(currentStateID)) screens.at(currentStateID)->draw(window);

    window.setView(window.getDefaultView());
    if (currentTransition != TransitionState::NONE) window.draw(transitionRect);
    window.display();
}

void Game::triggerScreenShake(float duration, float magnitude) {
    if(isShaking) return;
    isShaking = true;
    shakeDuration = duration;
    shakeMagnitude = magnitude;
    shakeClock.restart();
}

void Game::updateScreenShake() {
    if (!isShaking) {
        mainView.setCenter(GameConfig::WINDOW_WIDTH / 2.f, GameConfig::WINDOW_HEIGHT / 2.f);
        return;
    }

    if (shakeClock.getElapsedTime().asSeconds() > shakeDuration) {
        isShaking = false;
        mainView.setCenter(GameConfig::WINDOW_WIDTH / 2.f, GameConfig::WINDOW_HEIGHT / 2.f);
    } else {
        std::uniform_real_distribution<float> dist(-shakeMagnitude, shakeMagnitude);
        float x = dist(rng);
        float y = dist(rng);
        mainView.setCenter(GameConfig::WINDOW_WIDTH / 2.f + x, GameConfig::WINDOW_HEIGHT / 2.f + y);
    }
}

void Game::changeScreen(GameStateID newStateID) {
    if (currentTransition == TransitionState::NONE && newStateID != currentStateID) {
        nextStateID = newStateID;
        currentTransition = TransitionState::FADING_OUT;
        transitionClock.restart();
    }
}

void Game::handleScreenTransition(sf::Time dt) {
    if (currentTransition == TransitionState::NONE) return;
    float t = Utils::clamp(transitionClock.getElapsedTime().asSeconds() / GameConfig::TRANSITION_DURATION, 0.f, 1.f);
    sf::Uint8 alpha;

    if (currentTransition == TransitionState::FADING_OUT) {
        alpha = static_cast<sf::Uint8>(Utils::lerp(0.f, 255.f, t));
        if (t >= 1.0f) {
            currentStateID = nextStateID;
            if (screens.count(currentStateID)) {
                screens.at(currentStateID)->onEnter(*this);
                screens.at(currentStateID)->onResize(GameConfig::WINDOW_WIDTH, GameConfig::WINDOW_HEIGHT);
            }
            currentTransition = TransitionState::FADING_IN;
            transitionClock.restart();
        }
    } else { // FADING_IN
        alpha = static_cast<sf::Uint8>(Utils::lerp(255.f, 0.f, t));
        if (t >= 1.0f) currentTransition = TransitionState::NONE;
    }
    transitionRect.setFillColor(sf::Color(0, 0, 0, alpha));
}

void Game::handleResize(unsigned int actualWidth, unsigned int actualHeight) {
    float virtualWidth = static_cast<float>(GameConfig::WINDOW_WIDTH);
    float virtualHeight = static_cast<float>(GameConfig::WINDOW_HEIGHT);
    float virtualAspectRatio = virtualWidth / virtualHeight;
    float windowAspectRatio = static_cast<float>(actualWidth) / static_cast<float>(actualHeight);

    sf::FloatRect viewport(0.f, 0.f, 1.f, 1.f);

    if (windowAspectRatio > virtualAspectRatio) { // Letterbox (wider window)
        viewport.width = virtualAspectRatio / windowAspectRatio;
        viewport.left = (1.f - viewport.width) / 2.f;
    } else { // Pillarbox (taller window)
        viewport.height = windowAspectRatio / virtualAspectRatio;
        viewport.top = (1.f - viewport.height) / 2.f;
    }

    mainView.setViewport(viewport);
    mainView.setSize(virtualWidth, virtualHeight);
    mainView.setCenter(virtualWidth / 2.f, virtualHeight / 2.f);

    transitionRect.setSize({(float)actualWidth, (float)actualHeight});

    if (screens.count(currentStateID)) {
        screens.at(currentStateID)->onResize(GameConfig::WINDOW_WIDTH, GameConfig::WINDOW_HEIGHT);
    }
}


int main() {
    try {
        sf::Music backgroundMusic;
    if (!backgroundMusic.openFromFile("assets/dungeon_music.ogg")) {
        return -1;
    }

    backgroundMusic.setLoop(true);
    backgroundMusic.setVolume(50);
    backgroundMusic.play();
        Game game;
        game.run();
    } catch (const std::exception& e) {
        std::cerr << "Critical Error: " << e.what() << std::endl;
    #ifdef _WIN32
        MessageBoxA(NULL, e.what(), "Critical Error", MB_OK | MB_ICONERROR);
    #endif
        return 1;
    }
    return 0;
}