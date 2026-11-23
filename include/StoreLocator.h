#ifndef STORE_LOCATOR_H
#define STORE_LOCATOR_H

#include <vector>
#include <string>
#include <unordered_map>
#include "Store.h"
#include "Product.h"

struct StoreRanking {
    Store store;
    double distanceKm;
    double totalCost;
    double score;
    std::vector<std::string> missingItems;
};

class StoreLocator {
private:
    std::unordered_map<int, Store> stores;
    int nextId;

    static double haversine(double lat1, double lon1,
                            double lat2, double lon2);

public:
    StoreLocator() : nextId(1) {}

    int addStore(const std::string& name,
                 const std::string& address,
                 double lat, double lon,
                 double rating);
    bool setPrice(int storeId, int productId, double price);
    bool removeStore(int storeId);

    Store* getStore(int storeId);
    bool hasStore(int storeId) const;
    std::vector<Store> allStores() const;

    std::vector<StoreRanking> rankStoresFor(
        const std::vector<int>& productIds,
        const std::vector<Product>& products,
        double userLat, double userLon,
        double maxDistanceKm,
        double distanceWeight = 0.3) const;

    StoreRanking bestStoreFor(
        const std::vector<int>& productIds,
        const std::vector<Product>& products,
        double userLat, double userLon,
        double maxDistanceKm,
        double distanceWeight = 0.3) const;

    void saveToFile(const std::string& path) const;
    void loadFromFile(const std::string& path,
                      const std::string& pricePath);
    void savePrices(const std::string& path) const;
    void loadPrices(const std::string& path);

    size_t size() const { return stores.size(); }
    const std::unordered_map<int, Store>& raw() const { return stores; }
};

#endif