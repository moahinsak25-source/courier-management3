#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <cmath>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <ctime>

using namespace std;

const string CUSTOMER_FILE = "customers.txt";
const string PARCEL_FILE = "parcels.txt";
const string DELIVERY_FILE = "deliveries.txt";
const string PAYMENT_FILE = "payments.txt";
const char DELIM = '|';
const string STAFF_PASSWORD = "admin123";
const double MAX_WEIGHT_KG = 100.0;

namespace Status {
    const string BOOKED = "Booked";
    const string IN_TRANSIT = "In Transit";
    const string OUT_FOR_DELIVERY = "Out for Delivery";
    const string DELIVERED = "Delivered";
    const string CANCELLED = "Cancelled";

    vector<string> all() {
        vector<string> list;
        list.push_back(BOOKED);
        list.push_back(IN_TRANSIT);
        list.push_back(OUT_FOR_DELIVERY);
        list.push_back(DELIVERED);
        list.push_back(CANCELLED);
        return list;
    }

    bool isValid(const string& s) {
        vector<string> list = all();
        return find(list.begin(), list.end(), s) != list.end();
    }

    // Allowed workflow: Booked -> In Transit -> Out for Delivery -> Delivered
    // Cancellation is possible only before the parcel is out for delivery.
    bool canChange(const string& from, const string& to) {
        if (from == BOOKED) return to == IN_TRANSIT || to == CANCELLED;
        if (from == IN_TRANSIT) return to == OUT_FOR_DELIVERY || to == CANCELLED;
        if (from == OUT_FOR_DELIVERY) return to == DELIVERED || to == IN_TRANSIT;
        return false;
    }

    vector<string> nextOptions(const string& from) {
        vector<string> options;
        vector<string> list = all();
        for (size_t i = 0; i < list.size(); i++) {
            if (canChange(from, list[i])) options.push_back(list[i]);
        }
        return options;
    }

    bool isFinal(const string& s) {
        return s == DELIVERED || s == CANCELLED;
    }
}

namespace PaymentStatus {
    const string PENDING = "Pending";
    const string PAID = "Paid";
    const string REFUNDED = "Refunded";
    const string CANCELLED = "Cancelled";

    bool isValid(const string& s) {
        return s == PENDING || s == PAID || s == REFUNDED || s == CANCELLED;
    }
}

namespace InputHelper {

    string trim(const string& s) {
        size_t start = s.find_first_not_of(" \t\r\n");
        if (start == string::npos) return "";
        size_t end = s.find_last_not_of(" \t\r\n");
        return s.substr(start, end - start + 1);
    }

    string toLower(string s) {
        for (size_t i = 0; i < s.size(); i++) s[i] = (char)tolower((unsigned char)s[i]);
        return s;
    }

    string toUpper(string s) {
        for (size_t i = 0; i < s.size(); i++) s[i] = (char)toupper((unsigned char)s[i]);
        return s;
    }

    string titleCase(string s) {
        bool newWord = true;
        for (size_t i = 0; i < s.size(); i++) {
            unsigned char c = (unsigned char)s[i];
            if (isspace(c)) {
                newWord = true;
            } else if (newWord) {
                s[i] = (char)toupper(c);
                newWord = false;
            } else {
                s[i] = (char)tolower(c);
            }
        }
        return s;
    }

    bool containsIgnoreCase(const string& text, const string& part) {
        return toLower(text).find(toLower(part)) != string::npos;
    }

    // Prevents user text from breaking the delimiter-based file format.
    string sanitize(string s) {
        for (size_t i = 0; i < s.size(); i++) {
            if (s[i] == DELIM || s[i] == '\n' || s[i] == '\r') s[i] = '/';
        }
        return s;
    }

    vector<string> split(const string& line, char delim) {
        vector<string> parts;
        string item;
        stringstream ss(line);
        while (getline(ss, item, delim)) parts.push_back(item);
        if (!line.empty() && line[line.size() - 1] == delim) parts.push_back("");
        return parts;
    }

    bool toDouble(const string& text, double& value) {
        stringstream ss(text);
        char extra;
        return (ss >> value) && !(ss >> extra);
    }

    bool toInt(const string& text, int& value) {
        stringstream ss(text);
        char extra;
        return (ss >> value) && !(ss >> extra);
    }

    string fmt(double value, int places = 2) {
        ostringstream os;
        os << fixed << setprecision(places) << value;
        return os.str();
    }

    // Shortens long text so table columns stay aligned.
    string fit(const string& s, size_t width) {
        if (s.size() <= width) return s;
        if (width < 3) return s.substr(0, width);
        return s.substr(0, width - 2) + "..";
    }

    string todayDate() {
        time_t now = time(0);
        struct tm* t = localtime(&now);
        char buffer[16];
        strftime(buffer, sizeof(buffer), "%Y-%m-%d", t);
        return string(buffer);
    }

    // ---------------- validators ----------------
    bool isValidPhone(const string& p) {
        size_t start = (!p.empty() && p[0] == '+') ? 1 : 0;
        size_t digits = p.size() - start;
        if (digits < 7 || digits > 15) return false;
        for (size_t i = start; i < p.size(); i++) {
            if (!isdigit((unsigned char)p[i])) return false;
        }
        return true;
    }

    bool isValidEmail(const string& e) {
        if (e.find(' ') != string::npos) return false;
        size_t at = e.find('@');
        if (at == string::npos || at == 0 || at != e.rfind('@')) return false;
        size_t dot = e.find('.', at);
        return dot != string::npos && dot > at + 1 && dot < e.size() - 1;
    }

    bool isValidDate(const string& d) {
        if (d.size() != 10 || d[4] != '-' || d[7] != '-') return false;
        for (size_t i = 0; i < d.size(); i++) {
            if (i != 4 && i != 7 && !isdigit((unsigned char)d[i])) return false;
        }
        int y = atoi(d.substr(0, 4).c_str());
        int m = atoi(d.substr(5, 2).c_str());
        int day = atoi(d.substr(8, 2).c_str());
        if (y < 2000 || y > 2100 || m < 1 || m > 12 || day < 1) return false;
        int days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
        bool leap = (y % 4 == 0 && y % 100 != 0) || (y % 400 == 0);
        if (leap) days[1] = 29;
        return day <= days[m - 1];
    }

    bool isValidId(const string& id) {
        if (id.size() < 2 || id.size() > 10) return false;
        for (size_t i = 0; i < id.size(); i++) {
            if (!isalnum((unsigned char)id[i])) return false;
        }
        return true;
    }

    // ---------------- console input ----------------
    string readRaw(const string& prompt) {
        string input;
        cout << prompt;
        if (!getline(cin, input)) {
            cout << "\nInput stream closed. Exiting program.\n";
            exit(0);
        }
        return trim(input);
    }

    string readLine(const string& prompt) {
        while (true) {
            string input = readRaw(prompt);
            if (!input.empty()) return sanitize(input);
            cout << "  Error: input cannot be empty.\n";
        }
    }

    string readOptionalText(const string& prompt) {
        string input = readRaw(prompt);
        if (input.empty()) return "-";
        return sanitize(input);
    }

    // Blank input keeps the current value (used by update screens).
    string readOptional(const string& label, const string& current) {
        string input = readRaw("  " + label + " [" + current + "]: ");
        if (input.empty()) return current;
        return sanitize(input);
    }

    string readValidated(const string& prompt, bool (*valid)(const string&), const string& errorMsg) {
        while (true) {
            string input = readRaw(prompt);
            if (valid(input)) return input;
            cout << "  Error: " << errorMsg << "\n";
        }
    }

    string readOptionalValidated(const string& label, const string& current,
                                 bool (*valid)(const string&), const string& errorMsg) {
        while (true) {
            string input = readRaw("  " + label + " [" + current + "]: ");
            if (input.empty()) return current;
            if (valid(input)) return input;
            cout << "  Error: " << errorMsg << "\n";
        }
    }

    string readPhone(const string& prompt) {
        return readValidated(prompt, isValidPhone, "phone must be 7-15 digits (optional leading +).");
    }

    string readEmail(const string& prompt) {
        return readValidated(prompt, isValidEmail, "enter a valid email such as name@example.com.");
    }

    string readDate(const string& prompt) {
        return readValidated(prompt, isValidDate, "enter a real date in YYYY-MM-DD format.");
    }

    string readId(const string& prompt) {
        return toUpper(readValidated(prompt, isValidId, "ID must be 2-10 letters/digits with no spaces."));
    }

