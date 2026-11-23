#ifndef SUPPLIER_H
#define SUPPLIER_H

#include <string>
#include <vector>

struct Supplier {
    int id;
    std::string name;
    std::string contactPerson;
    std::string phone;
    std::string email;
    std::vector<int> productIds;

    Supplier() : id(0) {}
};

#endif