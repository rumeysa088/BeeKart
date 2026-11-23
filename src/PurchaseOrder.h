#ifndef PURCHASE_ORDER_H
#define PURCHASE_ORDER_H

#include <string>
#include <vector>

struct POItem {
    int productId;
    std::string productName;
    int quantity;
    double unitCost;
    double lineTotal;
};

struct PurchaseOrder {
    int id;
    int supplierId;
    std::string supplierName;
    std::string date;
    std::string status;
    std::vector<POItem> items;
    double total;

    PurchaseOrder() : id(0), supplierId(0), total(0.0) {}
};

#endif