    int readInt(const string& prompt, int minV, int maxV) {
        while (true) {
            string line = readRaw(prompt);
            int value;
            if (!toInt(line, value)) {
                cout << "  Error: please enter a whole number.\n";
                continue;
            }
            if (value < minV || value > maxV) {
                cout << "  Error: value must be between " << minV << " and " << maxV << ".\n";
                continue;
            }
            return value;
        }
    }

    double readDouble(const string& prompt, double minV, double maxV) {
        while (true) {
            string line = readRaw(prompt);
            double value;
            if (!toDouble(line, value)) {
                cout << "  Error: please enter a valid number.\n";
                continue;
            }
            if (value < minV || value > maxV) {
                cout << "  Error: value must be between " << fmt(minV, 1) << " and " << fmt(maxV, 1) << ".\n";
                continue;
            }
            return value;
        }
    }

    bool readYesNo(const string& prompt) {
        while (true) {
            string line = toLower(readRaw(prompt));
            if (line == "y" || line == "yes") return true;
            if (line == "n" || line == "no") return false;
            cout << "  Error: please answer y or n.\n";
        }
    }

    // Prints a numbered list and returns the zero-based index chosen.
    int chooseFromList(const string& title, const vector<string>& options) {
        cout << title << "\n";
        for (size_t i = 0; i < options.size(); i++) {
            cout << "  " << (i + 1) << ". " << options[i] << "\n";
        }
        return readInt("Choice: ", 1, (int)options.size()) - 1;
    }

    void waitForEnter() {
        readRaw("\nPress Enter to continue...");
    }

    void printTitle(const string& title) {
        cout << "\n" << string(60, '=') << "\n";
        cout << "  " << title << "\n";
        cout << string(60, '=') << "\n";
    }
}

using namespace InputHelper;

// ---------------------------------------------------------------------
//  BASE CLASS: every stored record can save itself and print itself
// ---------------------------------------------------------------------
class Record {
public:
    virtual ~Record() {}
    virtual string toFileLine() const = 0;
    virtual bool fromFileLine(const string& line) = 0;
    virtual void display() const = 0;
    virtual string getKey() const = 0;
};

// ---------------------------------------------------------------------
//  CUSTOMER
// ---------------------------------------------------------------------
class Customer : public Record {
private:
    string customerId, name, phone, email, address;

public:
    Customer() {}
    Customer(const string& id, const string& n, const string& p, const string& e, const string& a)
        : customerId(id), name(n), phone(p), email(e), address(a) {}

    string getCustomerId() const { return customerId; }
    string getName() const { return name; }
    string getPhone() const { return phone; }
    string getEmail() const { return email; }
    string getAddress() const { return address; }

    void setName(const string& v) { name = v; }
    void setPhone(const string& v) { phone = v; }
    void setEmail(const string& v) { email = v; }
    void setAddress(const string& v) { address = v; }

    string getKey() const override { return customerId; }

    string toFileLine() const override {
        return customerId + DELIM + name + DELIM + phone + DELIM + email + DELIM + address;
    }

    bool fromFileLine(const string& line) override {
        vector<string> f = split(line, DELIM);
        if (f.size() != 5 || f[0].empty() || f[1].empty()) return false;
        customerId = f[0];
        name = f[1];
        phone = f[2];
        email = f[3];
        address = f[4];
        return true;
    }

    void display() const override {
        cout << "  Customer ID : " << customerId << "\n";
        cout << "  Name        : " << name << "\n";
        cout << "  Phone       : " << phone << "\n";
        cout << "  Email       : " << email << "\n";
        cout << "  Address     : " << address << "\n";
    }

    void displayRow() const {
        cout << left << setw(8) << fit(customerId, 7)
             << setw(20) << fit(name, 19)
             << setw(16) << fit(phone, 15)
             << setw(26) << fit(email, 25)
             << fit(address, 30) << "\n";
    }
};

// ---------------------------------------------------------------------
//  PARCEL
// ---------------------------------------------------------------------
class Parcel : public Record {
private:
    string trackingNo, senderId, receiverName, receiverPhone, receiverAddress;
    string origin, destination, zone, parcelType, bookingDate, status;
    double weight, charge;
    bool express, insurance, fragile;

public:
    Parcel() : weight(0.0), charge(0.0), express(false), insurance(false), fragile(false) {
        status = Status::BOOKED;
    }

    string getTrackingNo() const { return trackingNo; }
    string getSenderId() const { return senderId; }
    string getReceiverName() const { return receiverName; }
    string getReceiverPhone() const { return receiverPhone; }
    string getReceiverAddress() const { return receiverAddress; }
    string getOrigin() const { return origin; }
    string getDestination() const { return destination; }
    string getZone() const { return zone; }
    string getParcelType() const { return parcelType; }
    string getBookingDate() const { return bookingDate; }
    string getStatus() const { return status; }
    double getWeight() const { return weight; }
    double getCharge() const { return charge; }
    bool isExpress() const { return express; }
    bool hasInsurance() const { return insurance; }
    bool isFragile() const { return fragile; }

    void setTrackingNo(const string& v) { trackingNo = v; }
    void setSenderId(const string& v) { senderId = v; }
    void setReceiverName(const string& v) { receiverName = v; }
    void setReceiverPhone(const string& v) { receiverPhone = v; }
    void setReceiverAddress(const string& v) { receiverAddress = v; }
    void setOrigin(const string& v) { origin = v; }
    void setDestination(const string& v) { destination = v; }
    void setZone(const string& v) { zone = v; }
    void setParcelType(const string& v) { parcelType = v; }
    void setBookingDate(const string& v) { bookingDate = v; }
    void setStatus(const string& v) { status = v; }
    void setWeight(double v) { weight = v; }
    void setCharge(double v) { charge = v; }
    void setExpress(bool v) { express = v; }
    void setInsurance(bool v) { insurance = v; }
    void setFragile(bool v) { fragile = v; }

    string getKey() const override { return trackingNo; }

    string toFileLine() const override {
        string line = trackingNo;
        line += DELIM; line += senderId;
        line += DELIM; line += receiverName;
        line += DELIM; line += receiverPhone;
        line += DELIM; line += receiverAddress;
        line += DELIM; line += origin;
        line += DELIM; line += destination;
        line += DELIM; line += zone;
        line += DELIM; line += parcelType;
        line += DELIM; line += fmt(weight);
        line += DELIM; line += bookingDate;
        line += DELIM; line += status;
        line += DELIM; line += (express ? "1" : "0");
        line += DELIM; line += (insurance ? "1" : "0");
        line += DELIM; line += (fragile ? "1" : "0");
        line += DELIM; line += fmt(charge);
        return line;
    }

    bool fromFileLine(const string& line) override {
        vector<string> f = split(line, DELIM);
        if (f.size() != 16 || f[0].empty()) return false;
        double w, c;
        if (!toDouble(f[9], w) || !toDouble(f[15], c)) return false;
        if (w <= 0.0 || w > MAX_WEIGHT_KG || c < 0.0) return false;
        if (!Status::isValid(f[11]) || !isValidDate(f[10])) return false;
        trackingNo = f[0];
        senderId = f[1];
        receiverName = f[2];
        receiverPhone = f[3];
        receiverAddress = f[4];
        origin = f[5];
        destination = f[6];
        zone = f[7];
        parcelType = f[8];
        weight = w;
        bookingDate = f[10];
        status = f[11];
        express = (f[12] == "1");
        insurance = (f[13] == "1");
        fragile = (f[14] == "1");
        charge = c;
        return true;
    }

    void display() const override {
        cout << "  Tracking No      : " << trackingNo << "\n";
        cout << "  Sender ID        : " << senderId << "\n";
        cout << "  Receiver         : " << receiverName << " (" << receiverPhone << ")\n";
        cout << "  Receiver Address : " << receiverAddress << "\n";
        cout << "  Route            : " << origin << " -> " << destination << " [" << zone << "]\n";
        cout << "  Parcel Type      : " << parcelType << "\n";
        cout << "  Weight           : " << fmt(weight) << " kg\n";
        cout << "  Extra Services   : " << (express ? "Express " : "") << (insurance ? "Insurance " : "")
             << (fragile ? "Fragile-Handling" : "");
        if (!express && !insurance && !fragile) cout << "None";
        cout << "\n";
        cout << "  Booking Date     : " << bookingDate << "\n";
        cout << "  Charge           : " << fmt(charge) << "\n";
        cout << "  Status           : " << status << "\n";
    }

    static void printHeader() {
        cout << "\n" << left << setw(10) << "Tracking" << setw(8) << "Sender"
             << setw(16) << "Receiver" << setw(14) << "Destination"
             << setw(9) << "Kg" << setw(11) << "Charge" << setw(17) << "Status" << "Date\n";
        cout << string(95, '-') << "\n";
    }

