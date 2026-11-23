#include <iostream>
#include <iomanip>
#include <limits>
#include <fstream>
#include <sstream>
#include <cctype>

#include "Inventory.h"
#include "Cart.h"
#include "RecipeEngine.h"
#include "Analytics.h"
#include "FileManager.h"
#include "Customer.h"
#include "StoreLocator.h"
#include "ShoppingAssistant.h"
#include "Management.h"
#include "CsvImporter.h"

using namespace std;

static Inventory inventory;
static Cart cart(&inventory);
static RecipeEngine recipeEngine(&inventory);
static Analytics analytics;
static vector<Customer> customers;
static StoreLocator storeLocator;
static Management management;
static ShoppingAssistant* assistant = nullptr;
static int nextSaleId = 1;

// ---------------- Utilities ----------------
static void pause() {
    cout << "\nPress Enter to continue...";
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
}

static string promptLine(const string& label) {
    cout << label;
    string s;
    getline(cin, s);
    return FileManager::trim(s);
}

static int promptInt(const string& label) {
    cout << label;
    int v; cin >> v; cin.ignore();
    return v;
}

static double promptDouble(const string& label) {
    cout << label;
    double v; cin >> v; cin.ignore();
    return v;
}

// ---------------- Customer persistence ----------------
static void saveCustomers(const string& path) {
    vector<string> lines;
    for (const auto& c : customers) {
        ostringstream oss;
        oss << c.id << "|" << c.name << "|" << c.phone << "|"
            << c.creditBalance << "|" << c.creditLimit << "|";
        for (size_t i = 0; i < c.paymentHistory.size(); ++i) {
            if (i) oss << ";";
            oss << c.paymentHistory[i].date << ","
                << c.paymentHistory[i].amount << ","
                << (c.paymentHistory[i].onTime ? 1 : 0);
        }
        lines.push_back(oss.str());
    }
    FileManager::writeLines(path, lines);
}

static void loadCustomers(const string& path) {
    customers.clear();
    auto lines = FileManager::readLines(path);
    for (const auto& line : lines) {
        auto f = FileManager::split(line, '|');
        if (f.size() < 6) continue;
        Customer c;
        c.id = stoi(f[0]);
        c.name = f[1];
        c.phone = f[2];
        c.creditBalance = stod(f[3]);
        c.creditLimit = stod(f[4]);
        auto recs = FileManager::split(f[5], ';');
        for (const auto& r : recs) {
            auto p = FileManager::split(r, ',');
            if (p.size() < 3) continue;
            PaymentRecord pr;
            pr.date = p[0];
            pr.amount = stod(p[1]);
            pr.onTime = (p[2] == "1");
            c.paymentHistory.push_back(pr);
        }
        customers.push_back(c);
    }
}

static Customer* findCustomer(const string& name) {
    for (auto& c : customers) if (c.name == name) return &c;
    return nullptr;
}

