#ifndef SHOPPING_ASSISTANT_H
#define SHOPPING_ASSISTANT_H

#include <vector>
#include <string>
#include "Inventory.h"
#include "RecipeEngine.h"
#include "StoreLocator.h"

struct ResolvedItem {
    std::string inputName;
    int productId;
    int quantity;
    bool found;
};

struct ShoppingPlan {
    std::vector<ResolvedItem> items;
    std::vector<RecipeSuggestion> recipes;
    std::vector<StoreRanking> storeRankings;
    bool hasBestStore;
    StoreRanking bestStore;
};

class ShoppingAssistant {
private:
    Inventory* inventory;
    RecipeEngine* recipeEngine;
    StoreLocator* locator;

public:
    ShoppingAssistant(Inventory* inv, RecipeEngine* re, StoreLocator* sl)
        : inventory(inv), recipeEngine(re), locator(sl) {}

    std::vector<ResolvedItem> parseList(
        const std::vector<std::pair<std::string, int>>& inputs) const;

    std::vector<RecipeSuggestion> suggestRecipes(
        const std::vector<ResolvedItem>& items,
        int maxMissing = 2) const;

    std::vector<StoreRanking> rankStores(
        const std::vector<ResolvedItem>& items,
        double userLat, double userLon,
        double maxDistanceKm,
        double distanceWeight = 0.3) const;

    ShoppingPlan buildPlan(
        const std::vector<std::pair<std::string, int>>& inputs,
        double userLat, double userLon,
        double maxDistanceKm,
        double distanceWeight = 0.3) const;
};

#endif