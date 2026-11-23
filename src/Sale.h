#ifndef SALE_H
#define SALE_H

#include <string>
#include <vector>

struct SaleItem {
    int productId;
    std::string productName;
    int quantity;
    double unitPrice;
    double lineTotal;
};

struct Sale {
    int id;
    std::string date;
    std::string customerName;
    std::vector<SaleItem> items;
    double subtotal;
    double tax;
    double total;
    std::string paymentMethod;
};

#endif
