#include "Management.h"
#include "FileManager.h"
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <sstream>

int Management::addEmployee(const std::string& name,
                            const std::string& role,
                            const std::string& phone,
                            double salary,
                            const std::string& joinDate) {
    Employee e;
    e.id = nextEmployeeId++;
    e.name = name;
    e.role = role;
    e.phone = phone;
    e.monthlySalary = salary;
    e.joinDate = joinDate;
    e.active = true;
    employees.push_back(e);
    return e.id;
}

bool Management::removeEmployee(int id) {
    for (auto& e : employees)
        if (e.id == id) { e.active = false; return true; }
    return false;
}

bool Management::updateSalary(int id, double newSalary) {
    for (auto& e : employees)
        if (e.id == id) { e.monthlySalary = newSalary; return true; }
    return false;
}

Employee* Management::findEmployee(int id) {
    for (auto& e : employees) if (e.id == id) return &e;
    return nullptr;
}

std::vector<Employee> Management::allEmployees() const { return employees; }

std::vector<Employee> Management::activeEmployees() const {
    std::vector<Employee> out;
    for (const auto& e : employees) if (e.active) out.push_back(e);
    return out;
}

void Management::markAttendance(int employeeId,
                                const std::string& date,
                                const std::string& checkIn,
                                const std::string& checkOut,
                                bool present) {
    AttendanceRecord r;
    r.employeeId = employeeId;
    r.date = date;
    r.checkIn = checkIn;
    r.checkOut = checkOut;
    r.present = present;
    for (auto& existing : attendance) {
        if (existing.employeeId == employeeId && existing.date == date) {
            existing = r;
            return;
        }
    }
    attendance.push_back(r);
}

std::vector<AttendanceRecord> Management::attendanceFor(int employeeId) const {
    std::vector<AttendanceRecord> out;
    for (const auto& r : attendance)
        if (r.employeeId == employeeId) out.push_back(r);
    return out;
}

std::vector<AttendanceRecord> Management::attendanceOn(
        const std::string& date) const {
    std::vector<AttendanceRecord> out;
    for (const auto& r : attendance)
        if (r.date == date) out.push_back(r);
    return out;
}

double Management::totalHours(int employeeId, const std::string& month) const {
    double total = 0.0;
    for (const auto& r : attendance) {
        if (r.employeeId != employeeId) continue;
        if (r.date.substr(0, 7) != month) continue;
        total += r.hoursWorked();
    }
    return total;
}

PayrollRecord Management::computePayroll(int employeeId,
                                         const std::string& month,
                                         double bonusPercent) const {
    PayrollRecord pr;
    for (const auto& e : employees) {
        if (e.id != employeeId) continue;
        pr.employeeId = e.id;
        pr.employeeName = e.name;
        pr.month = month;
        pr.baseSalary = e.monthlySalary;
        pr.hoursWorked = totalHours(employeeId, month);
        double expectedHours = 176.0;
        double ratio = (pr.hoursWorked > 0)
                       ? std::min(1.0, pr.hoursWorked / expectedHours)
                       : 1.0;
        double gross = e.monthlySalary * ratio;
        pr.deductions = e.monthlySalary - gross;
        pr.bonus = gross * bonusPercent;
        pr.netPay = gross + pr.bonus;
        break;
    }
    return pr;
}

void Management::runPayroll(const std::string& month, double bonusPercent) {
    payroll.clear();
    for (const auto& e : employees) {
        if (!e.active) continue;
        payroll.push_back(computePayroll(e.id, month, bonusPercent));
    }
}

std::vector<PayrollRecord> Management::payrollFor(
        const std::string& month) const {
    std::vector<PayrollRecord> out;
    for (const auto& p : payroll)
        if (p.month == month) out.push_back(p);
    return out;
}

int Management::addSupplier(const std::string& name,
                            const std::string& contact,
                            const std::string& phone,
                            const std::string& email) {
    Supplier s;
    s.id = nextSupplierId++;
    s.name = name;
    s.contactPerson = contact;
    s.phone = phone;
    s.email = email;
    suppliers.push_back(s);
    return s.id;
}

bool Management::linkSupplierToProduct(int supplierId, int productId) {
    for (auto& s : suppliers) {
        if (s.id == supplierId) {
            if (std::find(s.productIds.begin(), s.productIds.end(),
                          productId) == s.productIds.end())
                s.productIds.push_back(productId);
            return true;
        }
    }
    return false;
}

Supplier* Management::findSupplier(int id) {
    for (auto& s : suppliers) if (s.id == id) return &s;
    return nullptr;
}

std::vector<Supplier> Management::allSuppliers() const { return suppliers; }

int Management::createPurchaseOrder(int supplierId,
                                    const std::vector<POItem>& items,
                                    const std::string& date) {
    PurchaseOrder po;
    po.id = nextPOId++;
    po.supplierId = supplierId;
    Supplier* s = findSupplier(supplierId);
    po.supplierName = s ? s->name : "Unknown";
    po.date = date;
    po.status = "pending";
    double total = 0.0;
    for (auto& it : items) {
        POItem copy = it;
        copy.lineTotal = copy.quantity * copy.unitCost;
        total += copy.lineTotal;
        po.items.push_back(copy);
    }
    po.total = total;
    purchaseOrders.push_back(po);
    return po.id;
}

