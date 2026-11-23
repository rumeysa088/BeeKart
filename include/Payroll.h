#ifndef PAYROLL_H
#define PAYROLL_H

#include <string>

struct PayrollRecord {
    int employeeId;
    std::string employeeName;
    std::string month;
    double baseSalary;
    double hoursWorked;
    double deductions;
    double bonus;
    double netPay;

    PayrollRecord()
        : employeeId(0), baseSalary(0.0), hoursWorked(0.0),
          deductions(0.0), bonus(0.0), netPay(0.0) {}
};

#endif