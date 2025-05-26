#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <string>
#include <map>
#include <vector>
#include <iostream>
#include <fstream>
#include <optional>
#include <algorithm>

// --- ������������ � ��������� ��������� ---
enum class ToolType { Hands, Axe, Pickaxe, Hummer, Shovel, None };
enum class LootType { Wood, Gold, Diamond, MoonDust, None };

struct Tool
{
    ToolType type;
    int durability;
    std::wstring name;
    std::wstring textureKey;
    int price;
};

struct Loot
{
    LootType type;
    int count;
    std::wstring name;
    std::wstring textureKey;
    int price;
};

// --- �������� ������� ---
class AssetManager
{
    std::map<std::wstring, sf::Texture> textures;
    std::map<std::wstring, sf::Font> fonts;
public:
    void loadAll()
    {
        struct { std::wstring key, path; } texs[] = {
            {L"wood",       L"assets/loot/wood.png"},
            {L"gold",       L"assets/loot/gold.png"},
            {L"diamond",    L"assets/loot/diamond.png"},
            {L"moondust",   L"assets/loot/moondust.png"},
            {L"axe",        L"assets/tools/axe.png"},
            {L"pickaxe",    L"assets/tools/pickaxe.png"},
            {L"hummer",     L"assets/tools/hummer.png"},
            {L"shovel",     L"assets/tools/shovel.png"},
            {L"hands",      L"assets/tools/hands.png"},
            {L"inventory",  L"assets/inventory.png"},
            {L"base",       L"assets/base.png"},
            {L"store",      L"assets/store.png"},
            {L"museum",     L"assets/museum.png"}
        };
        for (auto& t : texs)
        {
            sf::Texture tex;
            if (!tex.loadFromFile(t.path))
                std::wcerr << L"�� ������� ��������� " << t.path << std::endl;
            textures[t.key] = std::move(tex);
        }
        sf::Font font;
        if (!font.openFromFile(L"assets/segoescb.ttf"))
            std::wcerr << L"�� ������� ��������� �����" << std::endl;
        fonts[L"main"] = std::move(font);
    }
    const sf::Texture& getTexture(const std::wstring& key) const { return textures.at(key); }
    const sf::Font& getFont(const std::wstring& key) const { return fonts.at(key); }
};

// --- ��������� ---
class Inventory
{
public:
    std::vector<Tool> tools;
    std::vector<Loot> loots;

    void addTool(const Tool& tool) { tools.push_back(tool); }
    void addLoot(const Loot& loot)
    {
        for (auto& l : loots)
        {
            if (l.type == loot.type) { l.count += loot.count; return; }
        }
        loots.push_back(loot);
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
        for (const auto& l : loots) if (l.type == type) return l.count;
        return 0;
    }
    void removeLoot(LootType type, int count)
    {
        for (auto& l : loots)
        {
            if (l.type == type)
            {
                l.count -= count;
                if (l.count <= 0) l.count = 0;
                return;
            }
        }
    }
};

// --- ����� ---
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

// --- ����������� ���������� ---
class Excavation
{
protected:
    std::wstring name;
    int cost;
    std::vector<Loot> possibleLoot;
    std::map<ToolType, std::pair<int, int>> toolRules; // {���� �������� ���, ����� � ���������}
public:
    Excavation(const std::wstring& n, int c) : name(n), cost(c) {}
    virtual ~Excavation() = default;
    virtual LootType dig(Player& player, AssetManager& assets, std::wstring& resultMsg) = 0;
    int getCost() const { return cost; }
    const std::wstring& getName() const { return name; }
    const std::vector<Loot>& getPossibleLoot() const { return possibleLoot; }
};

// --- ���������� ���������� ---
class ForestExpedition : public Excavation
{
public:
    ForestExpedition() : Excavation(L"������ ����������", 100)
    {
        possibleLoot.push_back({ LootType::Wood, 1, L"������", L"wood", 150 });
        toolRules[ToolType::Axe] = { 0, 10 };
        toolRules[ToolType::Hummer] = { 70, 25 };
    }
    LootType dig(Player& player, AssetManager& assets, std::wstring& resultMsg) override
    {
        player.useFood();
        Tool* tool = player.inventory.getTool(player.toolInHand);
        if (!tool || (tool->type != ToolType::Axe && tool->type != ToolType::Hummer))
        {
            resultMsg = L"���������� �� ��������!";
            return LootType::None;
        }
        int loseChance = toolRules[tool->type].first;
        int durabilityPenalty = toolRules[tool->type].second;
        if (rand() % 100 < loseChance)
        {
            resultMsg = L"��� �������!";
            tool->durability -= durabilityPenalty;
            return LootType::None;
        }
        tool->durability -= durabilityPenalty;
        player.addLoot(possibleLoot[0]);
        resultMsg = L"�� �����: " + possibleLoot[0].name;
        return possibleLoot[0].type;
    }
};

