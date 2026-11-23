#include "CsvImporter.h"
#include "Analytics.h"
#include "Sale.h"
#include "FileManager.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <iostream>

static std::string lower(const std::string& s) {
    std::string out = s;
    std::transform(out.begin(), out.end(), out.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return out;
}

static std::string stripQuotes(const std::string& s) {
    std::string t = s;
    if (!t.empty() && t.front() == '"') t.erase(t.begin());
    if (!t.empty() && t.back() == '"') t.pop_back();
    return FileManager::trim(t);
}

std::vector<std::string> CsvImporter::parseCsvLine(const std::string& line) {
    std::vector<std::string> fields;
    std::string current;
    bool inQuotes = false;

    for (size_t i = 0; i < line.size(); ++i) {
        char c = line[i];
        if (c == '"') {
            if (inQuotes && i + 1 < line.size() && line[i + 1] == '"') {
                current += '"';
                i++;
            } else {
                inQuotes = !inQuotes;
            }
        } else if (c == ',' && !inQuotes) {
            fields.push_back(stripQuotes(current));
            current.clear();
        } else {
            current += c;
        }
    }
    fields.push_back(stripQuotes(current));
    return fields;
}

int CsvImporter::columnIndex(const std::vector<std::string>& headers,
                             const std::string& name) {
    std::string target = lower(name);
    for (size_t i = 0; i < headers.size(); ++i) {
        if (lower(headers[i]) == target) return (int)i;
    }
    return -1;
}

int CsvImporter::importProducts(const std::string& csvPath,
                                Inventory& inventory) {
    std::ifstream file(csvPath);
    if (!file.is_open()) {
        std::cerr << "  [!] Cannot open " << csvPath << "\n";
        return 0;
    }

    std::string headerLine;
    std::getline(file, headerLine);
    auto headers = parseCsvLine(headerLine);

    int idxName = columnIndex(headers, "Product_Name");
    if (idxName < 0) idxName = columnIndex(headers, "product_name");
    if (idxName < 0) idxName = columnIndex(headers, "name");
    if (idxName < 0) idxName = columnIndex(headers, "item");

    int idxCat = columnIndex(headers, "Category");
    if (idxCat < 0) idxCat = columnIndex(headers, "category");
    if (idxCat < 0) idxCat = columnIndex(headers, "aisle");

    int idxPrice = columnIndex(headers, "Unit_Price");
    if (idxPrice < 0) idxPrice = columnIndex(headers, "unit_price");
    if (idxPrice < 0) idxPrice = columnIndex(headers, "price");

    int idxStock = columnIndex(headers, "Stock_Quantity");
    if (idxStock < 0) idxStock = columnIndex(headers, "stock_quantity");
    if (idxStock < 0) idxStock = columnIndex(headers, "quantity");
    if (idxStock < 0) idxStock = columnIndex(headers, "stock");

    int idxExpiry = columnIndex(headers, "Expiration_Date");
    if (idxExpiry < 0) idxExpiry = columnIndex(headers, "expiry_date");

    int idxReorder = columnIndex(headers, "Reorder_Level");
    if (idxReorder < 0) idxReorder = columnIndex(headers, "reorder_level");

    int idxSupplier = columnIndex(headers, "Supplier_Name");
    if (idxSupplier < 0) idxSupplier = columnIndex(headers, "supplier_name");
    if (idxSupplier < 0) idxSupplier = columnIndex(headers, "supplier");

    if (idxName < 0 || idxPrice < 0) {
        std::cerr << "  [!] CSV missing required name/price columns.\n";
        return 0;
    }

    int count = 0;
    std::string line;
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        auto f = parseCsvLine(line);
        auto safe = [&](int idx) -> std::string {
            if (idx < 0 || idx >= (int)f.size()) return "";
            return f[idx];
        };

        std::string name = safe(idxName);
        if (name.empty()) continue;

        std::string priceStr = safe(idxPrice);
        priceStr.erase(std::remove(priceStr.begin(), priceStr.end(), '$'),
                       priceStr.end());
        priceStr.erase(std::remove(priceStr.begin(), priceStr.end(), ','),
                       priceStr.end());
        double price = 100.0;
        try { if (!priceStr.empty()) price = std::stod(priceStr); }
        catch (...) { price = 100.0; }
        if (price < 5.0) price = price * 250.0;

        int stock = 20;
        try { if (!safe(idxStock).empty()) stock = std::stoi(safe(idxStock)); }
        catch (...) {}

        int reorder = 10;
        try { if (!safe(idxReorder).empty()) reorder = std::stoi(safe(idxReorder)); }
        catch (...) {}

        std::string cat = safe(idxCat);
        if (cat.empty()) cat = "General";
        std::string exp = safe(idxExpiry);
        std::string sup = safe(idxSupplier);
        if (sup.empty()) sup = "General";

        inventory.addProduct(name, cat, price, stock, exp, reorder, sup);
        count++;
    }

    std::cout << "  [+] Imported " << count << " products from "
              << csvPath << "\n";
    return count;
}

