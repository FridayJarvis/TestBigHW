#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <string>
#include <map>
#include <vector>
#include <iostream>
#include <fstream>
#include <optional>
#include <algorithm>

// --- Перечисления и структуры предметов ---
enum class ToolType { Hands, Hatchet, Pickaxe, Hummer, Shovel, None };
enum class LootType { Wood, Gold, Diamond, MoonDust, None };

struct Tool
{
    ToolType type;
    int durability;
    std::wstring name;
    std::string textureKey;
    int price;
};

struct Loot
{
    LootType type;
    std::wstring name;
    std::string textureKey;
    int price;
};

// --- Менеджер ассетов ---
class AssetManager
{
public:
    AssetManager(const AssetManager&) = delete;
    AssetManager& operator=(const AssetManager&) = delete;

    static AssetManager& getInstance()
    {
        static AssetManager instance;
        return instance;
    }

    void loadAllAsset()
    {
        loadTexture("spritesheet",  "assets/spritesheet.png");
        loadTexture("inventory",    "assets/textureOfInventory.png");
        loadTexture("hidden",       "assets/hidden-ground.png");
        loadTexture("opened",       "assets/opened-ground.png");

        loadTexture("hammer",       "assets/tools/hammer.png");
        loadTexture("hatchet",          "assets/tools/hatchet.png");
        loadTexture("pickaxe",      "assets/tools/pickaxe.png");
        loadTexture("shovel",       "assets/tools/shovel.png");

        loadTexture("diamond",      "assets/loot/diamond.png");
        loadTexture("gold",         "assets/loot/gold.png");
        loadTexture("moondust",     "assets/loot/moon-dust.png");
        loadTexture("wood",         "assets/loot/wood.png");

        loadFont("main", "assets/segoescb.ttf");
    }

    const sf::Font& getFont(const std::string& name) const
    {
        auto it = m_fonts.find(name);
        if (it == m_fonts.end())
            throw std::runtime_error("Font not found: " + name);

        return *it->second;
    }

    const sf::Texture& getTexture(const std::string& name) const
    {
        auto it = m_textures.find(name);
        if (it == m_textures.end())
            throw std::runtime_error("Texture not found: " + name);

        return *it->second;
    }

    sf::Sprite getSpriteFromSheet(const sf::IntRect& rect) const
    {
        return sf::Sprite(getTexture("spritesheet"), rect);
    }



private:
    std::unordered_map<std::string, std::unique_ptr<sf::Texture>> m_textures;
    std::unordered_map<std::string, std::unique_ptr<sf::Font>> m_fonts;

    AssetManager() = default;

    void loadTexture(const std::string& name, const std::string& filename)
    {
        auto texture = std::make_unique<sf::Texture>();

        if (!texture->loadFromFile(filename))
            throw std::runtime_error("Failed to load texture: " + filename);

        m_textures[name] = std::move(texture);
    }

    void loadFont(const std::string& name, const std::string& filename)
    {
        auto font = std::make_unique<sf::Font>();
        if (!font->openFromFile(filename))
            throw std::runtime_error("Failed to load font: " + filename);
        m_fonts[name] = std::move(font);
    }
};

// --- Инвентарь ---
class Inventory
{
public:
    static const int MAX_CAPACITY = 32;
    std::vector<Tool> tools;
    std::vector<Loot> loots;