class EgyptExpedition : public Excavation
{
public:
    EgyptExpedition() : Excavation(L"���������� ����������", 600)
    {
        possibleLoot.push_back({ LootType::Gold, 1, L"������", L"gold", 250 });
        toolRules[ToolType::Hummer] = { 0, 10 };
        toolRules[ToolType::Axe] = { 25, 25 };
        toolRules[ToolType::Pickaxe] = { 50, 25 };
    }
    LootType dig(Player& player, AssetManager& assets, std::wstring& resultMsg) override
    {
        player.useFood();
        Tool* tool = player.inventory.getTool(player.toolInHand);
        if (!tool || (tool->type != ToolType::Hummer && tool->type != ToolType::Axe && tool->type != ToolType::Pickaxe))
        {
            resultMsg = L"���������� �� ��������!";
            return LootType::None;
        }
        int loseChance = toolRules[tool->type].first;
        int durabilityPenalty = toolRules[tool->type].second;
        if (rand() % 100 < loseChance)
        {
            resultMsg = L"��� �������!";
            tool->durability -= durabilityPenalty;
            return LootType::None;
        }
        tool->durability -= durabilityPenalty;
        player.addLoot(possibleLoot[0]);
        resultMsg = L"�� �����: " + possibleLoot[0].name;
        return possibleLoot[0].type;
    }
};

class AfricaExpedition : public Excavation
{
public:
    AfricaExpedition() : Excavation(L"����������� ����������", 1000)
    {
        possibleLoot.push_back({ LootType::Diamond, 1, L"������", L"diamond", 750 });
        toolRules[ToolType::Pickaxe] = { 0, 10 };
        toolRules[ToolType::Shovel] = { 70, 25 };
    }
    LootType dig(Player& player, AssetManager& assets, std::wstring& resultMsg) override
    {
        player.useFood();
        Tool* tool = player.inventory.getTool(player.toolInHand);
        if (!tool || (tool->type != ToolType::Pickaxe && tool->type != ToolType::Shovel))
        {
            resultMsg = L"���������� �� ��������!";
            return LootType::None;
        }
        int loseChance = toolRules[tool->type].first;
        int durabilityPenalty = toolRules[tool->type].second;
        if (rand() % 100 < loseChance)
        {
            resultMsg = L"��� �������!";
            tool->durability -= durabilityPenalty;
            return LootType::None;
        }
        tool->durability -= durabilityPenalty;
        player.addLoot(possibleLoot[0]);
        resultMsg = L"�� �����: " + possibleLoot[0].name;
        return possibleLoot[0].type;
    }
};

class MoonExpedition : public Excavation
{
public:
    MoonExpedition() : Excavation(L"������ ����������", 3000)
    {
        possibleLoot.push_back({ LootType::MoonDust, 1, L"������ ����", L"moondust", 1300 });
        toolRules[ToolType::Hands] = { 25, 0 }; // ����� � ��� -2 ����������� ��������
        toolRules[ToolType::Shovel] = { 0, 10 };
    }
    LootType dig(Player& player, AssetManager& assets, std::wstring& resultMsg) override
    {
        if (player.toolInHand == ToolType::Hands) player.food -= 2;
        else player.useFood();
        Tool* tool = player.inventory.getTool(player.toolInHand);
        if (player.toolInHand != ToolType::Hands && (!tool || tool->type != ToolType::Shovel))
        {
            resultMsg = L"���������� �� ��������!";
            return LootType::None;
        }
        int loseChance = toolRules[player.toolInHand].first;
        int durabilityPenalty = toolRules[player.toolInHand].second;
        if (rand() % 100 < loseChance)
        {
            resultMsg = L"��� �������!";
            if (tool) tool->durability -= durabilityPenalty;
            return LootType::None;
        }
        if (tool) tool->durability -= durabilityPenalty;
        player.addLoot(possibleLoot[0]);
        resultMsg = L"�� �����: " + possibleLoot[0].name;
        return possibleLoot[0].type;
    }
};

// --- ������� ---
class Store
{
public:
    std::vector<Tool> toolsForSale = {
        {ToolType::Axe, 100, L"�����", L"axe", 200},
        {ToolType::Pickaxe, 100, L"�����", L"pickaxe", 300},
        {ToolType::Hummer, 100, L"�����", L"hummer", 250},
        {ToolType::Shovel, 100, L"������", L"shovel", 150}
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
        for (auto& l : player.inventory.loots)
        {
            if (l.type == type && l.count > 0)
            {
                player.money += l.price * l.count;
                l.count = 0;
            }
        }
    }
};

// --- ����� ---
class Museum
{
public:
    void show(const Player& player, AssetManager& assets, sf::RenderWindow& window)
    {
        // ������: ���������� ��� ������
        int x = 100, y = 100;
        for (const auto& l : player.inventory.loots)
        {
            if (l.count == 0) continue;
            sf::Sprite sprite(assets.getTexture(l.textureKey));
            sprite.setPosition({(float) x, (float) y });
            window.draw(sprite);
            // ����� �������� ����� � ����������� � �����
            x += 100;
        }
    }
};

