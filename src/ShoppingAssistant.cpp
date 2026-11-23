#include "ShoppingAssistant.h"
#include <vector>
#include <string>

std::vector<ResolvedItem> ShoppingAssistant::parseList(
        const std::vector<std::pair<std::string, int>>& inputs) const {
    std::vector<ResolvedItem> out;
    for (const auto& in : inputs) {
        ResolvedItem r;
        r.inputName = in.first;
        r.quantity = in.second;
        r.productId = inventory->findIdByName(in.first);
        r.found = (r.productId != -1);
        out.push_back(r);
    }
    return out;
}

std::vector<RecipeSuggestion> ShoppingAssistant::suggestRecipes(
        const std::vector<ResolvedItem>& items,
        int maxMissing) const {
    std::vector<std::string> have;
    for (const auto& it : items)
        if (it.found) have.push_back(it.inputName);
    return recipeEngine->suggestByAvailable(have, maxMissing);
}

std::vector<StoreRanking> ShoppingAssistant::rankStores(
        const std::vector<ResolvedItem>& items,
        double userLat, double userLon,
        double maxDistanceKm,
        double distanceWeight) const {
    std::vector<int> productIds;
    for (const auto& it : items)
        if (it.found) productIds.push_back(it.productId);
    auto products = inventory->allProducts();
    return locator->rankStoresFor(productIds, products,
                                  userLat, userLon,
                                  maxDistanceKm, distanceWeight);
}

ShoppingPlan ShoppingAssistant::buildPlan(
        const std::vector<std::pair<std::string, int>>& inputs,
        double userLat, double userLon,
        double maxDistanceKm,
        double distanceWeight) const {
    ShoppingPlan plan;
    plan.items = parseList(inputs);
    plan.recipes = suggestRecipes(plan.items, 2);
    plan.storeRankings = rankStores(plan.items, userLat, userLon,
                                    maxDistanceKm, distanceWeight);
    plan.hasBestStore = !plan.storeRankings.empty();
    if (plan.hasBestStore) plan.bestStore = plan.storeRankings.front();
    return plan;
}
