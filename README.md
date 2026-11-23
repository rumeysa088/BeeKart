# BeeKart (SmartMart)

A console-based supermarket management system written in C++17, built as a Data Structures & Algorithms project. It combines a point-of-sale system, inventory tracking, sales analytics, a recipe recommender, and a "find the best store for my shopping list" assistant.

## Features

| Menu option | What it does |
|---|---|
| **1. Point of Sale** | Add items to a cart, apply 5% tax, pay by cash or credit, print a receipt file, and see "customers also bought" suggestions |
| **2. Recipe Suggestions** | *Cook What You Have* (recipes you can make with few missing ingredients), *Expiry Rescue* (recipes that use items about to expire), and recipe search |
| **3. Inventory** | View all products, add a product, list low-stock items, list items expiring within 7 days |
| **4. Analytics Dashboard** | Top sellers, credit-risk ranking of customers, smart reorder alerts |
| **5. Smart Shopping Assistant** | Enter a shopping list and your location; get matching recipes and nearby stores ranked by price and distance |
| **6. Management** | Employees, attendance, payroll, suppliers, purchase orders, auto-generated orders, reports |
| **7. Save & Exit** | Saves all data to the `data/` folder |

## Data structures and algorithms used

- **Hash maps** (`unordered_map`) for O(1) average product lookup by ID and by lowercase name
- **Inverted index** (ingredient -> list of recipes) for fast recipe search and expiry rescue
- **Hash sets** for fast "do I already have this ingredient?" checks
- **Pair-counting in a hash map** for market-basket analysis (how often two products are bought together)
- **Sorting with custom comparators** for rankings (top sellers, risk, reorder urgency, stores, recipes)
- **Haversine formula** for the distance between two latitude/longitude points
- **Weighted scoring** to rank stores by a mix of distance and total basket cost

## Project structure

```
BeeKart/
├── include/      Header files (.h): class and struct declarations
├── src/          Source files (.cpp): implementations and main.cpp
├── data/         Saved data (.dat files), created and updated by the program
├── scripts/      seed.sh: resets the sample data
├── ide/          Dev-C++ project files (optional, Windows)
├── Makefile      Build instructions
└── README.md
```

These folders are created automatically the first time the program runs: `receipts/` (one text receipt per sale), `shopping_lists/`, and `kaggle/`.

## Requirements

- A C++17 compiler (**g++** 7 or newer recommended)
- **make** (optional but recommended)

## Build and run

### Linux / macOS

```bash
# from the project root folder
make            # compiles everything into ./smartmart
./smartmart     # run the program
```

Or build and run in one step:

```bash
make run
```

To delete compiled files and start a fresh build:

```bash
make clean
```

### Windows (MinGW / MSYS2)

Open a terminal in the project folder and run:

```bash
mingw32-make
smartmart.exe
```

If you only have g++ and no make, compile by listing the source files:

```bash
g++ -std=c++17 -O2 -Iinclude src/main.cpp src/Inventory.cpp src/Cart.cpp src/RecipeEngine.cpp src/Analytics.cpp src/FileManager.cpp src/StoreLocator.cpp src/ShoppingAssistant.cpp src/Management.cpp src/CsvImporter.cpp -o smartmart.exe
```

### Dev-C++

Compile all `.cpp` files in `src/` with the include path set to `include/`, and use the C++17 option. The old project files are kept in `ide/`.

> **Important:** run the program from the project root folder, because it reads and writes `data/` and `receipts/` using paths relative to where you start it.

## First run

On the first run the `data/` folder is empty, so the program fills it with sample data automatically: 53 products, 16 recipes, 20 stores with prices, employees, and suppliers. Use option **7** to exit properly, because that is when everything is saved.

To reset everything back to the sample data at any time (Linux / macOS / Git Bash):

```bash
bash scripts/seed.sh
```

## Example session

Try the Smart Shopping Assistant (option **5**) with these inputs:

```
Choice: 5
> Milk 2
> Eggs 1
> done
Your latitude  : 24.86
Your longitude : 67.01
Search radius (km): 10
```

The program lists which items it found, recipes you could make, and the nearby stores ranked from best to worst, with a recommended store at the end.

In the Point of Sale (option **1**), type a product name, enter a quantity, and type `done` to check out. Type `view` to see the cart or `remove <name>` to remove an item.

## Data files

All data is stored as plain text, one record per line, with fields separated by `|`.

| File | Contents |
|---|---|
| `products.dat` | Inventory |
| `recipes.dat` | Recipes and their ingredients |
| `sales.dat` | Completed sales (used for analytics) |
| `customers.dat` | Credit customers and payment history |
| `stores.dat`, `store_prices.dat` | Stores and their prices |
| `employees.dat`, `attendance.dat` | Staff records |
| `suppliers.dat`, `purchase_orders.dat` | Suppliers and orders |

## Known limitations

- Reorder alerts estimate "days" from the number of sales, not real calendar dates.
- Credit payment records currently use a fixed date.
- Number prompts do not validate input, so typing letters where a number is expected can break the menu.
- `CsvImporter` is included but not yet connected to a menu option.
