#include "RecipeEngine.h"
#include "FileManager.h"
#include <algorithm>
#include <sstream>
#include <cctype>

static std::string lower(const std::string& s) {
    std::string out = s;
    std::transform(out.begin(), out.end(), out.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return out;
}

void RecipeEngine::reindex() {
    ingredientIndex.clear();
    for (size_t i = 0; i < recipes.size(); ++i) {
        for (const auto& ing : recipes[i].ingredients)
            ingredientIndex[lower(ing)].push_back((int)i);
    }
}

int RecipeEngine::addRecipe(const std::string& name,
                            const std::vector<std::string>& ingredients,
                            const std::string& instructions,
                            int prepTime, const std::string& category) {
    Recipe r;
    r.id = (int)recipes.size() + 1;
    r.name = name;
    r.ingredients = ingredients;
    r.instructions = instructions;
    r.prepTimeMinutes = prepTime;
    r.category = category;
    recipes.push_back(r);
    reindex();
    return r.id;
}

bool RecipeEngine::removeRecipe(int id) {
    for (auto it = recipes.begin(); it != recipes.end(); ++it) {
        if (it->id == id) {
            recipes.erase(it);
            reindex();
            return true;
        }
    }
    return false;
}

std::vector<RecipeSuggestion> RecipeEngine::suggestByAvailable(
        const std::vector<std::string>& availableIngredients,
        int maxMissing) const {

    std::unordered_set<std::string> have;
    for (const auto& a : availableIngredients) have.insert(lower(a));

    std::vector<RecipeSuggestion> out;

    for (const auto& r : recipes) {
        RecipeSuggestion sug;
        sug.recipe = r;
        sug.missingCount = 0;
        sug.estimatedCost = 0.0;
        sug.expiringItemsUsed = 0;

        for (const auto& ing : r.ingredients) {
            if (have.find(lower(ing)) == have.end()) {
                sug.missingCount++;
                sug.missingItems.push_back(ing);
                Product* p = inventory->getProductByName(ing);
                if (p) {
                    sug.estimatedCost += p->price;
                    if (p->isExpiringSoon(5)) sug.expiringItemsUsed++;
                }
            }
        }
        if (sug.missingCount <= maxMissing)
            out.push_back(sug);
    }

    std::sort(out.begin(), out.end(),
              [](const RecipeSuggestion& a, const RecipeSuggestion& b) {
                  if (a.missingCount != b.missingCount)
                      return a.missingCount < b.missingCount;
                  return a.expiringItemsUsed > b.expiringItemsUsed;
              });
    return out;
}

std::vector<ExpiryRescue> RecipeEngine::expiryRescueSuggestions(int days) const {
    std::vector<ExpiryRescue> out;
    auto expiring = inventory->expiringProducts(days);
    for (const auto& p : expiring) {
        ExpiryRescue er;
        er.expiringProduct = p.name;
        er.daysLeft = p.daysUntilExpiry();
        auto it = ingredientIndex.find(lower(p.name));
        if (it != ingredientIndex.end())
            for (int idx : it->second) er.recipesUsingIt.push_back(recipes[idx]);
        out.push_back(er);
    }
    return out;
}

std::vector<Recipe> RecipeEngine::searchByName(const std::string& query) const {
    std::vector<Recipe> out;
    std::string q = lower(query);
    for (const auto& r : recipes)
        if (lower(r.name).find(q) != std::string::npos) out.push_back(r);
    return out;
}

std::vector<Recipe> RecipeEngine::searchByIngredient(const std::string& ing) const {
    std::vector<Recipe> out;
    auto it = ingredientIndex.find(lower(ing));
    if (it == ingredientIndex.end()) return out;
    for (int idx : it->second) out.push_back(recipes[idx]);
    return out;
}

std::vector<std::pair<std::string, double>>
RecipeEngine::generateShoppingList(int recipeId) const {
    std::vector<std::pair<std::string, double>> out;
    for (const auto& r : recipes) {
        if (r.id != recipeId) continue;
        for (const auto& ing : r.ingredients) {
            Product* p = inventory->getProductByName(ing);
            double price = p ? p->price : 0.0;
            out.push_back({ing, price});
        }
        break;
    }
    return out;
}

void RecipeEngine::saveToFile(const std::string& path) const {
    std::vector<std::string> lines;
    for (const auto& r : recipes) {
        std::ostringstream oss;
        oss << r.id << "|" << r.name << "|" << r.prepTimeMinutes << "|"
            << r.category << "|" << r.instructions << "|";
        for (size_t i = 0; i < r.ingredients.size(); ++i) {
            if (i) oss << ",";
            oss << r.ingredients[i];
        }
        lines.push_back(oss.str());
    }
    FileManager::writeLines(path, lines);
}

void RecipeEngine::loadFromFile(const std::string& path) {
    recipes.clear();
    auto lines = FileManager::readLines(path);
    for (const auto& line : lines) {
        auto f = FileManager::split(line, '|');
        if (f.size() < 6) continue;
        Recipe r;
        r.id = std::stoi(f[0]);
        r.name = f[1];
        r.prepTimeMinutes = std::stoi(f[2]);
        r.category = f[3];
        r.instructions = f[4];
        auto ings = FileManager::split(f[5], ',');
        for (auto& ing : ings) {
            std::string t = FileManager::trim(ing);
            if (!t.empty()) r.ingredients.push_back(t);
        }
        recipes.push_back(r);
    }
    reindex();
}