    int getUsedCapacity() const
    {
        return static_cast<int>(tools.size() + loots.size());
    }
    bool addTool(const Tool& tool)
    { 
		if (getUsedCapacity() >= MAX_CAPACITY) 
            return false;

        tools.push_back(tool);
		return true;
    }
    bool addLoot(const Loot& loot)
    {
        if (getUsedCapacity() >= MAX_CAPACITY)
			return false;
        loots.push_back(loot);
        return true;
    }
    bool hasTool(ToolType type) const
    {
        return std::any_of(tools.begin(), tools.end(), [type](const Tool& t) { return t.type == type; });
    }
    Tool* getTool(ToolType type)
    {
        for (auto& t : tools) if (t.type == type) return &t;
        return nullptr;
    }
    int getLootCount(LootType type) const
    {
        return std::count_if(loots.begin(), loots.end(), [type](const Loot& l) { return l.type == type; });
    }
    void removeLoot(LootType type, int count)
    {
        int removed = 0;
        for (auto it = loots.begin(); it != loots.end() && removed < count; )
        {
            if (it->type == type)
            {
                it = loots.erase(it);
                ++removed;
            }
            else ++it;
        }
    }
    void show(sf::RenderWindow& window)
    {
        float disiredWidthInventory = 1920.f;

        sf::Sprite inventoryBackground(AssetManager::getInstance().getTexture("inventory"));
        float scaleInventory = disiredWidthInventory / inventoryBackground.getTexture().getSize().x;
        inventoryBackground.setScale({ scaleInventory, scaleInventory });
        window.draw(inventoryBackground);

        float disiredItemSize = 236.f;
        int x = 0, y = 2;
        int iconsInRow = 8;
        int total = tools.size() + loots.size();

        for (int i = 0; i < total; ++i)
        {
            if (i % iconsInRow == 0 && i > 0)
            {
                x = 0;
                y += disiredItemSize + 2;
            }
            x += 2;

            std::string textureKey;
            if (i < tools.size())
                textureKey = tools[i].textureKey;
            else
                textureKey = loots[i - tools.size()].textureKey;

            sf::Sprite sprite(AssetManager::getInstance().getTexture(textureKey));
            float scaleLootX = disiredItemSize / sprite.getTexture().getSize().x;
            float scaleLootY = disiredItemSize / sprite.getTexture().getSize().y;
            sprite.setScale({ scaleLootX, scaleLootY });
            sprite.setPosition({ (float)x, (float)y });

            window.draw(sprite);
            x += 2 + disiredItemSize;
        }
    }
};

class PlayerAnimator
{
public:
    // Типы анимаций (индекс блока по 4 строки)
    enum AnimType
    {
        WalkHammer, UseHammer,
        WalkHatchet, UseHatchet,
        WalkPickaxe, UsePickaxe,
        WalkNone, UseHands,
        UseShovel, WalkShovel
    };

    // Количество кадров для каждого типа анимации
    static constexpr int animFrames[10] = { 9, 6, 9, 6, 9, 9, 9, 6, 8, 9 };

    int animType = WalkNone;
    int direction = 3; // 0-вверх, 1-влево, 2-вправо, 3-вниз
    int frame = 0;
    float timer = 0.f;
    float frameTime = 0.1f;

    void setAnim(ToolType tool, bool use, int dir)
    {
        direction = dir;
        switch (tool)
        {
            case ToolType::Hummer: animType = use ? UseHammer : WalkHammer; break;
            case ToolType::Hatchet: animType = use ? UseHatchet : WalkHatchet; break;
            case ToolType::Pickaxe: animType = use ? UsePickaxe : WalkPickaxe; break;
            case ToolType::Shovel: animType = use ? UseShovel : WalkShovel; break;
            case ToolType::Hands: animType = use ? UseHands : WalkNone; break;
            default: animType = WalkNone; break;
        }
    }

    void update(float dt, bool moving)
    {
        timer += dt;
        int maxFrame = animFrames[animType];
        if (timer >= frameTime)
        {
            timer = 0.f;
            frame++;
            if (frame >= maxFrame) frame = 0;
        }
        if (!moving) frame = 0;
    }

    sf::IntRect getRect() const
    {
        // Каждые 4 строки — один тип анимации, direction — строка внутри блока
        int y = animType * 4 + direction;
        return sf::IntRect({ frame * 64 , y * 64 }, { 64, 64 });
    }
};

// --- Игрок ---
class Player
{
public:
    int money = 1000;
    int food = 10;
    Inventory inventory;
    ToolType toolInHand = ToolType::None;

    void buyFood(int price, int amount)
    {
        if (money >= price) { money -= price; food += amount; }
    }
    bool spendMoney(int amount)
    {
        if (money >= amount) { money -= amount; return true; }
        return false;
    }
    void addLoot(const Loot& loot) { inventory.addLoot(loot); }
    void addTool(const Tool& tool) { inventory.addTool(tool); }
    bool hasTool(ToolType type) const { return inventory.hasTool(type); }
    void useFood() { if (food > 0) --food; }
    bool isAlive() const { return money > 0; }
};