    void displayRow() const {
        cout << left << setw(10) << trackingNo << setw(8) << fit(senderId, 7)
             << setw(16) << fit(receiverName, 15) << setw(14) << fit(destination, 13)
             << setw(9) << fmt(weight) << setw(11) << fmt(charge)
             << setw(17) << status << bookingDate << "\n";
    }
};

// ---------------------------------------------------------------------
//  DELIVERY (one record per status change = full delivery history)
// ---------------------------------------------------------------------
class Delivery : public Record {
private:
    string deliveryId, trackingNo, status, date, location, remarks, receivedBy;

public:
    Delivery() {}
    Delivery(const string& id, const string& tn, const string& st, const string& dt,
             const string& loc, const string& rem, const string& recv)
        : deliveryId(id), trackingNo(tn), status(st), date(dt),
          location(loc), remarks(rem), receivedBy(recv) {}

    string getDeliveryId() const { return deliveryId; }
    string getTrackingNo() const { return trackingNo; }
    string getStatus() const { return status; }
    string getDate() const { return date; }
    string getLocation() const { return location; }
    string getRemarks() const { return remarks; }
    string getReceivedBy() const { return receivedBy; }

    string getKey() const override { return deliveryId; }

    string toFileLine() const override {
        return deliveryId + DELIM + trackingNo + DELIM + status + DELIM + date + DELIM +
               location + DELIM + remarks + DELIM + receivedBy;
    }

    bool fromFileLine(const string& line) override {
        vector<string> f = split(line, DELIM);
        if (f.size() != 7 || f[0].empty() || f[1].empty() || !Status::isValid(f[2])) return false;
        deliveryId = f[0];
        trackingNo = f[1];
        status = f[2];
        date = f[3];
        location = f[4];
        remarks = f[5];
        receivedBy = f[6];
        return true;
    }

    void display() const override {
        cout << "  " << deliveryId << " | " << trackingNo << " | " << status << " | " << date
             << " | " << location << " | " << remarks << " | Received by: " << receivedBy << "\n";
    }

    static void printHeader() {
        cout << "\n" << left << setw(9) << "ID" << setw(10) << "Tracking" << setw(17) << "Status"
             << setw(12) << "Date" << setw(18) << "Location" << setw(16) << "Received By" << "Remarks\n";
        cout << string(95, '-') << "\n";
    }

    void displayRow() const {
        cout << left << setw(9) << deliveryId << setw(10) << trackingNo << setw(17) << status
             << setw(12) << date << setw(18) << fit(location, 17) << setw(16) << fit(receivedBy, 15)
             << fit(remarks, 25) << "\n";
    }
};

// ---------------------------------------------------------------------
//  PAYMENT
// ---------------------------------------------------------------------
class Payment : public Record {
private:
    string paymentId, trackingNo, method, date, status;
    double amount;

public:
    Payment() : amount(0.0) {}
    Payment(const string& id, const string& tn, double amt, const string& m,
            const string& d, const string& st)
        : paymentId(id), trackingNo(tn), method(m), date(d), status(st), amount(amt) {}

    string getPaymentId() const { return paymentId; }
    string getTrackingNo() const { return trackingNo; }
    double getAmount() const { return amount; }
    string getMethod() const { return method; }
    string getDate() const { return date; }
    string getStatus() const { return status; }

    void setAmount(double v) { amount = v; }
    void setStatus(const string& v) { status = v; }

    void markPaid(const string& m, const string& d) {
        method = m;
        date = d;
        status = PaymentStatus::PAID;
    }

    string getKey() const override { return paymentId; }

    string toFileLine() const override {
        return paymentId + DELIM + trackingNo + DELIM + fmt(amount) + DELIM + method + DELIM + date + DELIM + status;
    }

    bool fromFileLine(const string& line) override {
        vector<string> f = split(line, DELIM);
        if (f.size() != 6 || f[0].empty() || f[1].empty()) return false;
        double amt;
        if (!toDouble(f[2], amt) || amt < 0.0 || !PaymentStatus::isValid(f[5])) return false;
        paymentId = f[0];
        trackingNo = f[1];
        amount = amt;
        method = f[3];
        date = f[4];
        status = f[5];
        return true;
    }

    void display() const override {
        cout << "  " << paymentId << " | " << trackingNo << " | " << fmt(amount) << " | "
             << method << " | " << date << " | " << status << "\n";
    }

    static void printHeader() {
        cout << "\n" << left << setw(9) << "ID" << setw(10) << "Tracking" << setw(12) << "Amount"
             << setw(18) << "Method" << setw(12) << "Date" << "Status\n";
        cout << string(70, '-') << "\n";
    }

    void displayRow() const {
        cout << left << setw(9) << paymentId << setw(10) << trackingNo << setw(12) << fmt(amount)
             << setw(18) << method << setw(12) << date << status << "\n";
    }
};

// ---------------------------------------------------------------------
//  CHARGE CALCULATOR
//  charge = (zone base fee + billable kg * zone rate) + optional extras
// ---------------------------------------------------------------------
struct ChargeBreakdown {
    double baseFee;
    double weightFee;
    double expressFee;
    double insuranceFee;
    double fragileFee;
    double billableWeight;
    double total;
};

class ChargeCalculator {
public:
    static vector<string> zoneNames() {
        vector<string> zones;
        zones.push_back("Local");
        zones.push_back("Regional");
        zones.push_back("National");
        zones.push_back("International");
        return zones;
    }

    static double baseFee(const string& zone) {
        if (zone == "Local") return 100.0;
        if (zone == "Regional") return 200.0;
        if (zone == "National") return 350.0;
        if (zone == "International") return 1200.0;
        return 0.0;
    }

    static double ratePerKg(const string& zone) {
        if (zone == "Local") return 20.0;
        if (zone == "Regional") return 40.0;
        if (zone == "National") return 60.0;
        if (zone == "International") return 250.0;
        return 0.0;
    }

    static ChargeBreakdown calculate(const string& zone, double weight,
                                     bool express, bool insurance, bool fragile) {
        ChargeBreakdown b;
        b.baseFee = baseFee(zone);
        b.billableWeight = ceil(weight * 2.0) / 2.0;   // rounded up to nearest 0.5 kg
        b.weightFee = b.billableWeight * ratePerKg(zone);
        double subtotal = b.baseFee + b.weightFee;
        b.expressFee = express ? subtotal * 0.25 : 0.0;
        b.insuranceFee = insurance ? 50.0 : 0.0;
        b.fragileFee = fragile ? 75.0 : 0.0;
        b.total = subtotal + b.expressFee + b.insuranceFee + b.fragileFee;
        return b;
    }

    static void printBreakdown(const ChargeBreakdown& b) {
        cout << "\n  ---------- CHARGE BREAKDOWN ----------\n";
        cout << "  Zone base fee            : " << setw(10) << fmt(b.baseFee) << "\n";
        cout << "  Weight fee (" << fmt(b.billableWeight, 1) << " kg billed) : " << setw(10) << fmt(b.weightFee) << "\n";
        if (b.expressFee > 0) cout << "  Express service (25%)    : " << setw(10) << fmt(b.expressFee) << "\n";
        if (b.insuranceFee > 0) cout << "  Insurance                : " << setw(10) << fmt(b.insuranceFee) << "\n";
        if (b.fragileFee > 0) cout << "  Fragile handling         : " << setw(10) << fmt(b.fragileFee) << "\n";
        cout << "  --------------------------------------\n";
        cout << "  TOTAL CHARGE             : " << setw(10) << fmt(b.total) << "\n";
    }
};

vector<string> parcelTypes() {
    vector<string> types;
    types.push_back("Document");
    types.push_back("Package");
    types.push_back("Electronics");
    types.push_back("Clothing");
    types.push_back("Fragile Item");
    types.push_back("Other");
    return types;
}

vector<string> paymentMethods() {
    vector<string> methods;
    methods.push_back("Cash");
    methods.push_back("Card");
    methods.push_back("Online Transfer");
    return methods;
}