// ---------------- Screens ----------------
static void screenPOS() {
    cout << "\n========== POINT OF SALE ==========\n";
    cout << "Type item name to add. 'done' to checkout. 'view' to see cart.\n";
    cout << "Type 'remove <name>' to remove.\n\n";

    while (true) {
        cout << "Item: ";
        string input;
        getline(cin, input);
        input = FileManager::trim(input);
        if (input == "done") break;
        if (input == "view") {
            cout << "\n--- CART ---\n";
            for (const auto& l : cart.getLines())
                cout << "  " << l.name << " x" << l.quantity
                          << " = PKR " << (int)(l.quantity * l.unitPrice) << "\n";
            cout << "  Subtotal: PKR " << (int)cart.subtotal() << "\n\n";
            continue;
        }
        if (input.rfind("remove ", 0) == 0) {
            string name = input.substr(7);
            Product* p = inventory.getProductByName(name);
            if (p && cart.removeItem(p->id))
                cout << "  Removed: " << name << "\n";
            else
                cout << "  Not in cart.\n";
            continue;
        }

        Product* p = inventory.getProductByName(input);
        if (!p) { cout << "  Item not found.\n"; continue; }
        cout << "  " << p->name << " - PKR " << p->price
                  << " [Stock: " << p->stock << "]\n";
        int qty = promptInt("  Quantity: ");
        if (cart.addItem(p->id, qty)) cout << "  Added.\n";
        else cout << "  Failed.\n";

        auto sug = analytics.suggestionsFor(p->id, 3);
        if (!sug.empty()) {
            cout << "\n  Customers who bought " << p->name
                      << " also bought:\n";
            for (const auto& s : sug) {
                int oid = stoi(s.item);
                Product* op = inventory.getProduct(oid);
                if (op)
                    cout << "   - " << op->name << "  ("
                              << (int)(s.confidence * 100) << "%)\n";
            }
        }
    }

    if (cart.empty()) { cout << "\nCart empty.\n"; pause(); return; }

    cout << "\n--- CART SUMMARY ---\n";
    for (const auto& l : cart.getLines())
        cout << "  " << l.name << " x" << l.quantity
                  << " = PKR " << (int)(l.quantity * l.unitPrice) << "\n";
    cout << "  Subtotal: PKR " << (int)cart.subtotal() << "\n";
    cout << "  Tax (5%): PKR " << (int)cart.tax() << "\n";
    cout << "  TOTAL:    PKR " << (int)cart.total() << "\n";

    string custName = promptLine("\nCustomer name (or 'walk-in'): ");
    if (custName == "walk-in" || custName.empty()) custName = "walk-in";

    cout << "Payment method (1=Cash, 2=Credit): ";
    int pm; cin >> pm; cin.ignore();
    string paymentMethod = (pm == 2) ? "credit" : "cash";

    if (paymentMethod == "credit" && custName != "walk-in") {
        Customer* c = findCustomer(custName);
        if (!c) {
            Customer nc;
            nc.id = (int)customers.size() + 1;
            nc.name = custName;
            nc.phone = promptLine("  Phone: ");
            customers.push_back(nc);
            c = &customers.back();
        }
        if (c->isRisky()) {
            cout << "\n? CREDIT WARNING: " << c->name
                      << " has paid late often.\n";
            string ov = promptLine("  Override? (Y/N): ");
            if (ov != "Y" && ov != "y") paymentMethod = "cash";
        }
        if (paymentMethod == "credit") {
            c->creditBalance += cart.total();
            PaymentRecord pr;
            pr.date = "2026-09-30";
            pr.amount = cart.total();
            pr.onTime = true;
            c->paymentHistory.push_back(pr);
        }
    }

    Sale s = cart.checkout(custName, paymentMethod, nextSaleId++);
    analytics.addSale(s);

    ostringstream rpath;
    rpath << "receipts/receipt_" << s.id << ".txt";
    vector<string> rlines;
    rlines.push_back("===== SMARTMART RECEIPT =====");
    rlines.push_back("Sale ID: " + to_string(s.id));
    rlines.push_back("Date: " + s.date);
    rlines.push_back("Customer: " + s.customerName);
    rlines.push_back("Payment: " + s.paymentMethod);
    rlines.push_back("-----------------------------");
    for (const auto& it : s.items) {
        ostringstream l;
        l << it.productName << " x" << it.quantity
          << " = PKR " << (int)it.lineTotal;
        rlines.push_back(l.str());
    }
    rlines.push_back("-----------------------------");
    rlines.push_back("Subtotal: PKR " + to_string((int)s.subtotal));
    rlines.push_back("Tax:      PKR " + to_string((int)s.tax));
    rlines.push_back("TOTAL:    PKR " + to_string((int)s.total));
    FileManager::writeLines(rpath.str(), rlines);

    cout << "\nSale completed. Receipt saved to " << rpath.str() << "\n";

    auto alerts = analytics.reorderAlerts(inventory);
    if (!alerts.empty()) {
        cout << "\n--- SMART REORDER ALERTS ---\n";
        for (const auto& a : alerts)
            cout << "  ? " << a.name << ": stock " << a.currentStock
                      << ", runs out in " << fixed << setprecision(1)
                      << a.daysUntilStockout << " days. Order ~"
                      << a.recommendedOrder << "\n";
    }
    pause();
}