enum class CellState { Hidden, Opened, Loot };

struct MapCell
{
    CellState state = CellState::Hidden;
    LootType loot = LootType::None;
    bool lootPicked = false;
};

// --- Абстрактная экспедиция ---
class Excavation
{
protected:
    std::wstring name;
    int cost;
    std::vector<Loot> possibleLoot;
    std::map<ToolType, std::pair<int, int>> toolRules; // {шанс потерять лут, штраф к прочности}

    // --- Новое ---
    static constexpr int MAP_SIZE = 10;
    MapCell map[MAP_SIZE][MAP_SIZE];
    int playerX = MAP_SIZE / 2;
    int playerY = MAP_SIZE / 2;

public:
    Excavation(const std::wstring& n, int c) : name(n), cost(c)
    {
        // Генерация карты: всё скрыто, в центре игрок, в нескольких ячейках лут
        for (int y = 0; y < MAP_SIZE; ++y)
            for (int x = 0; x < MAP_SIZE; ++x)
                map[y][x] = MapCell{};
        // Пример: разместить один лут в центре
        map[MAP_SIZE / 2][MAP_SIZE / 2].state = CellState::Loot;
        map[MAP_SIZE / 2][MAP_SIZE / 2].loot = possibleLoot.empty() ? LootType::None : possibleLoot[0].type;
    }

    // --- Методы для работы с картой ---
    void movePlayer(int dx, int dy)
    {
        int nx = std::clamp(playerX + dx, 0, MAP_SIZE - 1);
        int ny = std::clamp(playerY + dy, 0, MAP_SIZE - 1);
        playerX = nx;
        playerY = ny;
        auto& cell = map[playerY][playerX];
        if (cell.state == CellState::Hidden)
            cell.state = CellState::Opened;
    }

    void pickLoot(Player& player)
    {
        auto& cell = map[playerY][playerX];
        if (cell.state == CellState::Loot && !cell.lootPicked)
        {
            player.addLoot({ cell.loot, L"", "", 0 });
            cell.lootPicked = true;
            cell.state = CellState::Opened;
        }
    }

    // --- Остальное без изменений ---
    virtual ~Excavation() = default;
    virtual LootType dig(Player& player, std::wstring& resultMsg) = 0;
    int getCost() const { return cost; }
    const std::wstring& getName() const { return name; }
    const std::vector<Loot>& getPossibleLoot() const { return possibleLoot; }
    // Для отрисовки:
    const MapCell(&getMap() const)[MAP_SIZE][MAP_SIZE]{ return map; }
    int getPlayerX() const { return playerX; }
    int getPlayerY() const { return playerY; }
};

// --- Конкретные экспедиции ---
class ForestExpedition : public Excavation
{
public:
    ForestExpedition() : Excavation(L"Лесная экспедиция", 100)
    {
        possibleLoot.push_back({ LootType::Wood, L"Дерево", "wood", 150 });
        toolRules[ToolType::Hatchet] = { 0, 10 };
        toolRules[ToolType::Hummer] = { 70, 25 };
    }
    LootType dig(Player& player, std::wstring& resultMsg) override
    {
        player.useFood();
        Tool* tool = player.inventory.getTool(player.toolInHand);
        if (!tool || (tool->type != ToolType::Hatchet && tool->type != ToolType::Hummer))
        {
            resultMsg = L"Инструмент не подходит!";
            return LootType::None;
        }
        int loseChance = toolRules[tool->type].first;
        int durabilityPenalty = toolRules[tool->type].second;
        if (rand() % 100 < loseChance)
        {
            resultMsg = L"Лут потерян!";
            tool->durability -= durabilityPenalty;
            return LootType::None;
        }
        tool->durability -= durabilityPenalty;
        player.addLoot(possibleLoot[0]);
        resultMsg = L"Вы нашли: " + possibleLoot[0].name;
        return possibleLoot[0].type;
    }
};