// ---------------------------------------------------------------------
//  GENERIC FILE HELPERS
// ---------------------------------------------------------------------
template <typename T>
bool loadRecords(const string& filename, vector<T>& list) {
    list.clear();
    ifstream in(filename.c_str());
    if (!in) {
        ofstream create(filename.c_str());       // first run: create an empty file
        if (!create) {
            cout << "  Error: cannot open or create " << filename << "\n";
            return false;
        }
        return true;
    }
    string line;
    int lineNo = 0;
    while (getline(in, line)) {
        lineNo++;
        line = trim(line);
        if (line.empty()) continue;
        T item;
        if (item.fromFileLine(line)) {
            list.push_back(item);
        } else {
            cout << "  Warning: " << filename << " line " << lineNo << " is corrupted and was skipped.\n";
        }
    }
    if (in.bad()) {
        cout << "  Error: read failure while reading " << filename << "\n";
        return false;
    }
    return true;
}

template <typename T>
bool saveRecords(const string& filename, const vector<T>& list) {
    string tempName = filename + ".tmp";
    ofstream out(tempName.c_str());
    if (!out) {
        cout << "  Error: cannot open " << tempName << " for writing.\n";
        return false;
    }
    for (size_t i = 0; i < list.size(); i++) out << list[i].toFileLine() << "\n";
    out.close();
    if (out.fail()) {
        cout << "  Error: write failure while saving " << filename << "\n";
        return false;
    }
    remove(filename.c_str());
    if (rename(tempName.c_str(), filename.c_str()) != 0) {
        cout << "  Error: could not replace " << filename << "\n";
        return false;
    }
    return true;
}

bool copyFile(const string& source, const string& target) {
    ifstream in(source.c_str());
    if (!in) return false;
    ofstream out(target.c_str());
    if (!out) return false;
    string line;
    while (getline(in, line)) out << line << "\n";
    return !out.fail();
}

// Builds the next ID such as TRK1005 from the largest number already used.
string nextId(const string& prefix, const vector<string>& ids, int start) {
    int maxNum = start;
    for (size_t i = 0; i < ids.size(); i++) {
        const string& id = ids[i];
        if (id.size() > prefix.size() && id.compare(0, prefix.size(), prefix) == 0) {
            int n;
            if (toInt(id.substr(prefix.size()), n) && n > maxNum) maxNum = n;
        }
    }
    return prefix + to_string(maxNum + 1);
}

// Sorting comparators
bool byTracking(const Parcel& a, const Parcel& b) {
    string x = a.getTrackingNo(), y = b.getTrackingNo();
    if (x.size() != y.size()) return x.size() < y.size();
    return x < y;
}
bool byWeightAsc(const Parcel& a, const Parcel& b) { return a.getWeight() < b.getWeight(); }
bool byWeightDesc(const Parcel& a, const Parcel& b) { return a.getWeight() > b.getWeight(); }
bool byChargeAsc(const Parcel& a, const Parcel& b) { return a.getCharge() < b.getCharge(); }
bool byChargeDesc(const Parcel& a, const Parcel& b) { return a.getCharge() > b.getCharge(); }

// ---------------------------------------------------------------------
//  COURIER SYSTEM (controller: owns all data and all menus)
// ---------------------------------------------------------------------
class CourierSystem {
private:
    vector<Customer> customers;
    vector<Parcel> parcels;
    vector<Delivery> deliveries;
    vector<Payment> payments;
    bool staffAuthorized;

    // lookups and generators
    int findCustomerIndex(const string& id) const;
    int findParcelIndex(const string& trackingNo) const;
    int findPaymentIndex(const string& trackingNo) const;
    string customerName(const string& id) const;
    bool customerHasActiveParcels(const string& id) const;
    string generateTrackingNumber() const;
    string generateDeliveryId() const;
    string generatePaymentId() const;
    bool authorize();
    void recalculateCharge(Parcel& p);

    // persistence
    bool loadAll();
    bool saveAll();
    void backupFiles() const;
    void showFileStatus() const;

    // display helpers
    void printCustomerTable(const vector<Customer>& list) const;
    void printParcelTable(const vector<Parcel>& list) const;
    void showParcelDetails(const Parcel& p) const;
    void showCustomerParcels(const Customer& c) const;

    // customer management
    void customerMenu();
    void addCustomer();
    void displayAllCustomers() const;
    void searchCustomer() const;
    void updateCustomer();
    void deleteCustomer();

    // parcel management
    void parcelMenu();
    void bookParcel();
    void displayAllParcels() const;
    void updateParcel();
    void deleteParcel();
    void chargeEstimate() const;

    // delivery management
    void deliveryMenu();
    void updateParcelStatus();
    void recordPayment();
    void viewParcelHistory() const;
    void viewAllDeliveryHistory() const;
    void viewPendingDeliveries() const;
    void viewPayments() const;

    // search, sort and filter
    void searchMenu();
    void trackParcel() const;
    void searchByCustomerId() const;
    void searchByCustomerName() const;
    void searchByPhone() const;
    void showParcelsByDestination() const;
    void showParcelsByStatus() const;
    void sortParcels() const;

    // reports
    void reportMenu() const;
    void overviewReport() const;
    void statusSummaryReport() const;
    void revenueByDestinationReport() const;
    void customerActivityReport() const;

    // data management
    void dataMenu();

public:
    CourierSystem() : staffAuthorized(false) {}
    void run();
};

// ===================== lookups and generators =====================
int CourierSystem::findCustomerIndex(const string& id) const {
    for (size_t i = 0; i < customers.size(); i++) {
        if (customers[i].getCustomerId() == id) return (int)i;
    }
    return -1;
}

int CourierSystem::findParcelIndex(const string& trackingNo) const {
    for (size_t i = 0; i < parcels.size(); i++) {
        if (parcels[i].getTrackingNo() == trackingNo) return (int)i;
    }
    return -1;
}

int CourierSystem::findPaymentIndex(const string& trackingNo) const {
    for (size_t i = 0; i < payments.size(); i++) {
        if (payments[i].getTrackingNo() == trackingNo) return (int)i;
    }
    return -1;
}

string CourierSystem::customerName(const string& id) const {
    int idx = findCustomerIndex(id);
    return idx == -1 ? "(customer removed)" : customers[idx].getName();
}

bool CourierSystem::customerHasActiveParcels(const string& id) const {
    for (size_t i = 0; i < parcels.size(); i++) {
        if (parcels[i].getSenderId() == id && !Status::isFinal(parcels[i].getStatus())) return true;
    }
    return false;
}

string CourierSystem::generateTrackingNumber() const {
    vector<string> ids;
    for (size_t i = 0; i < parcels.size(); i++) ids.push_back(parcels[i].getTrackingNo());
    return nextId("TRK", ids, 1000);
}

string CourierSystem::generateDeliveryId() const {
    vector<string> ids;
    for (size_t i = 0; i < deliveries.size(); i++) ids.push_back(deliveries[i].getDeliveryId());
    return nextId("DLV", ids, 0);
}

string CourierSystem::generatePaymentId() const {
    vector<string> ids;
    for (size_t i = 0; i < payments.size(); i++) ids.push_back(payments[i].getPaymentId());
    return nextId("PAY", ids, 0);
}

bool CourierSystem::authorize() {
    if (staffAuthorized) return true;
    for (int attempt = 1; attempt <= 3; attempt++) {
        string pw = readRaw("Staff password: ");
        if (pw == STAFF_PASSWORD) {
            staffAuthorized = true;
            cout << "  Access granted.\n";
            return true;
        }
        cout << "  Incorrect password (" << attempt << "/3).\n";
    }
    cout << "  Access denied. Returning to menu.\n";
    return false;
}

void CourierSystem::recalculateCharge(Parcel& p) {
    ChargeBreakdown b = ChargeCalculator::calculate(p.getZone(), p.getWeight(),
                                                    p.isExpress(), p.hasInsurance(), p.isFragile());
    p.setCharge(b.total);
    int payIdx = findPaymentIndex(p.getTrackingNo());
    if (payIdx != -1 && payments[payIdx].getStatus() == PaymentStatus::PENDING) {
        payments[payIdx].setAmount(b.total);
    }
}

// ===================== persistence =====================
bool CourierSystem::loadAll() {
    bool ok = loadRecords(CUSTOMER_FILE, customers);
    ok = loadRecords(PARCEL_FILE, parcels) && ok;
    ok = loadRecords(DELIVERY_FILE, deliveries) && ok;
    ok = loadRecords(PAYMENT_FILE, payments) && ok;
    return ok;
}

bool CourierSystem::saveAll() {
    bool ok = saveRecords(CUSTOMER_FILE, customers);
    ok = saveRecords(PARCEL_FILE, parcels) && ok;
    ok = saveRecords(DELIVERY_FILE, deliveries) && ok;
    ok = saveRecords(PAYMENT_FILE, payments) && ok;
    if (!ok) cout << "  WARNING: some data could not be saved to disk!\n";
    return ok;
}