// --- ����������/�������� ---
class SaveManager
{
public:
    void save(const Player& player)
    {
        std::wofstream f(L"save.txt");
        f << player.money << " " << player.food << "\n";
        for (const auto& t : player.inventory.tools)
            f << L"T " << int(t.type) << " " << t.durability << "\n";
        for (const auto& l : player.inventory.loots)
            f << L"L " << int(l.type) << " " << l.count << "\n";
    }
    void load(Player& player)
    {
        std::wifstream f(L"save.txt");
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
                player.addTool({ ToolType(ttype), dur, L"", L"", 0 });
            }
            else if (type == L"L")
            {
                int ltype, cnt;
                f >> ltype >> cnt;
                player.addLoot({ LootType(ltype), cnt, L"", L"", 0 });
            }
        }
    }
};

// --- ����� ���� ---
enum class GameScene { Base, ExpeditionChoice, Expedition, Store, Museum, Save, Exit };

// --- ������� ����� ���� ---
class Game
{
    sf::RenderWindow window;
    AssetManager assets;
    Player player;
    Store store;
    Museum museum;
    SaveManager saveManager;
    GameScene scene = GameScene::Base;
    std::unique_ptr<Excavation> currentExpedition;
    std::wstring lastExpeditionMsg;

public:
    Game() : window(sf::VideoMode({ 1920, 1080}), L"��������� ������������")
    {
        assets.loadAll();
        // ��������� �����������
        player.addTool({ ToolType::Axe,     100, L"�����",   L"axe",      0 });
        player.addTool({ ToolType::Hummer,  100, L"�����",   L"hummer",   0 });
        player.addTool({ ToolType::Pickaxe, 100, L"�����",   L"pickaxe",  0 });
        player.addTool({ ToolType::Shovel,  100, L"������",  L"shovel",   0 });
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
                if (key == sf::Keyboard::Scancode::Num1) player.toolInHand = ToolType::Axe;
                if (key == sf::Keyboard::Scancode::Num2) player.toolInHand = ToolType::Hummer;
                if (key == sf::Keyboard::Scancode::Num3) player.toolInHand = ToolType::Pickaxe;
                if (key == sf::Keyboard::Scancode::Num4) player.toolInHand = ToolType::Shovel;
                if (key == sf::Keyboard::Scancode::Num5) player.toolInHand = ToolType::Hands;
                if (key == sf::Keyboard::Scancode::Space && currentExpedition)
                {
                    lastExpeditionMsg.clear();
                    currentExpedition->dig(player, assets, lastExpeditionMsg);
                }
                if (key == sf::Keyboard::Scancode::Escape) scene = GameScene::Base;
                break;
            case GameScene::Store:
                if (key == sf::Keyboard::Scancode::Num1) store.buyTool(player, ToolType::Axe);
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
            default: break;
        }
    }

    void draw()
    {
        sf::Font font = assets.getFont(L"main");
        sf::Text text(font, L"", 24u);
        text.setFillColor(sf::Color::Black);

        if (scene == GameScene::Base)
        {
            text.setString(
                L"����\n"
                L"������: " + std::to_wstring(player.money) + L" ���: " + std::to_wstring(player.food) + "\n"
                L"1 - ����������\n"
                L"2 - �������\n"
                L"3 - �����\n"
                L"4 - ���������\n"
                L"Esc - �����"
            );
            window.draw(text);
        }
        else if (scene == GameScene::ExpeditionChoice)
        {
            text.setString(
                L"�������� ����������:\n"
                L"1 - ������ (100�)\n"
                L"2 - ���������� (600�)\n"
                L"3 - ����������� (1000�)\n"
                L"4 - ������ (3000�)\n"
                L"Esc - �����"
            );
            window.draw(text);
        }
        else if (scene == GameScene::Expedition)
        {
            text.setString(
                L"����������: " + currentExpedition->getName() + "\n"
                L"���������� � ����: " + std::to_wstring(int(player.toolInHand)) + "\n"
                L"1 - �����, 2 - �����, 3 - �����, 4 - ������, 5 - ����\n"
                L"������ - ������\n"
                L"Esc - �����\n" +
                lastExpeditionMsg
            );
            window.draw(text);
        }
        else if (scene == GameScene::Store)
        {
            text.setString(
                L"�������\n"
                L"1 - ������ ����� (200�)\n"
                L"2 - ������ ����� (300�)\n"
                L"3 - ������ ����� (250�)\n"
                L"4 - ������ ������ (150�)\n"
                L"5 - ������ ��� (50� �� 5)\n"
                L"6 - ������� ������\n"
                L"7 - ������� ������\n"
                L"8 - ������� ������\n"
                L"9 - ������� ������ ����\n"
                L"Esc - �����"
            );
            window.draw(text);
        }
        else if (scene == GameScene::Museum)
        {
            text.setString(L"����� (Esc - �����)");
            window.draw(text);
            museum.show(player, assets, window);
        }
    }
};

int main()
{
    Game game;
    game.run();
    return 0;
}