class EgyptExpedition : public Excavation
{
public:
    EgyptExpedition() : Excavation(L"Египетская экспедиция", 600)
    {
        possibleLoot.push_back({ LootType::Gold, L"Золото", "gold", 250 });
        toolRules[ToolType::Hummer] = { 0, 10 };
        toolRules[ToolType::Hatchet] = { 25, 25 };
        toolRules[ToolType::Pickaxe] = { 50, 25 };
    }
    LootType dig(Player& player, std::wstring& resultMsg) override
    {
        player.useFood();
        Tool* tool = player.inventory.getTool(player.toolInHand);
        if (!tool || (tool->type != ToolType::Hummer && tool->type != ToolType::Hatchet && tool->type != ToolType::Pickaxe))
        {
            resultMsg = L"Инструмент не подходит!";
            return LootType::None;
        }
        int loseChance = toolRules[tool->type].first;
        int durabilityPenalty = toolRules[tool->type].second;
        if (rand() % 100 < loseChance)
        {
            resultMsg = L"Лут потерян!";
            tool->durability -= durabilityPenalty;
            return LootType::None;
        }
        tool->durability -= durabilityPenalty;
        player.addLoot(possibleLoot[0]);
        resultMsg = L"Вы нашли: " + possibleLoot[0].name;
        return possibleLoot[0].type;
    }
};

class AfricaExpedition : public Excavation
{
public:
    AfricaExpedition() : Excavation(L"Африканская экспедиция", 1000)
    {
        possibleLoot.push_back({ LootType::Diamond, L"Алмазы", "diamond", 750 });
        toolRules[ToolType::Pickaxe] = { 0, 10 };
        toolRules[ToolType::Shovel] = { 70, 25 };
    }
    LootType dig(Player& player, std::wstring& resultMsg) override
    {
        player.useFood();
        Tool* tool = player.inventory.getTool(player.toolInHand);
        if (!tool || (tool->type != ToolType::Pickaxe && tool->type != ToolType::Shovel))
        {
            resultMsg = L"Инструмент не подходит!";
            return LootType::None;
        }
        int loseChance = toolRules[tool->type].first;
        int durabilityPenalty = toolRules[tool->type].second;
        if (rand() % 100 < loseChance)
        {
            resultMsg = L"Лут потерян!";
            tool->durability -= durabilityPenalty;
            return LootType::None;
        }
        tool->durability -= durabilityPenalty;
        player.addLoot(possibleLoot[0]);
        resultMsg = L"Вы нашли: " + possibleLoot[0].name;
        return possibleLoot[0].type;
    }
};

class MoonExpedition : public Excavation
{
public:
    MoonExpedition() : Excavation(L"Лунная экспедиция", 3000)
    {
        possibleLoot.push_back({ LootType::MoonDust, L"Лунная пыль", "moondust", 1300 });
        toolRules[ToolType::Hands] = { 25, 0 }; // штраф к еде -2 реализовать отдельно
        toolRules[ToolType::Shovel] = { 0, 10 };
    }
    LootType dig(Player& player, std::wstring& resultMsg) override
    {
        if (player.toolInHand == ToolType::Hands) player.food -= 2;
        else player.useFood();
        Tool* tool = player.inventory.getTool(player.toolInHand);
        if (player.toolInHand != ToolType::Hands && (!tool || tool->type != ToolType::Shovel))
        {
            resultMsg = L"Инструмент не подходит!";
            return LootType::None;
        }
        int loseChance = toolRules[player.toolInHand].first;
        int durabilityPenalty = toolRules[player.toolInHand].second;
        if (rand() % 100 < loseChance)
        {
            resultMsg = L"Лут потерян!";
            if (tool) tool->durability -= durabilityPenalty;
            return LootType::None;
        }
        if (tool) tool->durability -= durabilityPenalty;
        player.addLoot(possibleLoot[0]);
        resultMsg = L"Вы нашли: " + possibleLoot[0].name;
        return possibleLoot[0].type;
    }
};

// --- Магазин ---
class Store
{
public:
    std::vector<Tool> toolsForSale = {
        {ToolType::Hatchet,     100,    L"Топор",   "hatchet",      200},
        {ToolType::Pickaxe, 100,    L"Кирка",   "pickaxe",  300},
        {ToolType::Hummer,  100,    L"Молот",   "hummer",   250},
        {ToolType::Shovel,  100,    L"Лопата",  "shovel",   150}
    };
    int foodPrice = 50;
    int foodAmount = 5;

