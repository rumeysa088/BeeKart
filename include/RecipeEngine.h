#ifndef RECIPE_ENGINE_H
#define RECIPE_ENGINE_H

#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include "Recipe.h"
#include "Inventory.h"

struct RecipeSuggestion {
    Recipe recipe;
    int missingCount;
    std::vector<std::string> missingItems;
    double estimatedCost;
    int expiringItemsUsed;
};

struct ExpiryRescue {
    std::string expiringProduct;
    int daysLeft;
    std::vector<Recipe> recipesUsingIt;
};

class RecipeEngine {
private:
    std::vector<Recipe> recipes;
    Inventory* inventory;
    std::unordered_map<std::string, std::vector<int>> ingredientIndex;

    void reindex();

public:
    RecipeEngine(Inventory* inv) : inventory(inv) {}

    int addRecipe(const std::string& name,
                  const std::vector<std::string>& ingredients,
                  const std::string& instructions,
                  int prepTime, const std::string& category);
    bool removeRecipe(int id);

    std::vector<RecipeSuggestion> suggestByAvailable(
        const std::vector<std::string>& availableIngredients,
        int maxMissing = 3) const;

    std::vector<ExpiryRescue> expiryRescueSuggestions(int days = 5) const;

    std::vector<Recipe> searchByName(const std::string& query) const;
    std::vector<Recipe> searchByIngredient(const std::string& ing) const;

    std::vector<std::pair<std::string, double>>
    generateShoppingList(int recipeId) const;

    void saveToFile(const std::string& path) const;
    void loadFromFile(const std::string& path);

    const std::vector<Recipe>& all() const { return recipes; }
};

#endif