static void screenRecipes() {
    cout << "\n========== RECIPE SUGGESTIONS ==========\n";
    cout << "1. Cook What You Have\n2. Expiry Rescue\n3. Search\n";
    int choice = promptInt("Choice: ");

    if (choice == 1) {
        vector<string> have;
        cout << "Ingredients you have (blank to finish):\n";
        while (true) {
            string s = promptLine("> ");
            if (s.empty()) break;
            have.push_back(s);
        }
        auto sug = recipeEngine.suggestByAvailable(have, 3);
        if (sug.empty()) { cout << "None found.\n"; pause(); return; }
        for (size_t i = 0; i < sug.size(); ++i) {
            cout << "\n" << (i + 1) << ". " << sug[i].recipe.name
                      << "  (need " << sug[i].missingCount << " more)\n";
            for (const auto& m : sug[i].missingItems)
                cout << "     - " << m << "\n";
            cout << "   Extra cost: PKR " << (int)sug[i].estimatedCost << "\n";
        }
        int pick = promptInt("\nAdd missing items of # to cart (0=skip): ");
        if (pick > 0 && pick <= (int)sug.size()) {
            for (const auto& m : sug[pick - 1].missingItems) {
                Product* p = inventory.getProductByName(m);
                if (p && cart.addItem(p->id, 1))
                    cout << "  Added: " << p->name << "\n";
            }
        }
    }
    else if (choice == 2) {
        auto resc = recipeEngine.expiryRescueSuggestions(5);
        if (resc.empty()) cout << "Nothing expiring soon.\n";
        for (const auto& r : resc) {
            cout << "\n? " << r.expiringProduct
                      << " expires in " << r.daysLeft << " days.\n";
            for (const auto& rec : r.recipesUsingIt)
                cout << "   - " << rec.name << "\n";
        }
    }
    else if (choice == 3) {
        string q = promptLine("Search by name: ");
        auto res = recipeEngine.searchByName(q);
        for (const auto& r : res) {
            cout << "\n" << r.name << " (" << r.category << ", "
                      << r.prepTimeMinutes << " min)\n  Ingredients: ";
            for (size_t i = 0; i < r.ingredients.size(); ++i) {
                if (i) cout << ", ";
                cout << r.ingredients[i];
            }
            cout << "\n  " << r.instructions << "\n";
        }
    }
    pause();
}

static void screenInventory() {
    cout << "\n========== INVENTORY ==========\n";
    cout << "1. View all\n2. Add product\n3. Low stock\n4. Expiring\n";
    int c = promptInt("Choice: ");
    if (c == 1) {
        auto all = inventory.allProducts();
        cout << "\nID  Name              Stock  Price   Expiry\n";
        for (const auto& p : all) {
            cout << left << setw(4) << p.id
                      << setw(18) << p.name
                      << setw(7) << p.stock
                      << setw(8) << (int)p.price
                      << p.expiryDate << "\n";
        }
    }
    else if (c == 2) {
        string name = promptLine("Name: ");
        string cat = promptLine("Category: ");
        double price = promptDouble("Price: ");
        int stock = promptInt("Stock: ");
        string exp = promptLine("Expiry (YYYY-MM-DD or blank): ");
        int low = promptInt("Low-stock threshold: ");
        string sup = promptLine("Supplier: ");
        int id = inventory.addProduct(name, cat, price, stock, exp, low, sup);
        cout << "Added ID " << id << "\n";
    }
    else if (c == 3) {
        auto low = inventory.lowStockProducts();
        if (low.empty()) cout << "None.\n";
        for (const auto& p : low)
            cout << "  " << p.name << " � stock " << p.stock << "\n";
    }
    else if (c == 4) {
        auto exp = inventory.expiringProducts(7);
        if (exp.empty()) cout << "None.\n";
        for (const auto& p : exp)
            cout << "  " << p.name << " � " << p.daysUntilExpiry()
                      << " days left\n";
    }
    pause();
}

static void screenAnalytics() {
    cout << "\n========== ANALYTICS ==========\n";
    cout << "\n--- Top Sellers ---\n";
    for (const auto& t : analytics.topSellers(5)) {
        Product* p = inventory.getProduct(t.first);
        if (p) cout << "  " << p->name << " � " << t.second << " sold\n";
    }
    cout << "\n--- Credit Risk ---\n";
    for (const auto& r : analytics.rankCustomersByRisk(customers))
        cout << "  " << r.customer.name
                  << " � balance " << r.customer.creditBalance
                  << ", risk " << (int)(r.riskScore * 100) << "%\n";
    cout << "\n--- Reorder Alerts ---\n";
    auto alerts = analytics.reorderAlerts(inventory);
    if (alerts.empty()) cout << "  Healthy.\n";
    for (const auto& a : alerts)
        cout << "  " << a.name << " � "
                  << fixed << setprecision(1)
                  << a.daysUntilStockout << " days. Order ~"
                  << a.recommendedOrder << "\n";
    pause();
}

