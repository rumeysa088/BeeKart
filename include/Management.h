#ifndef MANAGEMENT_H
#define MANAGEMENT_H

#include <vector>
#include <string>
#include <unordered_map>
#include "Employee.h"
#include "Supplier.h"
#include "Attendance.h"
#include "Payroll.h"
#include "PurchaseOrder.h"
#include "Inventory.h"
#include "Analytics.h"

class Management {
private:
    std::vector<Employee> employees;
    std::vector<Supplier> suppliers;
    std::vector<AttendanceRecord> attendance;
    std::vector<PayrollRecord> payroll;
    std::vector<PurchaseOrder> purchaseOrders;

    int nextEmployeeId;
    int nextSupplierId;
    int nextPOId;

public:
    Management()
        : nextEmployeeId(1), nextSupplierId(1), nextPOId(1) {}

    int addEmployee(const std::string& name,
                    const std::string& role,
                    const std::string& phone,
                    double salary,
                    const std::string& joinDate);
    bool removeEmployee(int id);
    bool updateSalary(int id, double newSalary);
    Employee* findEmployee(int id);
    std::vector<Employee> allEmployees() const;
    std::vector<Employee> activeEmployees() const;

    void markAttendance(int employeeId,
                        const std::string& date,
                        const std::string& checkIn,
                        const std::string& checkOut,
                        bool present);
    std::vector<AttendanceRecord> attendanceFor(int employeeId) const;
    std::vector<AttendanceRecord> attendanceOn(const std::string& date) const;
    double totalHours(int employeeId, const std::string& month) const;

    PayrollRecord computePayroll(int employeeId,
                                 const std::string& month,
                                 double bonusPercent = 0.0) const;
    void runPayroll(const std::string& month, double bonusPercent = 0.0);
    std::vector<PayrollRecord> payrollFor(const std::string& month) const;

    int addSupplier(const std::string& name,
                    const std::string& contact,
                    const std::string& phone,
                    const std::string& email);
    bool linkSupplierToProduct(int supplierId, int productId);
    Supplier* findSupplier(int id);
    std::vector<Supplier> allSuppliers() const;

    int createPurchaseOrder(int supplierId,
                            const std::vector<POItem>& items,
                            const std::string& date);
    bool markPOReceived(int poId);
    bool cancelPO(int poId);
    std::vector<PurchaseOrder> allPurchaseOrders() const;
    std::vector<PurchaseOrder> pendingPOs() const;

    int autoGenerateOrders(const Analytics& analytics,
                           const Inventory& inventory,
                           const std::string& date);

    void printEmployeeReport() const;
    void printAttendanceReport(const std::string& month) const;
    void printPayrollReport(const std::string& month) const;
    void printSupplierReport() const;
    void printPOReport() const;

    void saveAll(const std::string& baseDir) const;
    void loadAll(const std::string& baseDir);
};

#endif