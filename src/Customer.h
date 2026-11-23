#ifndef CUSTOMER_H
#define CUSTOMER_H

#include <string>
#include <vector>

struct PaymentRecord {
    std::string date;
    double amount;
    bool onTime;
};

struct Customer {
    int id;
    std::string name;
    std::string phone;
    double creditBalance;
    double creditLimit;
    std::vector<PaymentRecord> paymentHistory;

    Customer() : id(0), creditBalance(0.0), creditLimit(5000.0) {}

    double onTimeRatio() const {
        if (paymentHistory.empty()) return 1.0;
        int onTime = 0;
        for (const auto& p : paymentHistory)
            if (p.onTime) onTime++;
        return (double)onTime / paymentHistory.size();
    }

    bool isRisky() const { return onTimeRatio() < 0.6; }
};

#endif