void CourierSystem::backupFiles() const {
    string files[] = {CUSTOMER_FILE, PARCEL_FILE, DELIVERY_FILE, PAYMENT_FILE};
    for (int i = 0; i < 4; i++) {
        string target = "backup_" + files[i];
        if (copyFile(files[i], target)) cout << "  Backed up " << files[i] << " -> " << target << "\n";
        else cout << "  Error: could not back up " << files[i] << "\n";
    }
}

void CourierSystem::showFileStatus() const {
    string names[] = {CUSTOMER_FILE, PARCEL_FILE, DELIVERY_FILE, PAYMENT_FILE};
    size_t counts[] = {customers.size(), parcels.size(), deliveries.size(), payments.size()};
    cout << "\n" << left << setw(18) << "File" << setw(12) << "Access" << "Records in memory\n";
    cout << string(46, '-') << "\n";
    for (int i = 0; i < 4; i++) {
        ifstream f(names[i].c_str());
        cout << left << setw(18) << names[i] << setw(12) << (f ? "OK" : "MISSING") << counts[i] << "\n";
    }
    int dupCustomers = 0, dupParcels = 0;
    for (size_t i = 0; i < customers.size(); i++)
        for (size_t j = i + 1; j < customers.size(); j++)
            if (customers[i].getCustomerId() == customers[j].getCustomerId()) dupCustomers++;
    for (size_t i = 0; i < parcels.size(); i++)
        for (size_t j = i + 1; j < parcels.size(); j++)
            if (parcels[i].getTrackingNo() == parcels[j].getTrackingNo()) dupParcels++;
    cout << "\nIntegrity check: " << dupCustomers << " duplicate customer ID(s), "
         << dupParcels << " duplicate tracking number(s).\n";
}

// ===================== display helpers =====================
void CourierSystem::printCustomerTable(const vector<Customer>& list) const {
    cout << "\n" << left << setw(8) << "ID" << setw(20) << "Name" << setw(16) << "Phone"
         << setw(26) << "Email" << "Address\n";
    cout << string(95, '-') << "\n";
    for (size_t i = 0; i < list.size(); i++) list[i].displayRow();
    cout << "Total records: " << list.size() << "\n";
}

void CourierSystem::printParcelTable(const vector<Parcel>& list) const {
    Parcel::printHeader();
    for (size_t i = 0; i < list.size(); i++) list[i].displayRow();
    cout << "Total parcels: " << list.size() << "\n";
}

void CourierSystem::showParcelDetails(const Parcel& p) const {
    p.display();
    cout << "  Sender Name      : " << customerName(p.getSenderId()) << "\n";
    int payIdx = findPaymentIndex(p.getTrackingNo());
    if (payIdx != -1) {
        cout << "  Payment          : " << payments[payIdx].getStatus() << " ("
             << fmt(payments[payIdx].getAmount()) << ")\n";
    }
}

void CourierSystem::showCustomerParcels(const Customer& c) const {
    cout << "\n";
    c.display();
    vector<Parcel> sent;
    for (size_t i = 0; i < parcels.size(); i++) {
        if (parcels[i].getSenderId() == c.getCustomerId()) sent.push_back(parcels[i]);
    }
    if (sent.empty()) cout << "  (this customer has no parcels)\n";
    else printParcelTable(sent);
}

// ===================== 1. CUSTOMER MANAGEMENT =====================
void CourierSystem::customerMenu() {
    int choice;
    do {
        printTitle("CUSTOMER MANAGEMENT");
        cout << "  1. Add customer\n  2. Display all customers\n  3. Search customer\n"
             << "  4. Update customer\n  5. Delete customer\n  6. Back to main menu\n";
        choice = readInt("Enter choice: ", 1, 6);
        switch (choice) {
            case 1: addCustomer(); break;
            case 2: displayAllCustomers(); break;
            case 3: searchCustomer(); break;
            case 4: updateCustomer(); break;
            case 5: deleteCustomer(); break;
        }
        if (choice != 6) waitForEnter();
    } while (choice != 6);
}

void CourierSystem::addCustomer() {
    cout << "\n--- Add Customer ---\n";
    string id = readId("Customer ID (e.g. C001): ");
    if (findCustomerIndex(id) != -1) {
        cout << "  Error: customer ID " << id << " already exists.\n";
        return;
    }
    string name = titleCase(readLine("Full name: "));
    string phone = readPhone("Phone: ");
    for (size_t i = 0; i < customers.size(); i++) {
        if (customers[i].getPhone() == phone) {
            cout << "  Warning: this phone number is already registered to " << customers[i].getName() << ".\n";
            break;
        }
    }
    string email = readEmail("Email: ");
    string address = readLine("Address: ");
    customers.push_back(Customer(id, name, phone, email, address));
    if (saveAll()) cout << "  Customer " << id << " added successfully.\n";
}

void CourierSystem::displayAllCustomers() const {
    if (customers.empty()) {
        cout << "\n  No customers registered yet.\n";
        return;
    }
    printCustomerTable(customers);
}

void CourierSystem::searchCustomer() const {
    cout << "\n--- Search Customer ---\n";
    cout << "  1. By customer ID\n  2. By name\n  3. By phone number\n";
    int choice = readInt("Choice: ", 1, 3);
    string key = readLine("Enter search text: ");
    vector<Customer> found;
    for (size_t i = 0; i < customers.size(); i++) {
        const Customer& c = customers[i];
        bool match = false;
        if (choice == 1) match = toUpper(c.getCustomerId()) == toUpper(key);
        else if (choice == 2) match = containsIgnoreCase(c.getName(), key);
        else match = c.getPhone().find(key) != string::npos;
        if (match) found.push_back(c);
    }
    if (found.empty()) {
        cout << "  No matching customer found.\n";
        return;
    }
    printCustomerTable(found);
}

void CourierSystem::updateCustomer() {
    cout << "\n--- Update Customer ---\n";
    string id = toUpper(readLine("Customer ID to update: "));
    int idx = findCustomerIndex(id);
    if (idx == -1) {
        cout << "  Error: customer not found.\n";
        return;
    }
    Customer& c = customers[idx];
    cout << "\nCurrent details:\n";
    c.display();
    cout << "\nPress Enter to keep the current value.\n";
    c.setName(titleCase(readOptional("Name", c.getName())));
    c.setPhone(readOptionalValidated("Phone", c.getPhone(), isValidPhone, "phone must be 7-15 digits."));
    c.setEmail(readOptionalValidated("Email", c.getEmail(), isValidEmail, "invalid email address."));
    c.setAddress(readOptional("Address", c.getAddress()));
    if (saveAll()) cout << "  Customer updated successfully.\n";
}

void CourierSystem::deleteCustomer() {
    cout << "\n--- Delete Customer ---\n";
    string id = toUpper(readLine("Customer ID to delete: "));
    int idx = findCustomerIndex(id);
    if (idx == -1) {
        cout << "  Error: customer not found.\n";
        return;
    }
    if (customerHasActiveParcels(id)) {
        cout << "  Error: this customer still has active parcels. Complete or cancel them first.\n";
        return;
    }
    customers[idx].display();
    if (!readYesNo("Delete this customer permanently? (y/n): ")) {
        cout << "  Deletion cancelled.\n";
        return;
    }
    customers.erase(customers.begin() + idx);
    if (saveAll()) cout << "  Customer deleted.\n";
}

// ===================== 2. PARCEL MANAGEMENT =====================
void CourierSystem::parcelMenu() {
    int choice;
    do {
        printTitle("PARCEL MANAGEMENT");
        cout << "  1. Book new parcel\n  2. Display all parcels\n  3. Track / view parcel details\n"
             << "  4. Update parcel\n  5. Delete parcel\n  6. Charge estimator\n  7. Back to main menu\n";
        choice = readInt("Enter choice: ", 1, 7);
        switch (choice) {
            case 1: bookParcel(); break;
            case 2: displayAllParcels(); break;
            case 3: trackParcel(); break;
            case 4: updateParcel(); break;
            case 5: deleteParcel(); break;
            case 6: chargeEstimate(); break;
        }
        if (choice != 7) waitForEnter();
    } while (choice != 7);
}

