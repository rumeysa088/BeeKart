#ifndef CSV_IMPORTER_H
#define CSV_IMPORTER_H

#include <string>
#include <vector>
#include "Inventory.h"
#include "RecipeEngine.h"
#include "StoreLocator.h"
#include "Management.h"

class Analytics;

class CsvImporter {
private:
    static std::vector<std::string> parseCsvLine(const std::string& line);
    static int columnIndex(const std::vector<std::string>& headers,
                           const std::string& name);

public:
    static int importProducts(const std::string& csvPath, Inventory& inventory);
    static int importRecipes(const std::string& csvPath, RecipeEngine& recipeEngine);
    static int importStores(const std::string& csvPath,
                            StoreLocator& locator,
                            Inventory& inventory,
                            double priceMultiplier);
    static int importSales(const std::string& csvPath,
                           Inventory& inventory,
                           Analytics& analytics);
    static int importSuppliers(const std::string& csvPath,
                               Management& management);
};

#endif