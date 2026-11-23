#ifndef EMPLOYEE_H
#define EMPLOYEE_H

#include <string>

struct Employee {
    int id;
    std::string name;
    std::string role;
    std::string phone;
    double monthlySalary;
    std::string joinDate;
    bool active;

    Employee() : id(0), monthlySalary(0.0), active(true) {}
};

#endif