void CourierSystem::bookParcel() {
    cout << "\n--- Book New Parcel ---\n";
    if (customers.empty()) {
        cout << "  Error: no customers registered. Add a customer first.\n";
        return;
    }
    string senderId = toUpper(readLine("Sender customer ID: "));
    if (findCustomerIndex(senderId) == -1) {
        cout << "  Error: sender ID not found.\n";
        return;
    }
    cout << "  Sender: " << customerName(senderId) << "\n";

    Parcel p;
    p.setSenderId(senderId);
    p.setReceiverName(titleCase(readLine("Receiver name: ")));
    p.setReceiverPhone(readPhone("Receiver phone: "));
    p.setReceiverAddress(readLine("Receiver address: "));
    p.setOrigin(titleCase(readLine("Origin city: ")));
    p.setDestination(titleCase(readLine("Destination city: ")));

    vector<string> zones = ChargeCalculator::zoneNames();
    p.setZone(zones[chooseFromList("Delivery zone:", zones)]);
    vector<string> types = parcelTypes();
    p.setParcelType(types[chooseFromList("Parcel type:", types)]);
    p.setWeight(readDouble("Weight in kg (0.1 - 100): ", 0.1, MAX_WEIGHT_KG));
    p.setExpress(readYesNo("Express delivery (+25%)? (y/n): "));
    p.setInsurance(readYesNo("Insurance (+50)? (y/n): "));
    p.setFragile(readYesNo("Fragile handling (+75)? (y/n): "));
    if (readYesNo("Use today's date as booking date? (y/n): ")) p.setBookingDate(todayDate());
    else p.setBookingDate(readDate("Booking date (YYYY-MM-DD): "));

    ChargeBreakdown b = ChargeCalculator::calculate(p.getZone(), p.getWeight(),
                                                    p.isExpress(), p.hasInsurance(), p.isFragile());
    if (b.total <= 0.0) {
        cout << "  Error: an invalid charge was calculated. Booking aborted.\n";
        return;
    }
    p.setCharge(b.total);
    ChargeCalculator::printBreakdown(b);
    if (!readYesNo("Confirm booking? (y/n): ")) {
        cout << "  Booking cancelled.\n";
        return;
    }

    p.setTrackingNo(generateTrackingNumber());
    p.setStatus(Status::BOOKED);
    parcels.push_back(p);
    deliveries.push_back(Delivery(generateDeliveryId(), p.getTrackingNo(), Status::BOOKED,
                                  p.getBookingDate(), p.getOrigin(), "Parcel booked", "-"));
    payments.push_back(Payment(generatePaymentId(), p.getTrackingNo(), p.getCharge(),
                               "-", "-", PaymentStatus::PENDING));

    if (readYesNo("Record payment now? (y/n): ")) {
        vector<string> methods = paymentMethods();
        string method = methods[chooseFromList("Payment method:", methods)];
        payments.back().markPaid(method, todayDate());
    }
    if (saveAll()) {
        cout << "\n  Parcel booked successfully!\n";
        cout << "  >>> TRACKING NUMBER: " << p.getTrackingNo() << " <<<\n";
    }
}

void CourierSystem::displayAllParcels() const {
    if (parcels.empty()) {
        cout << "\n  No parcels booked yet.\n";
        return;
    }
    printParcelTable(parcels);
}

void CourierSystem::updateParcel() {
    cout << "\n--- Update Parcel ---\n";
    string tn = toUpper(readLine("Tracking number: "));
    int idx = findParcelIndex(tn);
    if (idx == -1) {
        cout << "  Error: parcel not found.\n";
        return;
    }
    Parcel& p = parcels[idx];
    if (p.getStatus() != Status::BOOKED) {
        cout << "  Error: only parcels with status 'Booked' can be edited (current: " << p.getStatus() << ").\n";
        return;
    }
    int payIdx = findPaymentIndex(tn);
    bool chargeLocked = (payIdx != -1 && payments[payIdx].getStatus() == PaymentStatus::PAID);

    cout << "\nCurrent details:\n";
    showParcelDetails(p);
    cout << "\nPress Enter to keep the current value.\n";
    p.setReceiverName(titleCase(readOptional("Receiver name", p.getReceiverName())));
    p.setReceiverPhone(readOptionalValidated("Receiver phone", p.getReceiverPhone(), isValidPhone, "phone must be 7-15 digits."));
    p.setReceiverAddress(readOptional("Receiver address", p.getReceiverAddress()));

    if (chargeLocked) {
        cout << "  Note: payment already received, so destination, zone, type, weight and extras are locked.\n";
    } else {
        p.setDestination(titleCase(readOptional("Destination city", p.getDestination())));
        if (readYesNo("Change delivery zone (now " + p.getZone() + ")? (y/n): ")) {
            vector<string> zones = ChargeCalculator::zoneNames();
            p.setZone(zones[chooseFromList("Delivery zone:", zones)]);
        }
        if (readYesNo("Change parcel type (now " + p.getParcelType() + ")? (y/n): ")) {
            vector<string> types = parcelTypes();
            p.setParcelType(types[chooseFromList("Parcel type:", types)]);
        }
        while (true) {
            string w = readRaw("  Weight kg [" + fmt(p.getWeight()) + "]: ");
            if (w.empty()) break;
            double value;
            if (toDouble(w, value) && value >= 0.1 && value <= MAX_WEIGHT_KG) {
                p.setWeight(value);
                break;
            }
            cout << "  Error: weight must be a number between 0.1 and 100.\n";
        }
        if (readYesNo("Express delivery? (currently " + string(p.isExpress() ? "yes" : "no") + ") (y/n): "))
            p.setExpress(true);
        else
            p.setExpress(false);
        p.setInsurance(readYesNo("Insurance? (y/n): "));
        p.setFragile(readYesNo("Fragile handling? (y/n): "));
        recalculateCharge(p);
        cout << "  New charge: " << fmt(p.getCharge()) << "\n";
    }
    if (saveAll()) cout << "  Parcel updated successfully.\n";
}

void CourierSystem::deleteParcel() {
    cout << "\n--- Delete Parcel ---\n";
    string tn = toUpper(readLine("Tracking number: "));
    int idx = findParcelIndex(tn);
    if (idx == -1) {
        cout << "  Error: parcel not found.\n";
        return;
    }
    const string status = parcels[idx].getStatus();
    int payIdx = findPaymentIndex(tn);
    if (status == Status::DELIVERED || status == Status::IN_TRANSIT || status == Status::OUT_FOR_DELIVERY) {
        cout << "  Error: parcels that are in transit or delivered are kept for records and cannot be deleted.\n";
        return;
    }
    if (status == Status::BOOKED && payIdx != -1 && payments[payIdx].getStatus() == PaymentStatus::PAID) {
        cout << "  Error: this parcel is already paid. Cancel it first so the payment is refunded.\n";
        return;
    }
    parcels[idx].display();
    if (!readYesNo("Delete this parcel and its delivery/payment records? (y/n): ")) {
        cout << "  Deletion cancelled.\n";
        return;
    }
    parcels.erase(parcels.begin() + idx);
    for (int i = (int)deliveries.size() - 1; i >= 0; i--) {
        if (deliveries[i].getTrackingNo() == tn) deliveries.erase(deliveries.begin() + i);
    }
    for (int i = (int)payments.size() - 1; i >= 0; i--) {
        if (payments[i].getTrackingNo() == tn) payments.erase(payments.begin() + i);
    }
    if (saveAll()) cout << "  Parcel " << tn << " deleted.\n";
}

void CourierSystem::chargeEstimate() const {
    cout << "\n--- Charge Estimator (no booking is created) ---\n";
    vector<string> zones = ChargeCalculator::zoneNames();
    string zone = zones[chooseFromList("Delivery zone:", zones)];
    double weight = readDouble("Weight in kg (0.1 - 100): ", 0.1, MAX_WEIGHT_KG);
    bool express = readYesNo("Express delivery? (y/n): ");
    bool insurance = readYesNo("Insurance? (y/n): ");
    bool fragile = readYesNo("Fragile handling? (y/n): ");
    ChargeCalculator::printBreakdown(ChargeCalculator::calculate(zone, weight, express, insurance, fragile));
}

// ===================== 3. DELIVERY MANAGEMENT =====================
void CourierSystem::deliveryMenu() {
    int choice;
    do {
        printTitle("DELIVERY MANAGEMENT");
        cout << "  1. Update parcel status (staff only)\n  2. Record payment (staff only)\n"
             << "  3. View history of one parcel\n  4. View all delivery history\n"
             << "  5. View pending deliveries\n  6. View payments\n  7. Back to main menu\n";
        choice = readInt("Enter choice: ", 1, 7);
        switch (choice) {
            case 1: updateParcelStatus(); break;
            case 2: recordPayment(); break;
            case 3: viewParcelHistory(); break;
            case 4: viewAllDeliveryHistory(); break;
            case 5: viewPendingDeliveries(); break;
            case 6: viewPayments(); break;
        }
        if (choice != 7) waitForEnter();
    } while (choice != 7);
}

