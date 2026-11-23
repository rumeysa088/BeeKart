#ifndef ANALYTICS_H
#define ANALYTICS_H

#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include "Sale.h"
#include "Inventory.h"
#include "Customer.h"

struct BasketSuggestion {
    std::string item;
    int coPurchaseCount;
    double confidence;
};

struct CreditRisk {
    Customer customer;
    double riskScore;
};

struct ReorderAlert {
    int productId;
    std::string name;
    int currentStock;
    int dailyVelocity;
    double daysUntilStockout;
    int recommendedOrder;
};

class Analytics {
private:
    std::vector<Sale> sales;
    std::unordered_map<long long, int> coPurchaseCounts;
    std::unordered_map<int, int> itemPurchaseCount;

    static long long pairKey(int a, int b) {
        if (a > b) std::swap(a, b);
        return (long long)a * 100000LL + b;
    }

public:
    Analytics() {}

    void addSale(const Sale& s);
    void rebuild();

    std::vector<std::pair<int, int>> topSellers(int n) const;
    std::vector<BasketSuggestion> suggestionsFor(int productId, int topN = 5) const;
    std::vector<CreditRisk> rankCustomersByRisk(
        const std::vector<Customer>& customers) const;
    std::vector<ReorderAlert> reorderAlerts(const Inventory& inv) const;

    void saveToFile(const std::string& path) const;
    void loadFromFile(const std::string& path);

    const std::vector<Sale>& allSales() const { return sales; }
};

#endif