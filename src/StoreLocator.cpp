#include "StoreLocator.h"
#include "FileManager.h"
#include <cmath>
#include <algorithm>
#include <sstream>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

double StoreLocator::haversine(double lat1, double lon1,
                               double lat2, double lon2) {
    const double R = 6371.0;
    double dLat = (lat2 - lat1) * M_PI / 180.0;
    double dLon = (lon2 - lon1) * M_PI / 180.0;
    double a = std::sin(dLat / 2) * std::sin(dLat / 2) +
               std::cos(lat1 * M_PI / 180.0) *
               std::cos(lat2 * M_PI / 180.0) *
               std::sin(dLon / 2) * std::sin(dLon / 2);
    double c = 2 * std::atan2(std::sqrt(a), std::sqrt(1 - a));
    return R * c;
}

int StoreLocator::addStore(const std::string& name,
                           const std::string& address,
                           double lat, double lon,
                           double rating) {
    Store s;
    s.id = nextId++;
    s.name = name;
    s.address = address;
    s.latitude = lat;
    s.longitude = lon;
    s.rating = rating;
    stores[s.id] = s;
    return s.id;
}

bool StoreLocator::setPrice(int storeId, int productId, double price) {
    auto it = stores.find(storeId);
    if (it == stores.end()) return false;
    it->second.prices[productId] = price;
    return true;
}

bool StoreLocator::removeStore(int storeId) {
    return stores.erase(storeId) > 0;
}

Store* StoreLocator::getStore(int storeId) {
    auto it = stores.find(storeId);
    if (it == stores.end()) return nullptr;
    return &it->second;
}

bool StoreLocator::hasStore(int storeId) const {
    return stores.find(storeId) != stores.end();
}

std::vector<Store> StoreLocator::allStores() const {
    std::vector<Store> out;
    for (const auto& p : stores) out.push_back(p.second);
    std::sort(out.begin(), out.end(),
              [](const Store& a, const Store& b) { return a.id < b.id; });
    return out;
}

std::vector<StoreRanking> StoreLocator::rankStoresFor(
        const std::vector<int>& productIds,
        const std::vector<Product>& products,
        double userLat, double userLon,
        double maxDistanceKm,
        double distanceWeight) const {

    std::vector<StoreRanking> out;

    for (const auto& pair : stores) {
        const Store& s = pair.second;
        double dist = haversine(userLat, userLon, s.latitude, s.longitude);
        if (dist > maxDistanceKm) continue;

        StoreRanking r;
        r.store = s;
        r.distanceKm = dist;
        r.totalCost = 0.0;

        for (int pid : productIds) {
            if (s.hasProduct(pid)) {
                r.totalCost += s.getPrice(pid);
            } else {
                std::string name = "item#" + std::to_string(pid);
                for (const auto& p : products)
                    if (p.id == pid) { name = p.name; break; }
                r.missingItems.push_back(name);
                double sum = 0; int cnt = 0;
                for (const auto& sp : stores) {
                    if (sp.second.hasProduct(pid)) {
                        sum += sp.second.getPrice(pid);
                        cnt++;
                    }
                }
                r.totalCost += (cnt > 0) ? sum / cnt : 100.0;
            }
        }

        double normDist = std::min(1.0, dist / 10.0);
        double normCost = std::min(1.0, r.totalCost / 2000.0);
        r.score = normDist * distanceWeight + normCost * (1.0 - distanceWeight);
        out.push_back(r);
    }

    std::sort(out.begin(), out.end(),
              [](const StoreRanking& a, const StoreRanking& b) {
                  return a.score < b.score;
              });
    return out;
}

StoreRanking StoreLocator::bestStoreFor(
        const std::vector<int>& productIds,
        const std::vector<Product>& products,
        double userLat, double userLon,
        double maxDistanceKm,
        double distanceWeight) const {
    auto ranked = rankStoresFor(productIds, products, userLat, userLon,
                                maxDistanceKm, distanceWeight);
    if (ranked.empty()) return StoreRanking{};
    return ranked.front();
}

void StoreLocator::saveToFile(const std::string& path) const {
    std::vector<std::string> lines;
    for (const auto& p : stores) {
        const Store& s = p.second;
        std::ostringstream oss;
        oss << s.id << "|" << s.name << "|" << s.address << "|"
            << s.latitude << "|" << s.longitude << "|" << s.rating;
        lines.push_back(oss.str());
    }
    FileManager::writeLines(path, lines);
}

void StoreLocator::loadFromFile(const std::string& path,
                                const std::string& pricePath) {
    stores.clear();
    nextId = 1;
    auto lines = FileManager::readLines(path);
    for (const auto& line : lines) {
        auto f = FileManager::split(line, '|');
        if (f.size() < 6) continue;
        Store s;
        s.id = std::stoi(f[0]);
        s.name = f[1];
        s.address = f[2];
        s.latitude = std::stod(f[3]);
        s.longitude = std::stod(f[4]);
        s.rating = std::stod(f[5]);
        stores[s.id] = s;
        if (s.id >= nextId) nextId = s.id + 1;
    }
    loadPrices(pricePath);
}

void StoreLocator::savePrices(const std::string& path) const {
    std::vector<std::string> lines;
    for (const auto& p : stores) {
        const Store& s = p.second;
        for (const auto& pr : s.prices) {
            std::ostringstream oss;
            oss << s.id << "|" << pr.first << "|" << pr.second;
            lines.push_back(oss.str());
        }
    }
    FileManager::writeLines(path, lines);
}

void StoreLocator::loadPrices(const std::string& path) {
    auto lines = FileManager::readLines(path);
    for (const auto& line : lines) {
        auto f = FileManager::split(line, '|');
        if (f.size() < 3) continue;
        int sid = std::stoi(f[0]);
        int pid = std::stoi(f[1]);
        double price = std::stod(f[2]);
        auto it = stores.find(sid);
        if (it != stores.end()) it->second.prices[pid] = price;
    }
}
