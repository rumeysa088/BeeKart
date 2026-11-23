#ifndef CART_H
#define CART_H

#include <vector>
#include <string>
#include "Inventory.h"
#include "Sale.h"

struct CartLine {
    int productId;
    std::string name;
    int quantity;
    double unitPrice;
};

class Cart {
private:
    std::vector<CartLine> lines;
    Inventory* inventory;

public:
    Cart(Inventory* inv) : inventory(inv) {}

    bool addItem(int productId, int quantity);
    bool removeItem(int productId);
    bool updateQuantity(int productId, int quantity);
    void clear();

    double subtotal() const;
    double tax(double rate = 0.05) const;
    double total(double rate = 0.05) const;

    const std::vector<CartLine>& getLines() const { return lines; }
    bool empty() const { return lines.empty(); }

    Sale checkout(const std::string& customerName,
                  const std::string& paymentMethod,
                  int saleId);
};

#endif