void CourierSystem::updateParcelStatus() {
    cout << "\n--- Update Parcel Status ---\n";
    if (!authorize()) return;
    string tn = toUpper(readLine("Tracking number: "));
    int idx = findParcelIndex(tn);
    if (idx == -1) {
        cout << "  Error: parcel not found.\n";
        return;
    }
    Parcel& p = parcels[idx];
    cout << "  Current status: " << p.getStatus() << "\n";
    vector<string> options = Status::nextOptions(p.getStatus());
    if (options.empty()) {
        cout << "  Error: a " << p.getStatus() << " parcel cannot change status any more.\n";
        return;
    }
    string newStatus = options[chooseFromList("Allowed next statuses:", options)];
    string location = titleCase(readLine("Current location / hub: "));
    string remarks = readOptionalText("Remarks (optional): ");
    string receivedBy = "-";
    if (newStatus == Status::DELIVERED) receivedBy = readLine("Received by (name): ");
    if (!readYesNo("Change status to '" + newStatus + "'? (y/n): ")) {
        cout << "  Status change cancelled.\n";
        return;
    }

    int payIdx = findPaymentIndex(tn);
    if (newStatus == Status::DELIVERED && payIdx != -1 &&
        payments[payIdx].getStatus() == PaymentStatus::PENDING) {
        if (readYesNo("Payment is pending. Collect cash on delivery now? (y/n): ")) {
            payments[payIdx].markPaid("Cash", todayDate());
            cout << "  Payment of " << fmt(payments[payIdx].getAmount()) << " recorded.\n";
        }
    }
    if (newStatus == Status::CANCELLED && payIdx != -1) {
        if (payments[payIdx].getStatus() == PaymentStatus::PAID) {
            payments[payIdx].setStatus(PaymentStatus::REFUNDED);
            cout << "  Payment marked as refunded.\n";
        } else {
            payments[payIdx].setStatus(PaymentStatus::CANCELLED);
        }
    }
    p.setStatus(newStatus);
    deliveries.push_back(Delivery(generateDeliveryId(), tn, newStatus, todayDate(),
                                  location, remarks, receivedBy));
    if (saveAll()) cout << "  Status updated to '" << newStatus << "'.\n";
}

void CourierSystem::recordPayment() {
    cout << "\n--- Record Payment ---\n";
    if (!authorize()) return;
    string tn = toUpper(readLine("Tracking number: "));
    int idx = findParcelIndex(tn);
    int payIdx = findPaymentIndex(tn);
    if (idx == -1 || payIdx == -1) {
        cout << "  Error: parcel or payment record not found.\n";
        return;
    }
    if (parcels[idx].getStatus() == Status::CANCELLED) {
        cout << "  Error: this parcel is cancelled; no payment can be taken.\n";
        return;
    }
    if (payments[payIdx].getStatus() == PaymentStatus::PAID) {
        cout << "  This parcel has already been paid.\n";
        return;
    }
    cout << "  Amount due: " << fmt(payments[payIdx].getAmount()) << "\n";
    vector<string> methods = paymentMethods();
    string method = methods[chooseFromList("Payment method:", methods)];
    payments[payIdx].markPaid(method, todayDate());
    if (saveAll()) cout << "  Payment recorded successfully.\n";
}

void CourierSystem::viewParcelHistory() const {
    string tn = toUpper(readLine("Tracking number: "));
    if (findParcelIndex(tn) == -1) {
        cout << "  Error: parcel not found.\n";
        return;
    }
    bool any = false;
    for (size_t i = 0; i < deliveries.size(); i++) {
        if (deliveries[i].getTrackingNo() != tn) continue;
        if (!any) Delivery::printHeader();
        deliveries[i].displayRow();
        any = true;
    }
    if (!any) cout << "  No delivery history recorded.\n";
}

void CourierSystem::viewAllDeliveryHistory() const {
    if (deliveries.empty()) {
        cout << "\n  No delivery records yet.\n";
        return;
    }
    Delivery::printHeader();
    for (size_t i = 0; i < deliveries.size(); i++) deliveries[i].displayRow();
    cout << "Total delivery events: " << deliveries.size() << "\n";
}

void CourierSystem::viewPendingDeliveries() const {
    vector<Parcel> pending;
    for (size_t i = 0; i < parcels.size(); i++) {
        if (!Status::isFinal(parcels[i].getStatus())) pending.push_back(parcels[i]);
    }
    if (pending.empty()) {
        cout << "\n  No pending deliveries.\n";
        return;
    }
    printParcelTable(pending);
}

void CourierSystem::viewPayments() const {
    if (payments.empty()) {
        cout << "\n  No payment records yet.\n";
        return;
    }
    Payment::printHeader();
    for (size_t i = 0; i < payments.size(); i++) payments[i].displayRow();
}

// ===================== 4. SEARCH, TRACKING, SORT, FILTER =====================
void CourierSystem::searchMenu() {
    int choice;
    do {
        printTitle("SEARCH & TRACKING");
        cout << "  1. Track parcel by tracking number\n  2. Search by customer ID\n"
             << "  3. Search by customer name\n  4. Search by phone number\n"
             << "  5. Search by destination\n  6. Search / filter by status\n"
             << "  7. Sort parcels\n  8. Back to main menu\n";
        choice = readInt("Enter choice: ", 1, 8);
        switch (choice) {
            case 1: trackParcel(); break;
            case 2: searchByCustomerId(); break;
            case 3: searchByCustomerName(); break;
            case 4: searchByPhone(); break;
            case 5: showParcelsByDestination(); break;
            case 6: showParcelsByStatus(); break;
            case 7: sortParcels(); break;
        }
        if (choice != 8) waitForEnter();
    } while (choice != 8);
}

void CourierSystem::trackParcel() const {
    string tn = toUpper(readLine("Enter tracking number: "));
    int idx = findParcelIndex(tn);
    if (idx == -1) {
        cout << "  No parcel found with tracking number " << tn << ".\n";
        return;
    }
    cout << "\n";
    showParcelDetails(parcels[idx]);
    cout << "\n  Delivery history:";
    bool any = false;
    for (size_t i = 0; i < deliveries.size(); i++) {
        if (deliveries[i].getTrackingNo() != tn) continue;
        if (!any) Delivery::printHeader();
        deliveries[i].displayRow();
        any = true;
    }
    if (!any) cout << " none recorded.\n";
}

void CourierSystem::searchByCustomerId() const {
    string id = toUpper(readLine("Customer ID: "));
    int idx = findCustomerIndex(id);
    if (idx == -1) {
        cout << "  No customer found with ID " << id << ".\n";
        return;
    }
    showCustomerParcels(customers[idx]);
}

void CourierSystem::searchByCustomerName() const {
    string key = readLine("Customer name (or part of it): ");
    bool any = false;
    for (size_t i = 0; i < customers.size(); i++) {
        if (containsIgnoreCase(customers[i].getName(), key)) {
            showCustomerParcels(customers[i]);
            any = true;
        }
    }
    if (!any) cout << "  No customer name matches '" << key << "'.\n";
}

void CourierSystem::searchByPhone() const {
    string key = readLine("Phone number (or part of it): ");
    vector<Customer> foundCustomers;
    vector<Parcel> foundParcels;
    for (size_t i = 0; i < customers.size(); i++) {
        if (customers[i].getPhone().find(key) != string::npos) foundCustomers.push_back(customers[i]);
    }
    for (size_t i = 0; i < parcels.size(); i++) {
        if (parcels[i].getReceiverPhone().find(key) != string::npos) foundParcels.push_back(parcels[i]);
    }
    if (foundCustomers.empty() && foundParcels.empty()) {
        cout << "  No customer or receiver has that phone number.\n";
        return;
    }
    if (!foundCustomers.empty()) {
        cout << "\n  Matching customers:";
        printCustomerTable(foundCustomers);
    }
    if (!foundParcels.empty()) {
        cout << "\n  Parcels with matching receiver phone:";
        printParcelTable(foundParcels);
    }
}

void CourierSystem::showParcelsByDestination() const {
    string key = readLine("Destination city (or part of it): ");
    vector<Parcel> found;
    for (size_t i = 0; i < parcels.size(); i++) {
        if (containsIgnoreCase(parcels[i].getDestination(), key)) found.push_back(parcels[i]);
    }
    if (found.empty()) {
        cout << "  No parcels found for destination '" << key << "'.\n";
        return;
    }
    printParcelTable(found);
}