bool Management::markPOReceived(int poId) {
    for (auto& po : purchaseOrders)
        if (po.id == poId) { po.status = "received"; return true; }
    return false;
}

bool Management::cancelPO(int poId) {
    for (auto& po : purchaseOrders)
        if (po.id == poId) { po.status = "cancelled"; return true; }
    return false;
}

std::vector<PurchaseOrder> Management::allPurchaseOrders() const {
    return purchaseOrders;
}

std::vector<PurchaseOrder> Management::pendingPOs() const {
    std::vector<PurchaseOrder> out;
    for (const auto& po : purchaseOrders)
        if (po.status == "pending") out.push_back(po);
    return out;
}

int Management::autoGenerateOrders(const Analytics& analytics,
                                   const Inventory& inventory,
                                   const std::string& date) {
    auto alerts = analytics.reorderAlerts(inventory);
    if (alerts.empty()) return 0;

    std::unordered_map<std::string, std::vector<POItem>> bySupplier;
    for (const auto& a : alerts) {
        Product* p = const_cast<Inventory&>(inventory).getProduct(a.productId);
        if (!p) continue;
        POItem item;
        item.productId = p->id;
        item.productName = p->name;
        item.quantity = a.recommendedOrder;
        item.unitCost = p->price * 0.7;
        item.lineTotal = item.quantity * item.unitCost;
        bySupplier[p->supplier].push_back(item);
    }

    int created = 0;
    for (auto& pair : bySupplier) {
        int sid = -1;
        for (auto& s : suppliers)
            if (s.name == pair.first) { sid = s.id; break; }
        if (sid == -1) sid = addSupplier(pair.first, "Auto", "", "");
        createPurchaseOrder(sid, pair.second, date);
        created++;
    }
    return created;
}

void Management::printEmployeeReport() const {
    std::cout << "\n--- EMPLOYEES ---\n";
    std::cout << std::left << std::setw(5) << "ID"
              << std::setw(20) << "Name"
              << std::setw(15) << "Role"
              << std::setw(12) << "Phone"
              << std::setw(10) << "Salary" << "\n";
    std::cout << std::string(62, '-') << "\n";
    for (const auto& e : employees) {
        if (!e.active) continue;
        std::cout << std::setw(5) << e.id
                  << std::setw(20) << e.name
                  << std::setw(15) << e.role
                  << std::setw(12) << e.phone
                  << std::setw(10) << (int)e.monthlySalary << "\n";
    }
}

void Management::printAttendanceReport(const std::string& month) const {
    std::cout << "\n--- ATTENDANCE (" << month << ") ---\n";
    for (const auto& e : employees) {
        if (!e.active) continue;
        double hrs = totalHours(e.id, month);
        std::cout << "  " << e.name << ": " << std::fixed
                  << std::setprecision(1) << hrs << " hours\n";
    }
}

void Management::printPayrollReport(const std::string& month) const {
    std::cout << "\n--- PAYROLL (" << month << ") ---\n";
    std::cout << std::left << std::setw(20) << "Name"
              << std::setw(12) << "Base"
              << std::setw(12) << "Hours"
              << std::setw(12) << "Deduct"
              << std::setw(12) << "Bonus"
              << std::setw(12) << "Net" << "\n";
    std::cout << std::string(80, '-') << "\n";
    for (const auto& p : payroll) {
        if (p.month != month) continue;
        std::cout << std::setw(20) << p.employeeName
                  << std::setw(12) << (int)p.baseSalary
                  << std::setw(12) << (int)p.hoursWorked
                  << std::setw(12) << (int)p.deductions
                  << std::setw(12) << (int)p.bonus
                  << std::setw(12) << (int)p.netPay << "\n";
    }
}

void Management::printSupplierReport() const {
    std::cout << "\n--- SUPPLIERS ---\n";
    for (const auto& s : suppliers) {
        std::cout << "  [" << s.id << "] " << s.name
                  << "  (" << s.contactPerson << ", " << s.phone << ")\n";
        if (!s.productIds.empty()) {
            std::cout << "      Products: ";
            for (size_t i = 0; i < s.productIds.size(); ++i) {
                if (i) std::cout << ", ";
                std::cout << "ID#" << s.productIds[i];
            }
            std::cout << "\n";
        }
    }
}

void Management::printPOReport() const {
    std::cout << "\n--- PURCHASE ORDERS ---\n";
    for (const auto& po : purchaseOrders) {
        std::cout << "  [PO#" << po.id << "] " << po.supplierName
                  << "  " << po.date << "  [" << po.status << "]"
                  << "  Total: PKR " << (int)po.total << "\n";
        for (const auto& it : po.items)
            std::cout << "      - " << it.productName
                      << " x" << it.quantity
                      << " @ " << (int)it.unitCost
                      << " = " << (int)it.lineTotal << "\n";
    }
}

