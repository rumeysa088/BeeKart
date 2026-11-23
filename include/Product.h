#ifndef PRODUCT_H
#define PRODUCT_H

#include <string>

struct Product {
    int id;
    std::string name;
    std::string category;
    double price;
    int stock;
    std::string expiryDate;
    int lowStockThreshold;
    int unitsSold;
    std::string supplier;

    Product()
        : id(0), price(0.0), stock(0),
          lowStockThreshold(10), unitsSold(0) {}

    int daysUntilExpiry() const;
    bool isExpiringSoon(int days = 3) const;
    bool isLowStock() const { return stock <= lowStockThreshold; }
};

#endif