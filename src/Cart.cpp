#include "Cart.h"
#include <ctime>
#include <sstream>
#include <iomanip>

bool Cart::addItem(int productId, int quantity) {
    if (!inventory->hasProduct(productId)) return false;
    Product* p = inventory->getProduct(productId);
    if (!p) return false;
    if (p->stock < quantity) return false;

    for (auto& line : lines) {
        if (line.productId == productId) {
            if (p->stock < line.quantity + quantity) return false;
            line.quantity += quantity;
            return true;
        }
    }
    CartLine l;
    l.productId = productId;
    l.name = p->name;
    l.quantity = quantity;
    l.unitPrice = p->price;
    lines.push_back(l);
    return true;
}

bool Cart::removeItem(int productId) {
    for (auto it = lines.begin(); it != lines.end(); ++it) {
        if (it->productId == productId) {
            lines.erase(it);
            return true;
        }
    }
    return false;
}

bool Cart::updateQuantity(int productId, int quantity) {
    if (quantity <= 0) return removeItem(productId);
    Product* p = inventory->getProduct(productId);
    if (!p || p->stock < quantity) return false;
    for (auto& line : lines) {
        if (line.productId == productId) {
            line.quantity = quantity;
            return true;
        }
    }
    return false;
}

void Cart::clear() { lines.clear(); }

double Cart::subtotal() const {
    double s = 0.0;
    for (const auto& l : lines) s += l.quantity * l.unitPrice;
    return s;
}

double Cart::tax(double rate) const { return subtotal() * rate; }

double Cart::total(double rate) const { return subtotal() + tax(rate); }

Sale Cart::checkout(const std::string& customerName,
                    const std::string& paymentMethod,
                    int saleId) {
    Sale s;
    s.id = saleId;
    s.customerName = customerName.empty() ? "walk-in" : customerName;
    s.paymentMethod = paymentMethod;

    /*std::time_t now = std::time(nullptr);
    std::tm* lt = std::localtime(&now);
    std::ostringstream oss;
    oss << std::put_time(lt, "%Y-%m-%d %H:%M");
    s.date = oss.str();*/
    
        std::time_t now = std::time(nullptr);
    std::tm* lt = std::localtime(&now);
    char buffer[80];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M", lt);
    s.date = buffer;

    for (const auto& l : lines) {
        SaleItem si;
        si.productId = l.productId;
        si.productName = l.name;
        si.quantity = l.quantity;
        si.unitPrice = l.unitPrice;
        si.lineTotal = l.quantity * l.unitPrice;
        s.items.push_back(si);
        inventory->updateStock(l.productId, -l.quantity);
    }

    s.subtotal = subtotal();
    s.tax = tax(0.05);
    s.total = s.subtotal + s.tax;

    clear();
    return s;
}