static void screenShoppingAssistant() {
    cout << "\n========== SMART SHOPPING ASSISTANT ==========\n";
    cout << "Enter list. 'done' to finish. Format: <item> <qty>\n";
    vector<pair<string, int>> inputs;
    while (true) {
        string line = promptLine("> ");
        if (line == "done" || line.empty()) break;
        size_t lastSpace = line.find_last_of(' ');
        string name = line;
        int qty = 1;
        if (lastSpace != string::npos) {
            string last = line.substr(lastSpace + 1);
            bool isNum = !last.empty();
            for (char c : last) if (!isdigit((unsigned char)c)) isNum = false;
            if (isNum) {
                qty = stoi(last);
                name = line.substr(0, lastSpace);
            }
        }
        inputs.push_back({name, qty});
    }
    if (inputs.empty()) { cout << "Nothing entered.\n"; pause(); return; }

    double lat = promptDouble("Your latitude  : ");
    double lon = promptDouble("Your longitude : ");
    double radius = promptDouble("Search radius (km): ");
    double weight = 0.3;

    ShoppingPlan plan = assistant->buildPlan(inputs, lat, lon, radius, weight);

    cout << "\n--- YOUR LIST ---\n";
    for (const auto& it : plan.items)
        cout << "  " << (it.found ? "?" : "?") << " "
                  << it.inputName << " x" << it.quantity << "\n";

    cout << "\n--- RECIPES YOU CAN MAKE ---\n";
    if (plan.recipes.empty()) cout << "  None.\n";
    for (size_t i = 0; i < plan.recipes.size() && i < 5; ++i) {
        const auto& r = plan.recipes[i];
        cout << "  " << (i + 1) << ". " << r.recipe.name
                  << "  (need " << r.missingCount << " more";
        if (!r.missingItems.empty()) {
            cout << ": ";
            for (size_t j = 0; j < r.missingItems.size(); ++j) {
                if (j) cout << ", ";
                cout << r.missingItems[j];
            }
        }
        cout << ")\n";
    }

    cout << "\n--- NEARBY STORES ---\n";
    if (plan.storeRankings.empty()) cout << "  None in radius.\n";
    for (size_t i = 0; i < plan.storeRankings.size(); ++i) {
        const auto& r = plan.storeRankings[i];
        cout << "  " << (i + 1) << ". " << r.store.name
                  << "  (" << fixed << setprecision(2)
                  << r.distanceKm << " km, rating " << r.store.rating << ")\n";
        cout << "     Total: PKR " << (int)r.totalCost;
        if (!r.missingItems.empty()) {
            cout << "  [missing: ";
            for (size_t j = 0; j < r.missingItems.size(); ++j) {
                if (j) cout << ", ";
                cout << r.missingItems[j];
            }
            cout << "]";
        }
        cout << "\n";
    }
    if (plan.hasBestStore) {
        cout << "\n? RECOMMENDED: " << plan.bestStore.store.name
                  << "\n   Distance: " << fixed << setprecision(2)
                  << plan.bestStore.distanceKm << " km\n";
        cout << "   Total: PKR " << (int)plan.bestStore.totalCost << "\n";
        cout << "   Address: " << plan.bestStore.store.address << "\n";
    }
    pause();
}

static void screenManagement() {
    cout << "\n========== MANAGEMENT ==========\n";
    cout << "1. Employees\n2. Attendance\n3. Payroll\n";
    cout << "4. Suppliers\n5. Purchase Orders\n";
    cout << "6. Auto-Generate Orders\n7. Reports\n";
    int c = promptInt("Choice: ");

    if (c == 1) {
        cout << "\n1. Add\n2. View\n3. Update salary\n4. Deactivate\n";
        int x = promptInt("Choice: ");
        if (x == 1) {
            string n = promptLine("Name: ");
            string r = promptLine("Role: ");
            string p = promptLine("Phone: ");
            double s = promptDouble("Monthly salary: ");
            string d = promptLine("Join date (YYYY-MM-DD): ");
            int id = management.addEmployee(n, r, p, s, d);
            cout << "Added ID " << id << "\n";
        } else if (x == 2) management.printEmployeeReport();
        else if (x == 3) {
            int id = promptInt("Employee ID: ");
            double s = promptDouble("New salary: ");
            cout << (management.updateSalary(id, s) ? "OK\n" : "Not found\n");
        } else if (x == 4) {
            int id = promptInt("Employee ID: ");
            cout << (management.removeEmployee(id) ? "OK\n" : "Not found\n");
        }
    }
    else if (c == 2) {
        cout << "\n1. Mark\n2. View month\n";
        int x = promptInt("Choice: ");
        if (x == 1) {
            int id = promptInt("Employee ID: ");
            string d = promptLine("Date (YYYY-MM-DD): ");
            string in = promptLine("Check-in (HH:MM): ");
            string out = promptLine("Check-out (HH:MM): ");
            management.markAttendance(id, d, in, out, true);
            cout << "Marked.\n";
        } else {
            string m = promptLine("Month (YYYY-MM): ");
            management.printAttendanceReport(m);
        }
    }
    else if (c == 3) {
        string m = promptLine("Month (YYYY-MM): ");
        double b = promptDouble("Bonus (0.0 - 1.0): ");
        management.runPayroll(m, b);
        management.printPayrollReport(m);
    }
    else if (c == 4) {
        cout << "\n1. Add\n2. View\n";
        int x = promptInt("Choice: ");
        if (x == 1) {
            string n = promptLine("Name: ");
            string cp = promptLine("Contact: ");
            string ph = promptLine("Phone: ");
            string em = promptLine("Email: ");
            int id = management.addSupplier(n, cp, ph, em);
            cout << "Added ID " << id << "\n";
        } else management.printSupplierReport();
    }
    else if (c == 5) management.printPOReport();
    else if (c == 6) {
        string d = promptLine("Date (YYYY-MM-DD): ");
        int created = management.autoGenerateOrders(analytics, inventory, d);
        cout << "Generated " << created << " POs.\n";
        management.printPOReport();
    }
    else if (c == 7) {
        management.printEmployeeReport();
        management.printSupplierReport();
        management.printPOReport();
    }
    pause();
}