void Management::saveAll(const std::string& baseDir) const {
    {
        std::vector<std::string> lines;
        for (const auto& e : employees) {
            std::ostringstream oss;
            oss << e.id << "|" << e.name << "|" << e.role << "|"
                << e.phone << "|" << e.monthlySalary << "|"
                << e.joinDate << "|" << (e.active ? 1 : 0);
            lines.push_back(oss.str());
        }
        FileManager::writeLines(baseDir + "/employees.dat", lines);
    }
    {
        std::vector<std::string> lines;
        for (const auto& r : attendance) {
            std::ostringstream oss;
            oss << r.employeeId << "|" << r.date << "|"
                << r.checkIn << "|" << r.checkOut << "|"
                << (r.present ? 1 : 0);
            lines.push_back(oss.str());
        }
        FileManager::writeLines(baseDir + "/attendance.dat", lines);
    }
    {
        std::vector<std::string> lines;
        for (const auto& s : suppliers) {
            std::ostringstream oss;
            oss << s.id << "|" << s.name << "|" << s.contactPerson << "|"
                << s.phone << "|" << s.email << "|";
            for (size_t i = 0; i < s.productIds.size(); ++i) {
                if (i) oss << ",";
                oss << s.productIds[i];
            }
            lines.push_back(oss.str());
        }
        FileManager::writeLines(baseDir + "/suppliers.dat", lines);
    }
    {
        std::vector<std::string> lines;
        for (const auto& po : purchaseOrders) {
            std::ostringstream oss;
            oss << po.id << "|" << po.supplierId << "|" << po.supplierName
                << "|" << po.date << "|" << po.status << "|" << po.total << "|";
            for (size_t i = 0; i < po.items.size(); ++i) {
                if (i) oss << ";";
                oss << po.items[i].productId << ","
                    << po.items[i].productName << ","
                    << po.items[i].quantity << ","
                    << po.items[i].unitCost;
            }
            lines.push_back(oss.str());
        }
        FileManager::writeLines(baseDir + "/purchase_orders.dat", lines);
    }
}

void Management::loadAll(const std::string& baseDir) {
    employees.clear();
    attendance.clear();
    suppliers.clear();
    purchaseOrders.clear();
    nextEmployeeId = nextSupplierId = nextPOId = 1;

    for (const auto& line : FileManager::readLines(baseDir + "/employees.dat")) {
        auto f = FileManager::split(line, '|');
        if (f.size() < 7) continue;
        Employee e;
        e.id = std::stoi(f[0]);
        e.name = f[1];
        e.role = f[2];
        e.phone = f[3];
        e.monthlySalary = std::stod(f[4]);
        e.joinDate = f[5];
        e.active = (f[6] == "1");
        employees.push_back(e);
        if (e.id >= nextEmployeeId) nextEmployeeId = e.id + 1;
    }
    for (const auto& line : FileManager::readLines(baseDir + "/attendance.dat")) {
        auto f = FileManager::split(line, '|');
        if (f.size() < 5) continue;
        AttendanceRecord r;
        r.employeeId = std::stoi(f[0]);
        r.date = f[1];
        r.checkIn = f[2];
        r.checkOut = f[3];
        r.present = (f[4] == "1");
        attendance.push_back(r);
    }
    for (const auto& line : FileManager::readLines(baseDir + "/suppliers.dat")) {
        auto f = FileManager::split(line, '|');
        if (f.size() < 5) continue;
        Supplier s;
        s.id = std::stoi(f[0]);
        s.name = f[1];
        s.contactPerson = f[2];
        s.phone = f[3];
        s.email = f[4];
        if (f.size() > 5 && !f[5].empty())
            for (auto& pid : FileManager::split(f[5], ','))
                s.productIds.push_back(std::stoi(pid));
        suppliers.push_back(s);
        if (s.id >= nextSupplierId) nextSupplierId = s.id + 1;
    }
    for (const auto& line :
         FileManager::readLines(baseDir + "/purchase_orders.dat")) {
        auto f = FileManager::split(line, '|');
        if (f.size() < 7) continue;
        PurchaseOrder po;
        po.id = std::stoi(f[0]);
        po.supplierId = std::stoi(f[1]);
        po.supplierName = f[2];
        po.date = f[3];
        po.status = f[4];
        po.total = std::stod(f[5]);
        if (!f[6].empty()) {
            for (const auto& it : FileManager::split(f[6], ';')) {
                auto parts = FileManager::split(it, ',');
                if (parts.size() < 4) continue;
                POItem item;
                item.productId = std::stoi(parts[0]);
                item.productName = parts[1];
                item.quantity = std::stoi(parts[2]);
                item.unitCost = std::stod(parts[3]);
                item.lineTotal = item.quantity * item.unitCost;
                po.items.push_back(item);
            }
        }
        purchaseOrders.push_back(po);
        if (po.id >= nextPOId) nextPOId = po.id + 1;
    }
}
