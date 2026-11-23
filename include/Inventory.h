#ifndef INVENTORY_H
#define INVENTORY_H

#include <unordered_map>
#include <vector>
#include <string>
#include "Product.h"

class Inventory {
private:
    std::unordered_map<int, Product> products;
    std::unordered_map<std::string, int> nameIndex;
    int nextId;

public:
    Inventory() : nextId(1) {}

    int addProduct(const std::string& name, const std::string& category,
                   double price, int stock, const std::string& expiry,
                   int lowStockThreshold, const std::string& supplier);
    bool updateStock(int id, int delta);
    bool setStock(int id, int stock);
    bool removeProduct(int id);

    Product* getProduct(int id);
    Product* getProductByName(const std::string& name);
    int findIdByName(const std::string& name) const;
    bool hasProduct(int id) const;

    std::vector<Product> allProducts() const;
    std::vector<Product> lowStockProducts() const;
    std::vector<Product> expiringProducts(int days = 3) const;
    std::vector<Product> searchByName(const std::string& query) const;
    std::vector<Product> topSellers(int n) const;

    void saveToFile(const std::string& path) const;
    void loadFromFile(const std::string& path);

    int getNextId() const { return nextId; }
    size_t size() const { return products.size(); }
    const std::unordered_map<int, Product>& raw() const { return products; }
};

#endif