// ---------------- Seed data ----------------
static void seedAllIfEmpty() {
    if (inventory.size() == 0) {
        inventory.addProduct("Chicken", "Meat", 350, 20, "2026-10-10", 5, "FreshFarms");
        inventory.addProduct("Beef", "Meat", 650, 15, "2026-10-08", 5, "FreshFarms");
        inventory.addProduct("Mutton", "Meat", 1200, 10, "2026-10-09", 3, "FreshFarms");
        inventory.addProduct("Fish", "Seafood", 500, 12, "2026-10-05", 4, "SeaFresh");
        inventory.addProduct("Prawns", "Seafood", 900, 8, "2026-10-04", 3, "SeaFresh");
        inventory.addProduct("Rice", "Grains", 250, 40, "", 10, "GrainCo");
        inventory.addProduct("Wheat Flour", "Grains", 1100, 25, "", 5, "GrainCo");
        inventory.addProduct("Basmati Rice", "Grains", 380, 30, "", 8, "GrainCo");
        inventory.addProduct("Lentils", "Grains", 220, 35, "", 10, "GrainCo");
        inventory.addProduct("Chickpeas", "Grains", 180, 40, "", 10, "GrainCo");
        inventory.addProduct("Onion", "Vegetables", 100, 50, "2026-10-20", 10, "VeggiesLtd");
        inventory.addProduct("Tomato", "Vegetables", 80, 30, "2026-10-04", 8, "VeggiesLtd");
        inventory.addProduct("Potato", "Vegetables", 60, 60, "2026-10-25", 10, "VeggiesLtd");
        inventory.addProduct("Garlic", "Vegetables", 60, 25, "2026-10-18", 5, "VeggiesLtd");
        inventory.addProduct("Ginger", "Vegetables", 90, 20, "2026-10-18", 5, "VeggiesLtd");
        inventory.addProduct("Carrot", "Vegetables", 70, 30, "2026-10-15", 8, "VeggiesLtd");
        inventory.addProduct("Spinach", "Vegetables", 40, 40, "2026-10-02", 8, "VeggiesLtd");
        inventory.addProduct("Cauliflower", "Vegetables", 120, 20, "2026-10-12", 5, "VeggiesLtd");
        inventory.addProduct("Cabbage", "Vegetables", 70, 25, "2026-10-14", 6, "VeggiesLtd");
        inventory.addProduct("Capsicum", "Vegetables", 150, 20, "2026-10-10", 5, "VeggiesLtd");
        inventory.addProduct("Yogurt", "Dairy", 120, 20, "2026-10-03", 5, "DairyFresh");
        inventory.addProduct("Milk", "Dairy", 210, 45, "2026-10-05", 10, "DairyFresh");
        inventory.addProduct("Eggs", "Dairy", 280, 30, "2026-10-12", 10, "DairyFresh");
        inventory.addProduct("Butter", "Dairy", 380, 12, "2026-11-01", 5, "DairyFresh");
        inventory.addProduct("Cheese", "Dairy", 450, 15, "2026-11-20", 5, "DairyFresh");
        inventory.addProduct("Cream", "Dairy", 220, 10, "2026-10-20", 4, "DairyFresh");
        inventory.addProduct("Bread", "Bakery", 120, 10, "2026-10-02", 10, "BakeryPlus");
        inventory.addProduct("Bun", "Bakery", 30, 40, "2026-10-03", 15, "BakeryPlus");
        inventory.addProduct("Cake", "Bakery", 800, 5, "2026-10-06", 3, "BakeryPlus");
        inventory.addProduct("Cookies", "Bakery", 150, 25, "2026-11-15", 8, "BakeryPlus");
        inventory.addProduct("Spices", "Condiments", 150, 50, "", 10, "SpiceWorld");
        inventory.addProduct("Salt", "Condiments", 40, 60, "", 15, "SpiceWorld");
        inventory.addProduct("Sugar", "Condiments", 180, 40, "", 10, "SpiceWorld");
        inventory.addProduct("Tea", "Beverages", 950, 20, "2027-06-01", 5, "BevCo");
        inventory.addProduct("Coffee", "Beverages", 1200, 15, "2027-08-01", 5, "BevCo");
        inventory.addProduct("Juice", "Beverages", 180, 30, "2026-11-10", 8, "BevCo");
        inventory.addProduct("Soft Drink", "Beverages", 120, 60, "2027-01-15", 15, "BevCo");
        inventory.addProduct("Water Bottle", "Beverages", 60, 100, "2027-06-01", 20, "BevCo");
        inventory.addProduct("Apple", "Fruits", 320, 30, "2026-10-08", 8, "FruitMart");
        inventory.addProduct("Banana", "Fruits", 180, 40, "2026-10-04", 10, "FruitMart");
        inventory.addProduct("Mango", "Fruits", 400, 25, "2026-10-05", 6, "FruitMart");
        inventory.addProduct("Orange", "Fruits", 260, 35, "2026-10-10", 8, "FruitMart");
        inventory.addProduct("Grapes", "Fruits", 380, 20, "2026-10-06", 5, "FruitMart");
        inventory.addProduct("Strawberry", "Fruits", 500, 15, "2026-10-03", 4, "FruitMart");
        inventory.addProduct("Watermelon", "Fruits", 250, 10, "2026-10-07", 3, "FruitMart");
        inventory.addProduct("Pasta", "Snacks", 280, 25, "2027-04-01", 8, "SnackCo");
        inventory.addProduct("Noodles", "Snacks", 120, 40, "2027-04-01", 10, "SnackCo");
        inventory.addProduct("Chips", "Snacks", 80, 50, "2027-01-01", 15, "SnackCo");
        inventory.addProduct("Chocolate", "Snacks", 250, 30, "2027-05-01", 8, "SnackCo");
        inventory.addProduct("Biscuits", "Snacks", 90, 60, "2027-03-01", 15, "SnackCo");
        inventory.addProduct("Soap", "Household", 150, 30, "", 8, "HomeCare");
        inventory.addProduct("Shampoo", "Household", 450, 15, "", 5, "HomeCare");
        inventory.addProduct("Detergent", "Household", 600, 20, "", 6, "HomeCare");
    }

    if (recipeEngine.all().empty()) {
        recipeEngine.addRecipe("Chicken Biryani",
            {"Chicken", "Rice", "Onion", "Tomato", "Yogurt", "Spices"},
            "Cook rice. Fry onions. Add chicken + spices. Layer and steam.",
            60, "Dinner");
        recipeEngine.addRecipe("Chicken Karahi",
            {"Chicken", "Tomato", "Yogurt", "Ginger", "Garlic", "Spices"},
            "Fry chicken with tomato, ginger, garlic. Add yogurt. Simmer.",
            40, "Dinner");
        recipeEngine.addRecipe("Chicken Pulao",
            {"Chicken", "Rice", "Onion", "Spices"},
            "Fry onion. Add chicken + rice + spices. Cook.",
            50, "Dinner");
        recipeEngine.addRecipe("Beef Curry",
            {"Beef", "Onion", "Tomato", "Yogurt", "Spices"},
            "Brown beef. Add onion + tomato + spices. Simmer 1 hour.",
            75, "Dinner");
        recipeEngine.addRecipe("Fish Fry",
            {"Fish", "Spices", "Wheat Flour"},
            "Marinate fish. Coat in flour. Fry until golden.",
            25, "Dinner");
        recipeEngine.addRecipe("Raita",
            {"Yogurt", "Onion", "Tomato"},
            "Mix yogurt with chopped onion and tomato.",
            5, "Side");
        recipeEngine.addRecipe("Omelette",
            {"Eggs", "Onion", "Tomato", "Butter"},
            "Beat eggs. Add onion + tomato. Fry in butter.",
            10, "Breakfast");
        recipeEngine.addRecipe("Garlic Bread",
            {"Bread", "Butter", "Garlic"},
            "Mix butter + garlic. Spread on bread. Toast.",
            10, "Snack");
        recipeEngine.addRecipe("Vegetable Salad",
            {"Tomato", "Onion", "Capsicum", "Carrot"},
            "Chop everything. Mix. Add lemon + salt.",
            10, "Side");
        recipeEngine.addRecipe("Pasta Alfredo",
            {"Pasta", "Cream", "Cheese", "Garlic"},
            "Boil pasta. Make cream sauce. Mix.",
            25, "Dinner");
        recipeEngine.addRecipe("Fruit Salad",
            {"Apple", "Banana", "Orange", "Grapes"},
            "Chop all fruits. Mix. Chill.",
            10, "Snack");
        recipeEngine.addRecipe("Mango Shake",
            {"Mango", "Milk", "Sugar"},
            "Blend mango + milk + sugar.",
            5, "Beverage");
        recipeEngine.addRecipe("Chickpea Curry",
            {"Chickpeas", "Onion", "Tomato", "Spices"},
            "Fry onion. Add tomato + spices. Add chickpeas. Simmer.",
            35, "Dinner");
        recipeEngine.addRecipe("Lentil Soup",
            {"Lentils", "Onion", "Garlic", "Spices"},
            "Boil lentils. Fry onion + garlic. Mix. Simmer.",
            30, "Dinner");
        recipeEngine.addRecipe("Chicken Sandwich",
            {"Bread", "Chicken", "Yogurt", "Capsicum"},
            "Grill chicken. Mix with yogurt. Put on bread.",
            15, "Lunch");
        recipeEngine.addRecipe("Potato Curry",
            {"Potato", "Onion", "Tomato", "Spices"},
            "Fry onion. Add tomato + spices. Add potatoes. Cook.",
            30, "Dinner");
    }

    if (storeLocator.size() == 0) {
        struct Seed { const char* name; const char* addr; double lat, lon, rating, mult; };
        Seed seeds[] = {
            {"Imtiaz Super Market",  "Bahadurabad",     24.8820, 67.0620, 4.2, 1.00},
            {"Chase Up",             "Gulshan",         24.9200, 67.0900, 4.0, 1.05},
            {"Naheed Supermarket",   "Bahadurabad",     24.8830, 67.0580, 4.5, 1.10},
            {"Carrefour",            "DHA Phase 6",     24.8130, 67.0300, 4.4, 0.95},
            {"Hyperstar",            "Clifton",         24.8130, 67.0300, 4.3, 0.98},
            {"Al-Fatah",             "Gulshan",         24.9100, 67.0900, 4.1, 1.03},
            {"Metro Cash & Carry",   "Rashid Minhas",   24.9050, 67.0820, 4.3, 0.92},
            {"Agha's Supermarket",   "Clifton",         24.8200, 67.0330, 4.0, 1.08},
            {"Moti Bazaar Mart",     "Saddar",          24.8600, 67.0200, 3.9, 0.90},
            {"Bombay Bazar",         "Saddar",          24.8580, 67.0180, 3.8, 0.88},
            {"Fresh Mart",           "PECHS",           24.8700, 67.0700, 4.1, 1.00},
            {"Green Valley",         "North Nazimabad", 24.9400, 67.0400, 4.0, 0.95},
            {"Zamzam Mart",          "Nazimabad",       24.9200, 67.0300, 3.9, 0.93},
            {"Super Save",           "Korangi",         24.8300, 67.1300, 3.8, 0.90},
            {"Makro",                "Korangi",         24.8200, 67.1200, 4.2, 0.88},
            {"Noor Supermarket",     "Malir",           24.8900, 67.1900, 3.9, 0.92},
            {"Sabir Superstore",     "FB Area",         24.9500, 67.0600, 4.0, 0.97},
            {"Hilal Supermarket",    "Bahadurabad",     24.8850, 67.0600, 4.1, 1.02},
            {"Karachi Mart",         "Gulshan",         24.9150, 67.0850, 3.9, 0.94},
            {"Food Palace",          "DHA Phase 2",     24.8000, 67.0450, 4.3, 1.05},
        };

        for (const auto& s : seeds)
            storeLocator.addStore(s.name, s.addr, s.lat, s.lon, s.rating);

        auto products = inventory.allProducts();
        int sid = 1;
        for (const auto& s : seeds) {
            for (const auto& p : products) {
                double variation = 1.0 + ((p.id * 7 + sid * 13) % 21 - 10) / 100.0;
                double price = p.price * s.mult * variation;
                if (price < 10) price = 10;
                storeLocator.setPrice(sid, p.id, price);
            }
            sid++;
        }
    }

    if (management.allEmployees().empty()) {
        management.addEmployee("Ahmed Khan", "Manager", "0300-1234567", 80000, "2024-01-15");
        management.addEmployee("Sara Ali", "Cashier", "0301-2345678", 35000, "2024-03-10");
        management.addEmployee("Bilal Ahmed", "Stock Clerk", "0302-3456789", 30000, "2024-06-01");
        management.addEmployee("Fatima Noor", "Cashier", "0303-4567890", 35000, "2025-02-20");
        management.addEmployee("Hassan Raza", "Supervisor", "0304-5678901", 55000, "2023-11-05");

        int s1 = management.addSupplier("FreshFarms", "Mr. Imran", "021-3456789", "imran@fresh.pk");
        int s2 = management.addSupplier("DairyFresh", "Ms. Ayesha", "021-4567890", "ayesha@dairy.pk");
        int s3 = management.addSupplier("VeggiesLtd", "Mr. Kamran", "021-5678901", "kamran@veg.pk");
        int s4 = management.addSupplier("GrainCo", "Mr. Salman", "021-6789012", "salman@grain.pk");
        int s5 = management.addSupplier("BakeryPlus", "Ms. Nadia", "021-7890123", "nadia@bakery.pk");
        int s6 = management.addSupplier("SpiceWorld", "Mr. Asif", "021-8901234", "asif@spice.pk");
        int s7 = management.addSupplier("FruitMart", "Mr. Tariq", "021-9012345", "tariq@fruit.pk");

        management.linkSupplierToProduct(s1, 1);
        management.linkSupplierToProduct(s1, 2);
        management.linkSupplierToProduct(s1, 3);
        management.linkSupplierToProduct(s2, 21);
        management.linkSupplierToProduct(s2, 22);
        management.linkSupplierToProduct(s2, 23);
        management.linkSupplierToProduct(s2, 24);
        management.linkSupplierToProduct(s3, 11);
        management.linkSupplierToProduct(s3, 12);
        management.linkSupplierToProduct(s3, 13);
        management.linkSupplierToProduct(s4, 6);
        management.linkSupplierToProduct(s4, 7);
        management.linkSupplierToProduct(s4, 8);
        management.linkSupplierToProduct(s5, 27);
        management.linkSupplierToProduct(s5, 28);
        management.linkSupplierToProduct(s6, 31);
        management.linkSupplierToProduct(s6, 32);
        management.linkSupplierToProduct(s7, 39);
        management.linkSupplierToProduct(s7, 40);
    }
}

