#ifndef STORE_H
#define STORE_H

#include <string>
#include <unordered_map>

struct Store {
    int id;
    std::string name;
    std::string address;
    double latitude;
    double longitude;
    double rating;
    std::unordered_map<int, double> prices;

    Store() : id(0), latitude(0.0), longitude(0.0), rating(0.0) {}

    bool hasProduct(int productId) const {
        return prices.find(productId) != prices.end();
    }

    double getPrice(int productId) const {
        auto it = prices.find(productId);
        return (it == prices.end()) ? -1.0 : it->second;
    }
};

#endif