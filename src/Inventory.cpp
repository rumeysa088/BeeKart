#include "Inventory.h"
#include "FileManager.h"
#include <algorithm>
#include <ctime>
#include <sstream>
#include <iomanip>

/*
static std::tm parseDate(const std::string& s) {
    std::tm tm = {};
    if (s.empty()) return tm;
    std::stringstream ss(s);
    ss >> std::get_time(&tm, "%Y-%m-%d");
    return tm;
}*/

static std::tm parseDate(const std::string& s) {
    struct std::tm tm = {};
    if (s.empty()) return tm;
    sscanf(s.c_str(), "%d-%d-%d", &tm.tm_year, &tm.tm_mon, &tm.tm_mday);
    tm.tm_year -= 1900; // Adjust for C standard
    tm.tm_mon -= 1;     // Adjust for C standard
    return tm;
}

int Product::daysUntilExpiry() const {
    if (expiryDate.empty()) return 9999;
    std::tm target = parseDate(expiryDate);
    std::time_t t = std::mktime(&target);
    std::time_t now = std::time(nullptr);
    double diff = std::difftime(t, now) / (60 * 60 * 24);
    return (int)diff;
}

bool Product::isExpiringSoon(int days) const {
    int d = daysUntilExpiry();
    return d >= 0 && d <= days;
}

static std::string lower(const std::string& s) {
    std::string out = s;
    std::transform(out.begin(), out.end(), out.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return out;
}

int Inventory::addProduct(const std::string& name,
                          const std::string& category,
                          double price, int stock,
                          const std::string& expiry,
                          int lowStockThreshold,
                          const std::string& supplier) {
    Product p;
    p.id = nextId++;
    p.name = name;
    p.category = category;
    p.price = price;
    p.stock = stock;
    p.expiryDate = expiry;
    p.lowStockThreshold = lowStockThreshold;
    p.supplier = supplier;
    p.unitsSold = 0;
    products[p.id] = p;
    nameIndex[lower(name)] = p.id;
    return p.id;
}

bool Inventory::updateStock(int id, int delta) {
    auto it = products.find(id);
    if (it == products.end()) return false;
    if (it->second.stock + delta < 0) return false;
    it->second.stock += delta;
    if (delta < 0) it->second.unitsSold += -delta;
    return true;
}

bool Inventory::setStock(int id, int stock) {
    auto it = products.find(id);
    if (it == products.end()) return false;
    it->second.stock = stock;
    return true;
}

bool Inventory::removeProduct(int id) {
    auto it = products.find(id);
    if (it == products.end()) return false;
    nameIndex.erase(lower(it->second.name));
    products.erase(it);
    return true;
}

Product* Inventory::getProduct(int id) {
    auto it = products.find(id);
    if (it == products.end()) return nullptr;
    return &it->second;
}

Product* Inventory::getProductByName(const std::string& name) {
    auto it = nameIndex.find(lower(name));
    if (it == nameIndex.end()) return nullptr;
    return getProduct(it->second);
}

int Inventory::findIdByName(const std::string& name) const {
    auto it = nameIndex.find(lower(name));
    if (it == nameIndex.end()) return -1;
    return it->second;
}

bool Inventory::hasProduct(int id) const {
    return products.find(id) != products.end();
}

std::vector<Product> Inventory::allProducts() const {
    std::vector<Product> out;
    for (const auto& p : products) out.push_back(p.second);
    std::sort(out.begin(), out.end(),
              [](const Product& a, const Product& b) { return a.id < b.id; });
    return out;
}

std::vector<Product> Inventory::lowStockProducts() const {
    std::vector<Product> out;
    for (const auto& p : products)
        if (p.second.isLowStock()) out.push_back(p.second);
    return out;
}

std::vector<Product> Inventory::expiringProducts(int days) const {
    std::vector<Product> out;
    for (const auto& p : products)
        if (p.second.isExpiringSoon(days)) out.push_back(p.second);
    std::sort(out.begin(), out.end(),
              [](const Product& a, const Product& b) {
                  return a.daysUntilExpiry() < b.daysUntilExpiry();
              });
    return out;
}

std::vector<Product> Inventory::searchByName(const std::string& query) const {
    std::vector<Product> out;
    std::string q = lower(query);
    for (const auto& p : products)
        if (lower(p.second.name).find(q) != std::string::npos)
            out.push_back(p.second);
    return out;
}

std::vector<Product> Inventory::topSellers(int n) const {
    std::vector<Product> out;
    for (const auto& p : products) out.push_back(p.second);
    std::sort(out.begin(), out.end(),
              [](const Product& a, const Product& b) {
                  return a.unitsSold > b.unitsSold;
              });
    if ((int)out.size() > n) out.resize(n);
    return out;
}

void Inventory::saveToFile(const std::string& path) const {
    std::vector<std::string> lines;
    for (const auto& p : products) {
        const Product& pr = p.second;
        std::ostringstream oss;
        oss << pr.id << "|" << pr.name << "|" << pr.category << "|"
            << pr.price << "|" << pr.stock << "|" << pr.expiryDate << "|"
            << pr.lowStockThreshold << "|" << pr.unitsSold << "|"
            << pr.supplier;
        lines.push_back(oss.str());
    }
    FileManager::writeLines(path, lines);
}

void Inventory::loadFromFile(const std::string& path) {
    products.clear();
    nameIndex.clear();
    nextId = 1;
    auto lines = FileManager::readLines(path);
    for (const auto& line : lines) {
        auto f = FileManager::split(line, '|');
        if (f.size() < 9) continue;
        Product p;
        p.id = std::stoi(f[0]);
        p.name = f[1];
        p.category = f[2];
        p.price = std::stod(f[3]);
        p.stock = std::stoi(f[4]);
        p.expiryDate = f[5];
        p.lowStockThreshold = std::stoi(f[6]);
        p.unitsSold = std::stoi(f[7]);
        p.supplier = f[8];
        products[p.id] = p;
        nameIndex[lower(p.name)] = p.id;
        if (p.id >= nextId) nextId = p.id + 1;
    }
}