int main() {
    FileManager::ensureDir("data");
    FileManager::ensureDir("receipts");
    FileManager::ensureDir("shopping_lists");
    FileManager::ensureDir("kaggle");

    inventory.loadFromFile("data/products.dat");
    recipeEngine.loadFromFile("data/recipes.dat");
    analytics.loadFromFile("data/sales.dat");
    loadCustomers("data/customers.dat");
    storeLocator.loadFromFile("data/stores.dat", "data/store_prices.dat");
    management.loadAll("data");

    seedAllIfEmpty();

    assistant = new ShoppingAssistant(&inventory, &recipeEngine, &storeLocator);

    while (true) {
        cout << "\n=========================================\n";
        cout << "        SMARTMART MAIN MENU\n";
        cout << "=========================================\n";
        cout << "1. Point of Sale\n";
        cout << "2. Recipe Suggestions\n";
        cout << "3. Inventory\n";
        cout << "4. Analytics Dashboard\n";
        cout << "5. Smart Shopping Assistant\n";
        cout << "6. Management\n";
        cout << "7. Save & Exit\n";
        int choice = promptInt("Choice: ");

        if (choice == 1) screenPOS();
        else if (choice == 2) screenRecipes();
        else if (choice == 3) screenInventory();
        else if (choice == 4) screenAnalytics();
        else if (choice == 5) screenShoppingAssistant();
        else if (choice == 6) screenManagement();
        else if (choice == 7) break;
        else cout << "Invalid choice.\n";
    }

    inventory.saveToFile("data/products.dat");
    recipeEngine.saveToFile("data/recipes.dat");
    analytics.saveToFile("data/sales.dat");
    saveCustomers("data/customers.dat");
    storeLocator.saveToFile("data/stores.dat");
    storeLocator.savePrices("data/store_prices.dat");
    management.saveAll("data");

    delete assistant;
    cout << "\nData saved. Goodbye.\n";
    return 0;
}