    void buyTool(Player& player, ToolType type)
    {
        for (const auto& t : toolsForSale)
        {
            if (t.type == type && player.money >= t.price)
            {
                player.money -= t.price;
                player.addTool(t);
            }
        }
    }
    void buyFood(Player& player)
    {
        if (player.money >= foodPrice)
        {
            player.money -= foodPrice;
            player.food += foodAmount;
        }
    }
    void sellLoot(Player& player, LootType type)
    {
        int count = player.inventory.getLootCount(type);
        if (count > 0)
        {
            int price = 0;
            for (const auto& l : player.inventory.loots)
            {
                if (l.type == type)
                {
                    price = l.price;
                    break;
                }
            }
            player.money += price * count;
            player.inventory.removeLoot(type, count);
        }
    }
};

// --- Музей ---
class Museum
{
public:
    void show(const Player& player, sf::RenderWindow& window)
    {
        float disiredWidthInventory = 1920.f;
        sf::Sprite inventoryBackground(AssetManager::getInstance().getTexture("inventory"));
        float scaleInventory = disiredWidthInventory / inventoryBackground.getTexture().getSize().x;
        inventoryBackground.setScale({ scaleInventory, scaleInventory });
        window.draw(inventoryBackground);

        float disiredItemSize = 236.f;
        int x = 0, y = 2;
        int iconsInRow = 8;
        int margin = 2;

        // --- Лут ---
        for (int i = 0; i < 4; ++i) // 4 типа лута
        {
            LootType type = static_cast<LootType>(i);
            std::string textureKey;
            std::wstring name;
            int price = 0;
            switch (type)
            {
                case LootType::Wood:     textureKey = "wood";     name = L"Дерево";     price = 150; break;
                case LootType::Gold:     textureKey = "gold";     name = L"Золото";     price = 250; break;
                case LootType::Diamond:  textureKey = "diamond";  name = L"Алмазы";     price = 750; break;
                case LootType::MoonDust: textureKey = "moondust"; name = L"Лунная пыль"; price = 1300; break;
                default: continue;
            }
            int count = player.inventory.getLootCount(type);

            int row = i / iconsInRow;
            int col = i % iconsInRow;
            float drawX = margin + col * (disiredItemSize + margin);
            float drawY = margin + row * (disiredItemSize + 60 + margin); // 60px под текст

            sf::Sprite sprite(AssetManager::getInstance().getTexture(textureKey));
            float scaleX = disiredItemSize / sprite.getTexture().getSize().x;
            float scaleY = disiredItemSize / sprite.getTexture().getSize().y;
            sprite.setScale({ scaleX, scaleY });
            sprite.setPosition({ drawX, drawY });
            window.draw(sprite);

            // Количество
            sf::Text text(AssetManager::getInstance().getFont("main"));
            text.setCharacterSize(36);
            text.setFillColor(sf::Color::Black);
            text.setString(L"x" + std::to_wstring(count));
            text.setPosition({ drawX, drawY + disiredItemSize + 5 });
            window.draw(text);
        }

        // --- Инструменты ---
        for (int i = 0; i < 5; ++i) // 5 типов инструментов
        {
            ToolType type = static_cast<ToolType>(i);
            std::string textureKey;
            std::wstring name;
            switch (type)
            {
                case ToolType::Hatchet:  textureKey = "hatchet";  name = L"Топор";   break;
                case ToolType::Pickaxe:  textureKey = "pickaxe";  name = L"Кирка";   break;
                case ToolType::Hummer:   textureKey = "hammer";   name = L"Молот";   break;
                case ToolType::Shovel:   textureKey = "shovel";   name = L"Лопата";  break;
                case ToolType::Hands:    continue; // не отображаем "руки"
                default: continue;
            }
            int count = 0;
            for (const auto& t : player.inventory.tools)
                if (t.type == type) ++count;

            int row = (i + 4) / iconsInRow; // после лута
            int col = (i + 4) % iconsInRow;
            float drawX = margin + col * (disiredItemSize + margin);
            float drawY = margin + row * (disiredItemSize + 60 + margin);

            sf::Sprite sprite(AssetManager::getInstance().getTexture(textureKey));
            float scaleX = disiredItemSize / sprite.getTexture().getSize().x;
            float scaleY = disiredItemSize / sprite.getTexture().getSize().y;
            sprite.setScale({ scaleX, scaleY });
            sprite.setPosition({ drawX, drawY });
            window.draw(sprite);

            // Количество
            sf::Text text(AssetManager::getInstance().getFont("main"));
            text.setCharacterSize(36);
            text.setFillColor(sf::Color::Black);
            text.setString(L"x" + std::to_wstring(count));
            text.setPosition({ drawX, drawY + disiredItemSize + 5 });
            window.draw(text);
        }
    }
};

