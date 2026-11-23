#ifndef ATTENDANCE_H
#define ATTENDANCE_H

#include <string>

struct AttendanceRecord {
    int employeeId;
    std::string date;
    std::string checkIn;
    std::string checkOut;
    bool present;

    AttendanceRecord() : employeeId(0), present(false) {}

    double hoursWorked() const {
        if (!present || checkIn.empty() || checkOut.empty()) return 0.0;
        auto toMin = [](const std::string& t) {
            int h = std::stoi(t.substr(0, 2));
            int m = std::stoi(t.substr(3, 2));
            return h * 60 + m;
        };
        return (toMin(checkOut) - toMin(checkIn)) / 60.0;
    }
};

#endif