int CsvImporter::importRecipes(const std::string& csvPath,
                               RecipeEngine& recipeEngine) {
    std::ifstream file(csvPath);
    if (!file.is_open()) {
        std::cerr << "  [!] Cannot open " << csvPath << "\n";
        return 0;
    }

    std::string headerLine;
    std::getline(file, headerLine);
    auto headers = parseCsvLine(headerLine);

    int idxName = columnIndex(headers, "name");
    if (idxName < 0) idxName = columnIndex(headers, "Title");
    if (idxName < 0) idxName = columnIndex(headers, "recipe");

    int idxIng = columnIndex(headers, "ingredients");
    if (idxIng < 0) idxIng = columnIndex(headers, "Ingredients");

    int idxInst = columnIndex(headers, "instructions");
    if (idxInst < 0) idxInst = columnIndex(headers, "Instructions");
    if (idxInst < 0) idxInst = columnIndex(headers, "summary");

    int idxCat = columnIndex(headers, "cuisine");
    if (idxCat < 0) idxCat = columnIndex(headers, "Category");
    if (idxCat < 0) idxCat = columnIndex(headers, "course");

    int idxTime = columnIndex(headers, "prep_time");
    if (idxTime < 0) idxTime = columnIndex(headers, "Times");
    if (idxTime < 0) idxTime = columnIndex(headers, "time");

    if (idxName < 0 || idxIng < 0) {
        std::cerr << "  [!] CSV missing name/ingredients columns.\n";
        return 0;
    }

    int count = 0;
    std::string line;
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        auto f = parseCsvLine(line);
        auto safe = [&](int idx) -> std::string {
            if (idx < 0 || idx >= (int)f.size()) return "";
            return f[idx];
        };

        std::string name = safe(idxName);
        if (name.empty()) continue;

        std::string ingStr = safe(idxIng);
        for (char& c : ingStr)
            if (c == '|' || c == ';') c = ',';

        std::vector<std::string> ings;
        std::stringstream ss(ingStr);
        std::string token;
        while (std::getline(ss, token, ',')) {
            std::string t = stripQuotes(token);
            std::string clean;
            bool inWord = false;
            for (size_t i = 0; i < t.size(); ++i) {
                char c = t[i];
                if (std::isalpha((unsigned char)c) || c == ' ') {
                    if (inWord || std::isalpha((unsigned char)c)) {
                        clean += c;
                        inWord = true;
                    }
                } else {
                    if (inWord) clean += ' ';
                }
            }
            clean = FileManager::trim(clean);
            std::stringstream cs(clean);
            std::string w1, w2;
            cs >> w1;
            if (cs >> w2) clean = w1 + " " + w2;
            else clean = w1;
            if (!clean.empty() && clean.size() > 2)
                ings.push_back(clean);
        }

        if (ings.empty()) continue;

        std::string inst = safe(idxInst);
        if (inst.empty()) inst = "Cook as desired.";
        std::string cat = safe(idxCat);
        if (cat.empty()) cat = "Dinner";

        int prepTime = 30;
        try { if (!safe(idxTime).empty()) prepTime = std::stoi(safe(idxTime)); }
        catch (...) {}

        recipeEngine.addRecipe(name, ings, inst, prepTime, cat);
        count++;
    }

    std::cout << "  [+] Imported " << count << " recipes from "
              << csvPath << "\n";
    return count;
}

int CsvImporter::importStores(const std::string& csvPath,
                              StoreLocator& locator,
                              Inventory& inventory,
                              double priceMultiplier) {
    std::ifstream file(csvPath);
    if (!file.is_open()) {
        std::cerr << "  [!] Cannot open " << csvPath << "\n";
        return 0;
    }

    std::string headerLine;
    std::getline(file, headerLine);
    auto headers = parseCsvLine(headerLine);

    int idxName = columnIndex(headers, "name");
    if (idxName < 0) idxName = columnIndex(headers, "store_name");
    int idxAddr = columnIndex(headers, "address");
    if (idxAddr < 0) idxAddr = columnIndex(headers, "location");
    if (idxAddr < 0) idxAddr = columnIndex(headers, "city");
    int idxLat = columnIndex(headers, "latitude");
    if (idxLat < 0) idxLat = columnIndex(headers, "lat");
    int idxLon = columnIndex(headers, "longitude");
    if (idxLon < 0) idxLon = columnIndex(headers, "lon");
    if (idxLon < 0) idxLon = columnIndex(headers, "lng");
    int idxRating = columnIndex(headers, "rating");

    if (idxName < 0 || idxLat < 0 || idxLon < 0) {
        std::cerr << "  [!] CSV missing name/lat/lon columns.\n";
        return 0;
    }

    auto products = inventory.allProducts();
    int count = 0;
    std::string line;
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        auto f = parseCsvLine(line);
        auto safe = [&](int idx) -> std::string {
            if (idx < 0 || idx >= (int)f.size()) return "";
            return f[idx];
        };

        std::string name = safe(idxName);
        if (name.empty()) continue;

        double lat = 0, lon = 0;
        try { lat = std::stod(safe(idxLat)); } catch (...) { continue; }
        try { lon = std::stod(safe(idxLon)); } catch (...) { continue; }

        double rating = 4.0;
        try { if (!safe(idxRating).empty()) rating = std::stod(safe(idxRating)); }
        catch (...) {}

        int sid = locator.addStore(name, safe(idxAddr), lat, lon, rating);

        for (const auto& p : products) {
            double variation = 1.0 + (((p.id * 7 + sid * 13) % 21) - 10) / 100.0;
            double price = p.price * priceMultiplier * variation;
            if (price < 10) price = 10;
            locator.setPrice(sid, p.id, price);
        }
        count++;
    }

    std::cout << "  [+] Imported " << count << " stores from "
              << csvPath << "\n";
    return count;
}