// --- Сохранение/загрузка ---
class SaveManager
{
public:
    void save(const Player& player)
    {
        std::wofstream f(L"save/save.txt");
        f << player.money << " " << player.food << "\n";
        for (const auto& t : player.inventory.tools)
            f << L"T " << int(t.type) << " " << t.durability << "\n";
        for (const auto& l : player.inventory.loots)
            f << L"L " << int(l.type) << " " << l.name << " " << l.textureKey.c_str() << " " << l.price << "\n";
    }
    void load(Player& player)
    {
        std::wifstream f(L"save/save.txt");
        if (!f) return;
        player.inventory.tools.clear();
        player.inventory.loots.clear();
        f >> player.money >> player.food;
        std::wstring type;
        while (f >> type)
        {
            if (type == L"T")
            {
                int ttype, dur;
                f >> ttype >> dur;
                player.addTool({ ToolType(ttype), dur, L"", "", 0 });
            }
            else if (type == L"L")
            {
                int ltype, cnt;
                f >> ltype >> cnt;
                player.addLoot({ LootType(ltype), L"", "", 0 });
            }
        }
    }
};

// --- Сцены игры ---
enum class GameScene { Base, ExpeditionChoice, Expedition, Store, Museum, Inventory, Save, Exit };

// --- Главный класс игры ---
class Game
{
    sf::RenderWindow window = sf::RenderWindow(sf::VideoMode({ 1920, 1080 }), L"Симулятор палеонтолога");
    Player player;
    PlayerAnimator animator;
    Store store;
    Museum museum;
    SaveManager saveManager;
    GameScene scene = GameScene::Base;
    std::unique_ptr<Excavation> currentExpedition;
    std::wstring lastExpeditionMsg;

public:
    Game()
    {
		AssetManager::getInstance().loadAllAsset();

        player.addTool({ ToolType::Hatchet, 100, L"Топор", "hatchet", 0 });
    }

    void run()
    {
        while (window.isOpen() && scene != GameScene::Exit)
        {
            while (std::optional event = window.pollEvent())
            {
                if (event->is<sf::Event::Closed>())
                    window.close();
                if (auto pressed = event->getIf<sf::Event::KeyPressed>())
                    handleInput(pressed->scancode);

            }
            window.clear(sf::Color::White);
            draw();
            window.display();
        }
    }

