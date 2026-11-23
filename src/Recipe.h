#ifndef RECIPE_H
#define RECIPE_H

#include <string>
#include <vector>

struct Recipe {
    int id;
    std::string name;
    std::vector<std::string> ingredients;
    std::string instructions;
    int prepTimeMinutes;
    std::string category;

    Recipe() : id(0), prepTimeMinutes(0) {}
};

#endif