int CsvImporter::importSales(const std::string& csvPath,
                             Inventory& inventory,
                             Analytics& analytics) {
    std::ifstream file(csvPath);
    if (!file.is_open()) {
        std::cerr << "  [!] Cannot open " << csvPath << "\n";
        return 0;
    }

    std::string headerLine;
    std::getline(file, headerLine);
    auto headers = parseCsvLine(headerLine);

    int idxDate = columnIndex(headers, "date");
    if (idxDate < 0) idxDate = columnIndex(headers, "transaction_date");
    int idxCust = columnIndex(headers, "customer_id");
    if (idxCust < 0) idxCust = columnIndex(headers, "customer_name");
    int idxProd = columnIndex(headers, "product_name");
    if (idxProd < 0) idxProd = columnIndex(headers, "Product_Name");
    int idxQty = columnIndex(headers, "quantity");
    if (idxQty < 0) idxQty = columnIndex(headers, "Quantity");
    int idxPrice = columnIndex(headers, "unit_price");
    if (idxPrice < 0) idxPrice = columnIndex(headers, "Unit_Price");

    if (idxProd < 0 || idxQty < 0) {
        std::cerr << "  [!] CSV missing product/quantity columns.\n";
        return 0;
    }

    int count = 0;
    int saleIdCounter = 100000;
    std::string line;
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        auto f = parseCsvLine(line);
        auto safe = [&](int idx) -> std::string {
            if (idx < 0 || idx >= (int)f.size()) return "";
            return f[idx];
        };

        std::string prodName = safe(idxProd);
        if (prodName.empty()) continue;

        int qty = 1;
        try { qty = std::stoi(safe(idxQty)); } catch (...) { continue; }

        double price = 100.0;
        try { if (!safe(idxPrice).empty()) price = std::stod(safe(idxPrice)); }
        catch (...) {}

        Sale s;
        s.id = saleIdCounter++;
        s.date = safe(idxDate).empty() ? "2026-01-01" : safe(idxDate);
        s.customerName = safe(idxCust).empty() ? "walk-in" : safe(idxCust);
        s.paymentMethod = "cash";

        SaleItem item;
        item.productName = prodName;
        int pid = inventory.findIdByName(prodName);
        item.productId = (pid >= 0) ? pid : 0;
        item.quantity = qty;
        item.unitPrice = (pid >= 0 && inventory.getProduct(pid))
                         ? inventory.getProduct(pid)->price : price;
        item.lineTotal = item.quantity * item.unitPrice;

        s.items.push_back(item);
        s.subtotal = item.lineTotal;
        s.tax = s.subtotal * 0.05;
        s.total = s.subtotal + s.tax;

        analytics.addSale(s);
        count++;
    }

    std::cout << "  [+] Imported " << count << " sales transactions from "
              << csvPath << "\n";
    return count;
}

int CsvImporter::importSuppliers(const std::string& csvPath,
                                 Management& management) {
    std::ifstream file(csvPath);
    if (!file.is_open()) {
        std::cerr << "  [!] Cannot open " << csvPath << "\n";
        return 0;
    }

    std::string headerLine;
    std::getline(file, headerLine);
    auto headers = parseCsvLine(headerLine);

    int idxName = columnIndex(headers, "name");
    if (idxName < 0) idxName = columnIndex(headers, "supplier_name");
    int idxContact = columnIndex(headers, "contact_person");
    if (idxContact < 0) idxContact = columnIndex(headers, "contact");
    int idxPhone = columnIndex(headers, "phone");
    int idxEmail = columnIndex(headers, "email");

    if (idxName < 0) {
        std::cerr << "  [!] CSV missing name column.\n";
        return 0;
    }

    int count = 0;
    std::string line;
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        auto f = parseCsvLine(line);
        auto safe = [&](int idx) -> std::string {
            if (idx < 0 || idx >= (int)f.size()) return "";
            return f[idx];
        };
        std::string name = safe(idxName);
        if (name.empty()) continue;

        management.addSupplier(name,
                               safe(idxContact),
                               safe(idxPhone),
                               safe(idxEmail));
        count++;
    }

    std::cout << "  [+] Imported " << count << " suppliers from "
              << csvPath << "\n";
    return count;
}