    void handleInput(sf::Keyboard::Scancode key)
    {
        switch (scene)
        {
            case GameScene::Base:
                if (key == sf::Keyboard::Scancode::Num1) scene = GameScene::ExpeditionChoice;
                if (key == sf::Keyboard::Scancode::Num2) scene = GameScene::Store;
                if (key == sf::Keyboard::Scancode::Num3) scene = GameScene::Museum;
                if (key == sf::Keyboard::Scancode::Num4) { saveManager.save(player); }
                if (key == sf::Keyboard::Scancode::Escape) scene = GameScene::Exit;
                break;
            case GameScene::ExpeditionChoice:
                if (key == sf::Keyboard::Scancode::Num1 && player.spendMoney(100))
                {
                    currentExpedition = std::make_unique<ForestExpedition>();
                    scene = GameScene::Expedition;
                }
                if (key == sf::Keyboard::Scancode::Num2 && player.spendMoney(600))
                {
                    currentExpedition = std::make_unique<EgyptExpedition>();
                    scene = GameScene::Expedition;
                }
                if (key == sf::Keyboard::Scancode::Num3 && player.spendMoney(1000))
                {
                    currentExpedition = std::make_unique<AfricaExpedition>();
                    scene = GameScene::Expedition;
                }
                if (key == sf::Keyboard::Scancode::Num4 && player.spendMoney(3000))
                {
                    currentExpedition = std::make_unique<MoonExpedition>();
                    scene = GameScene::Expedition;
                }
                if (key == sf::Keyboard::Scancode::Escape) scene = GameScene::Base;
                break;
            case GameScene::Expedition:
            {
                if (key == sf::Keyboard::Scancode::Num1) player.toolInHand = ToolType::Hands;
                if (key == sf::Keyboard::Scancode::Num2) player.toolInHand = ToolType::Hatchet;
                if (key == sf::Keyboard::Scancode::Num3) player.toolInHand = ToolType::Pickaxe;
                if (key == sf::Keyboard::Scancode::Num4) player.toolInHand = ToolType::Hummer;
                if (key == sf::Keyboard::Scancode::Num5) player.toolInHand = ToolType::Shovel;
                if (key == sf::Keyboard::Scancode::I) scene = GameScene::Inventory;
                if (key == sf::Keyboard::Scancode::Space && currentExpedition)
                {
                    lastExpeditionMsg.clear();
                    currentExpedition->dig(player, lastExpeditionMsg);
                }
                if (key == sf::Keyboard::Scancode::Escape) scene = GameScene::Base;
                int dx = 0, dy = 0;
                int dir = animator.direction; // по умолчанию текущее направление
                float animDt = 0.1f;
                if (key == sf::Keyboard::Scancode::W) { dy = -1; dir = 0; }
                if (key == sf::Keyboard::Scancode::A) { dx = -1; dir = 1; }
                if (key == sf::Keyboard::Scancode::D) { dx = 1;  dir = 2; }
                if (key == sf::Keyboard::Scancode::S) { dy = 1;  dir = 3; }

                if ((dx != 0 || dy != 0) && currentExpedition)
                {
                    currentExpedition->movePlayer(dx, dy);
                    animator.setAnim(player.toolInHand, false, dir);
                    animator.update(animDt, true);
                }
                else
                {
                    animator.setAnim(player.toolInHand, false, dir);
                    animator.update(animDt, false);
                }

                // Остальные действия (копать, подбирать лут и т.д.)
                if (key == sf::Keyboard::Scancode::F && currentExpedition)
                    currentExpedition->pickLoot(player);

                if (key == sf::Keyboard::Scancode::Space && currentExpedition)
                {
                    lastExpeditionMsg.clear();
                    currentExpedition->dig(player, lastExpeditionMsg);
                    // Можно добавить animator.setAnim(..., true, dir) для анимации использования инструмента
                }
                if (key == sf::Keyboard::Scancode::Escape) scene = GameScene::Base;

                break;
            }
            case GameScene::Store:
                if (key == sf::Keyboard::Scancode::Num1) store.buyTool(player, ToolType::Hatchet);
                if (key == sf::Keyboard::Scancode::Num2) store.buyTool(player, ToolType::Pickaxe);
                if (key == sf::Keyboard::Scancode::Num3) store.buyTool(player, ToolType::Hummer);
                if (key == sf::Keyboard::Scancode::Num4) store.buyTool(player, ToolType::Shovel);
                if (key == sf::Keyboard::Scancode::Num5) store.buyFood(player);
                if (key == sf::Keyboard::Scancode::Num6) store.sellLoot(player, LootType::Wood);
                if (key == sf::Keyboard::Scancode::Num7) store.sellLoot(player, LootType::Gold);
                if (key == sf::Keyboard::Scancode::Num8) store.sellLoot(player, LootType::Diamond);
                if (key == sf::Keyboard::Scancode::Num9) store.sellLoot(player, LootType::MoonDust);
                if (key == sf::Keyboard::Scancode::Escape) scene = GameScene::Base;
                break;
            case GameScene::Museum:
                if (key == sf::Keyboard::Scancode::Escape) scene = GameScene::Base;
                break;
			case GameScene::Inventory:
                if (key == sf::Keyboard::Scancode::Escape) scene = GameScene::Expedition;
                break;
            default: break;
        }
    }