void CourierSystem::showParcelsByStatus() const {
    vector<string> statuses = Status::all();
    string chosen = statuses[chooseFromList("Select status:", statuses)];
    vector<Parcel> found;
    for (size_t i = 0; i < parcels.size(); i++) {
        if (parcels[i].getStatus() == chosen) found.push_back(parcels[i]);
    }
    if (found.empty()) {
        cout << "  No parcels with status '" << chosen << "'.\n";
        return;
    }
    printParcelTable(found);
}

void CourierSystem::sortParcels() const {
    if (parcels.empty()) {
        cout << "  No parcels to sort.\n";
        return;
    }
    cout << "\n  1. Tracking number\n  2. Weight (light to heavy)\n  3. Weight (heavy to light)\n"
         << "  4. Charge (low to high)\n  5. Charge (high to low)\n";
    int choice = readInt("Sort by: ", 1, 5);
    vector<Parcel> sorted = parcels;
    switch (choice) {
        case 1: sort(sorted.begin(), sorted.end(), byTracking); break;
        case 2: sort(sorted.begin(), sorted.end(), byWeightAsc); break;
        case 3: sort(sorted.begin(), sorted.end(), byWeightDesc); break;
        case 4: sort(sorted.begin(), sorted.end(), byChargeAsc); break;
        case 5: sort(sorted.begin(), sorted.end(), byChargeDesc); break;
    }
    printParcelTable(sorted);
}

// ===================== 5. REPORTS =====================
void CourierSystem::reportMenu() const {
    int choice;
    do {
        printTitle("REPORTS");
        cout << "  1. Business overview\n  2. Parcel status summary\n  3. Revenue by destination\n"
             << "  4. Delivery history\n  5. Customer activity\n  6. Back to main menu\n";
        choice = readInt("Enter choice: ", 1, 6);
        switch (choice) {
            case 1: overviewReport(); break;
            case 2: statusSummaryReport(); break;
            case 3: revenueByDestinationReport(); break;
            case 4: viewAllDeliveryHistory(); break;
            case 5: customerActivityReport(); break;
        }
        if (choice != 6) waitForEnter();
    } while (choice != 6);
}

void CourierSystem::overviewReport() const {
    int delivered = 0, cancelled = 0, pending = 0;
    for (size_t i = 0; i < parcels.size(); i++) {
        const string& s = parcels[i].getStatus();
        if (s == Status::DELIVERED) delivered++;
        else if (s == Status::CANCELLED) cancelled++;
        else pending++;
    }
    double revenue = 0.0, outstanding = 0.0, refunded = 0.0;
    for (size_t i = 0; i < payments.size(); i++) {
        if (payments[i].getStatus() == PaymentStatus::PAID) revenue += payments[i].getAmount();
        else if (payments[i].getStatus() == PaymentStatus::PENDING) outstanding += payments[i].getAmount();
        else if (payments[i].getStatus() == PaymentStatus::REFUNDED) refunded += payments[i].getAmount();
    }
    cout << "\n========== BUSINESS OVERVIEW ==========\n";
    cout << left << setw(32) << "Total customers" << customers.size() << "\n";
    cout << setw(32) << "Total parcels" << parcels.size() << "\n";
    cout << setw(32) << "Delivered parcels" << delivered << "\n";
    cout << setw(32) << "Pending / in-transit parcels" << pending << "\n";
    cout << setw(32) << "Cancelled parcels" << cancelled << "\n";
    cout << setw(32) << "Total revenue (paid)" << fmt(revenue) << "\n";
    cout << setw(32) << "Outstanding payments" << fmt(outstanding) << "\n";
    cout << setw(32) << "Refunded amount" << fmt(refunded) << "\n";
}

void CourierSystem::statusSummaryReport() const {
    vector<string> statuses = Status::all();
    cout << "\n========== PARCEL STATUS SUMMARY ==========\n";
    if (parcels.empty()) {
        cout << "  No parcels recorded.\n";
        return;
    }
    for (size_t i = 0; i < statuses.size(); i++) {
        int count = 0;
        for (size_t j = 0; j < parcels.size(); j++) {
            if (parcels[j].getStatus() == statuses[i]) count++;
        }
        double pct = count * 100.0 / parcels.size();
        cout << left << setw(20) << statuses[i] << setw(6) << count << setw(8) << (fmt(pct, 1) + "%")
             << string((int)(pct / 5.0), '*') << "\n";
    }
}

void CourierSystem::revenueByDestinationReport() const {
    map<string, double> revenue;
    map<string, int> parcelCount;
    for (size_t i = 0; i < parcels.size(); i++) {
        if (parcels[i].getStatus() != Status::CANCELLED) parcelCount[parcels[i].getDestination()]++;
    }
    for (size_t i = 0; i < payments.size(); i++) {
        if (payments[i].getStatus() != PaymentStatus::PAID) continue;
        int idx = findParcelIndex(payments[i].getTrackingNo());
        if (idx != -1) revenue[parcels[idx].getDestination()] += payments[i].getAmount();
    }
    cout << "\n========== REVENUE BY DESTINATION ==========\n";
    if (parcelCount.empty()) {
        cout << "  No data available.\n";
        return;
    }
    cout << left << setw(20) << "Destination" << setw(10) << "Parcels" << "Paid revenue\n";
    cout << string(45, '-') << "\n";
    double total = 0.0;
    for (map<string, int>::const_iterator it = parcelCount.begin(); it != parcelCount.end(); ++it) {
        double amount = revenue.count(it->first) ? revenue[it->first] : 0.0;
        total += amount;
        cout << left << setw(20) << fit(it->first, 19) << setw(10) << it->second << fmt(amount) << "\n";
    }
    cout << string(45, '-') << "\n" << left << setw(30) << "TOTAL" << fmt(total) << "\n";
}

void CourierSystem::customerActivityReport() const {
    cout << "\n========== CUSTOMER ACTIVITY ==========\n";
    if (customers.empty()) {
        cout << "  No customers registered.\n";
        return;
    }
    cout << left << setw(8) << "ID" << setw(22) << "Name" << setw(10) << "Parcels" << "Total charges (excl. cancelled)\n";
    cout << string(60, '-') << "\n";
    for (size_t i = 0; i < customers.size(); i++) {
        int count = 0;
        double spent = 0.0;
        for (size_t j = 0; j < parcels.size(); j++) {
            if (parcels[j].getSenderId() != customers[i].getCustomerId()) continue;
            count++;
            if (parcels[j].getStatus() != Status::CANCELLED) spent += parcels[j].getCharge();
        }
        cout << left << setw(8) << customers[i].getCustomerId() << setw(22) << fit(customers[i].getName(), 21)
             << setw(10) << count << fmt(spent) << "\n";
    }
}

// ===================== 6. DATA MANAGEMENT =====================
void CourierSystem::dataMenu() {
    int choice;
    do {
        printTitle("FILE / DATA MANAGEMENT");
        cout << "  1. Save all data now\n  2. Reload data from files\n  3. Create backup copies\n"
             << "  4. Show file status and integrity check\n  5. Back to main menu\n";
        choice = readInt("Enter choice: ", 1, 5);
        switch (choice) {
            case 1:
                if (saveAll()) cout << "  All data saved.\n";
                break;
            case 2:
                if (readYesNo("Unsaved changes in memory will be replaced. Continue? (y/n): ")) {
                    if (loadAll()) cout << "  Data reloaded from files.\n";
                }
                break;
            case 3: backupFiles(); break;
            case 4: showFileStatus(); break;
        }
        if (choice != 5) waitForEnter();
    } while (choice != 5);
}

void CourierSystem::run() {
    cout << "\n*** COURIER AND PARCEL MANAGEMENT SYSTEM ***\n";
    if (!loadAll()) cout << "  Some files could not be loaded. Check file permissions.\n";
    cout << "  Loaded " << customers.size() << " customer(s) and " << parcels.size() << " parcel(s).\n";
    int choice;
    do {
        printTitle("MAIN MENU");
        cout << "  1. Customer Management\n  2. Parcel Management\n  3. Delivery Management\n"
             << "  4. Search & Tracking\n  5. Reports\n  6. File / Data Management\n  7. Exit\n";
        choice = readInt("Enter choice: ", 1, 7);
        switch (choice) {
            case 1: customerMenu(); break;
            case 2: parcelMenu(); break;
            case 3: deliveryMenu(); break;
            case 4: searchMenu(); break;
            case 5: reportMenu(); break;
            case 6: dataMenu(); break;
        }
    } while (choice != 7);
    saveAll();
    cout << "\nAll data saved. Goodbye!\n";
}

int main() {
    CourierSystem app;
    app.run();
    return 0;
}
