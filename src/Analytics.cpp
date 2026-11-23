#include "Analytics.h"
#include "FileManager.h"
#include <algorithm>
#include <sstream>

void Analytics::addSale(const Sale& s) {
    sales.push_back(s);
    for (const auto& item : s.items) itemPurchaseCount[item.productId]++;

    std::unordered_set<int> ids;
    for (const auto& item : s.items) ids.insert(item.productId);
    std::vector<int> v(ids.begin(), ids.end());
    for (size_t i = 0; i < v.size(); ++i)
        for (size_t j = i + 1; j < v.size(); ++j)
            coPurchaseCounts[pairKey(v[i], v[j])]++;
}

void Analytics::rebuild() {
    coPurchaseCounts.clear();
    itemPurchaseCount.clear();
    for (const auto& s : sales) {
        for (const auto& item : s.items) itemPurchaseCount[item.productId]++;
        std::unordered_set<int> ids;
        for (const auto& item : s.items) ids.insert(item.productId);
        std::vector<int> v(ids.begin(), ids.end());
        for (size_t i = 0; i < v.size(); ++i)
            for (size_t j = i + 1; j < v.size(); ++j)
                coPurchaseCounts[pairKey(v[i], v[j])]++;
    }
}

std::vector<std::pair<int, int>> Analytics::topSellers(int n) const {
    std::vector<std::pair<int, int>> out(itemPurchaseCount.begin(),
                                          itemPurchaseCount.end());
//    std::sort(out.begin(), out.end(),
//              [](const auto& a, const auto& b) { return a.second > b.second; });
 
     std::sort(out.begin(), out.end(),
              [](const std::pair<int, int>& a, const std::pair<int, int>& b) { 
                  return a.second > b.second; 
              });
    if ((int)out.size() > n) out.resize(n);
    return out;
}

std::vector<BasketSuggestion> Analytics::suggestionsFor(
        int productId, int topN) const {
    std::vector<BasketSuggestion> out;
    int baseCount = 0;
    auto bit = itemPurchaseCount.find(productId);
    if (bit != itemPurchaseCount.end()) baseCount = bit->second;
    if (baseCount == 0) return out;

    for (const auto& p : coPurchaseCounts) {
        int a = (int)(p.first / 100000LL);
        int b = (int)(p.first % 100000LL);
        int other = -1;
        if (a == productId) other = b;
        else if (b == productId) other = a;
        if (other == -1) continue;

        BasketSuggestion sug;
        sug.item = std::to_string(other);
        sug.coPurchaseCount = p.second;
        sug.confidence = (double)p.second / baseCount;
        out.push_back(sug);
    }

    std::sort(out.begin(), out.end(),
              [](const BasketSuggestion& a, const BasketSuggestion& b) {
                  return a.confidence > b.confidence;
              });
    if ((int)out.size() > topN) out.resize(topN);
    return out;
}

std::vector<CreditRisk> Analytics::rankCustomersByRisk(
        const std::vector<Customer>& customers) const {
    std::vector<CreditRisk> out;
    for (const auto& c : customers) {
        CreditRisk r;
        r.customer = c;
        double ratio = c.onTimeRatio();
        double balanceFactor = c.creditBalance / (c.creditLimit + 1.0);
        r.riskScore = (1.0 - ratio) * 0.7 + std::min(1.0, balanceFactor) * 0.3;
        out.push_back(r);
    }
    std::sort(out.begin(), out.end(),
              [](const CreditRisk& a, const CreditRisk& b) {
                  return a.riskScore > b.riskScore;
              });
    return out;
}

std::vector<ReorderAlert> Analytics::reorderAlerts(
        const Inventory& inv) const {
    std::vector<ReorderAlert> out;
    std::unordered_map<int, int> productSales;
    int totalDays = 1;
    if (!sales.empty()) totalDays = std::max(1, (int)sales.size() / 3);

    for (const auto& s : sales)
        for (const auto& item : s.items)
            productSales[item.productId] += item.quantity;

    for (const auto& pair : inv.raw()) {
        const Product& p = pair.second;
        int sold = productSales[p.id];
        int velocity = std::max(1, sold / totalDays);

        ReorderAlert alert;
        alert.productId = p.id;
        alert.name = p.name;
        alert.currentStock = p.stock;
        alert.dailyVelocity = velocity;
        alert.daysUntilStockout = (double)p.stock / velocity;
        alert.recommendedOrder = std::max(50, velocity * 7);
        if (alert.daysUntilStockout <= 3.0) out.push_back(alert);
    }

    std::sort(out.begin(), out.end(),
              [](const ReorderAlert& a, const ReorderAlert& b) {
                  return a.daysUntilStockout < b.daysUntilStockout;
              });
    return out;
}

void Analytics::saveToFile(const std::string& path) const {
    std::vector<std::string> lines;
    for (const auto& s : sales) {
        std::ostringstream oss;
        oss << s.id << "|" << s.date << "|" << s.customerName << "|"
            << s.paymentMethod << "|" << s.subtotal << "|" << s.tax << "|"
            << s.total << "|";
        for (size_t i = 0; i < s.items.size(); ++i) {
            if (i) oss << ";";
            oss << s.items[i].productId << "," << s.items[i].productName << ","
                << s.items[i].quantity << "," << s.items[i].unitPrice;
        }
        lines.push_back(oss.str());
    }
    FileManager::writeLines(path, lines);
}

void Analytics::loadFromFile(const std::string& path) {
    sales.clear();
    auto lines = FileManager::readLines(path);
    for (const auto& line : lines) {
        auto f = FileManager::split(line, '|');
        if (f.size() < 8) continue;
        Sale s;
        s.id = std::stoi(f[0]);
        s.date = f[1];
        s.customerName = f[2];
        s.paymentMethod = f[3];
        s.subtotal = std::stod(f[4]);
        s.tax = std::stod(f[5]);
        s.total = std::stod(f[6]);
        auto items = FileManager::split(f[7], ';');
        for (const auto& it : items) {
            auto parts = FileManager::split(it, ',');
            if (parts.size() < 4) continue;
            SaleItem si;
            si.productId = std::stoi(parts[0]);
            si.productName = parts[1];
            si.quantity = std::stoi(parts[2]);
            si.unitPrice = std::stod(parts[3]);
            si.lineTotal = si.quantity * si.unitPrice;
            s.items.push_back(si);
        }
        sales.push_back(s);
    }
    rebuild();
}