    void draw()
    {
        sf::Text text(AssetManager::getInstance().getFont("main"), L"", 24u);
        text.setFillColor(sf::Color::Black);

        if (scene == GameScene::Base)
        {
            text.setString(
                L"База\n"
                L"Деньги: " + std::to_wstring(player.money) + L" Еда: " + std::to_wstring(player.food) + "\n"
                L"1 - Экспедиция\n"
                L"2 - Магазин\n"
                L"3 - Музей\n"
                L"4 - Сохранить\n"
                L"Esc - Выйти"
            );
            window.draw(text);
        }
        else if (scene == GameScene::ExpeditionChoice)
        {
            text.setString(
                L"Выберите экспедицию:\n"
                L"1 - Лесная (100р)\n"
                L"2 - Египетская (600р)\n"
                L"3 - Африканская (1000р)\n"
                L"4 - Лунная (3000р)\n"
                L"Esc - Назад"
            );
            window.draw(text);
        }
        else if (scene == GameScene::Expedition)
        {
            text.setString(
                L"Экспедиция: " + currentExpedition->getName() + "\n"
                L"Инструмент в руке: " + std::to_wstring(int(player.toolInHand) + 1) + "\n"
                L"1 - Руки, 2 - Топор, 3 - Кирка, 4 - Молот, 5 - Лопата\n"
                L"Пробел - Копать\n"
                L"Esc - Назад\n" +
                lastExpeditionMsg
            );

            window.draw(text);
            const int cellSize = 64;
            const int mapPx = cellSize * 10;
            const int offsetX = (1920 - mapPx) / 2;
            const int offsetY = (1080 - mapPx) / 2;
            const auto& map = currentExpedition->getMap();
            for (int y = 0; y < 10; ++y)
            {
                for (int x = 0; x < 10; ++x)
                {
                    const auto& cell = map[y][x];
                    sf::Sprite sprite(AssetManager::getInstance().getTexture("hidden"));
                    if (cell.state == CellState::Hidden)
                        sprite.setTexture(AssetManager::getInstance().getTexture("hidden"));
                    else if (cell.state == CellState::Opened)
                        sprite.setTexture(AssetManager::getInstance().getTexture("opened"));
                    else if (cell.state == CellState::Loot && !cell.lootPicked)
                    {
                        std::string lootKey = "wood";
                        if (cell.loot == LootType::Gold) lootKey = "gold";
                        if (cell.loot == LootType::Diamond) lootKey = "diamond";
                        if (cell.loot == LootType::MoonDust) lootKey = "moondust";
                        sprite.setTexture(AssetManager::getInstance().getTexture(lootKey));
                    }
                    sprite.setPosition({ (float) offsetX + x * cellSize, (float) offsetY + y * cellSize });
                    sprite.setScale({
                        (float)cellSize / sprite.getTexture().getSize().x,
                        (float)cellSize / sprite.getTexture().getSize().y
                    });
                    window.draw(sprite);
                }
            }
            // Игрок
            sf::Sprite playerSprite = AssetManager::getInstance().getSpriteFromSheet(animator.getRect());
            playerSprite.setPosition({
                (float) offsetX + currentExpedition->getPlayerX() * cellSize,
                (float) offsetY + currentExpedition->getPlayerY() * cellSize
             });
            window.draw(playerSprite);
        }
        else if (scene == GameScene::Store)
        {
            text.setString(
                L"Магазин\n"
                L"1 - Купить топор (200р)\n"
                L"2 - Купить кирку (300р)\n"
                L"3 - Купить молот (250р)\n"
                L"4 - Купить лопату (150р)\n"
                L"5 - Купить еду (50р за 5)\n"
                L"6 - Продать дерево\n"
                L"7 - Продать золото\n"
                L"8 - Продать алмазы\n"
                L"9 - Продать лунную пыль\n"
                L"Esc - Назад"
            );
            window.draw(text);
        }
        else if (scene == GameScene::Museum)
        {
            text.setString(L"Музей (Esc - Назад)");
            window.draw(text);
            museum.show(player, window); // Новый метод для музея
        }
        else if (scene == GameScene::Inventory)
        {
            text.setString(L"Инвентарь (Esc - Назад)");
            window.draw(text);
            player.inventory.show(window); // Метод из Inventory
        }
    }
};

int main()
{
    Game game;
    game.run();
    return 0;
}