// Engineer.cpp
#include "Engineer.h"
#include "Utils.h"
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <stdexcept>

// ------------------------------ Clearance helpers ---------------------------
std::string clearanceToString(Clearance c) {
    switch (c) {
        case Clearance::Low:    return "Low";
        case Clearance::Medium: return "Medium";
        case Clearance::High:   return "High";
    }
    return "Low";
}

Clearance clearanceFromInt(int value) {
    if (value >= 2) return Clearance::High;
    if (value == 1) return Clearance::Medium;
    return Clearance::Low;
}

// --------------------------------- Engineer ---------------------------------
Engineer::Engineer() : engineerID(""), username(""), encryptedPassword(""), clearance(Clearance::Low) {}

Engineer::Engineer(std::string id, std::string user, std::string encryptedPwd, Clearance level)
    : engineerID(std::move(id)), username(std::move(user)),
      encryptedPassword(std::move(encryptedPwd)), clearance(level) {}

Engineer Engineer::createWithPlainPassword(const std::string& id, const std::string& user,
                                           const std::string& plainPassword, Clearance level) {
    return Engineer(id, user, encrypt(plainPassword), level);
}

std::string Engineer::encrypt(const std::string& plain) {
    static const std::string key = "NeoVerse2035";
    std::ostringstream out;
    for (std::size_t i = 0; i < plain.size(); ++i) {
        unsigned char c = static_cast<unsigned char>(plain[i]) ^
                          static_cast<unsigned char>(key[i % key.size()]);
        out << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(c);
    }
    return out.str();
}

bool Engineer::verifyPassword(const std::string& plain) const {
    return encrypt(plain) == encryptedPassword;
}

void Engineer::setPassword(const std::string& plain) { encryptedPassword = encrypt(plain); }

std::string Engineer::describe() const {
    return engineerID + " | " + username + " | clearance: " + clearanceToString(clearance);
}

std::string Engineer::toRecord() const {
    return engineerID + "|" + username + "|" + encryptedPassword + "|" +
           std::to_string(static_cast<int>(clearance));
}

Engineer Engineer::fromRecord(const std::string& line) {
    std::vector<std::string> p = util::split(line, '|');
    if (p.size() < 4) throw std::invalid_argument("bad engineer record");
    return Engineer(p[0], p[1], p[2], clearanceFromInt(std::stoi(p[3])));
}

// -------------------------------- AuthManager -------------------------------
bool AuthManager::addEngineer(const Engineer& e) {
    // std::any_of scans the vector: O(n)
    bool duplicate = std::any_of(engineers.begin(), engineers.end(), [&](const Engineer& x) {
        return x.getUsername() == e.getUsername() || x.getID() == e.getID();
    });
    if (duplicate) return false;
    engineers.push_back(e);   // amortised O(1)
    sortedByUsername = false;
    return true;
}

void AuthManager::setAll(std::vector<Engineer> list) {
    engineers = std::move(list);
    sortedByUsername = false;
}

std::string AuthManager::nextEngineerID() const {
    for (std::size_t n = engineers.size() + 1;; ++n) {
        std::ostringstream id;
        id << "ENG" << std::setw(3) << std::setfill('0') << n;
        bool used = std::any_of(engineers.begin(), engineers.end(),
                                [&](const Engineer& e) { return e.getID() == id.str(); });
        if (!used) return id.str();
    }
}

std::optional<Engineer> AuthManager::loginLinear(const std::string& user,
                                                 const std::string& pass) const {
    auto it = std::find_if(engineers.begin(), engineers.end(),
                           [&](const Engineer& e) { return e.getUsername() == user; });
    if (it != engineers.end() && it->verifyPassword(pass)) return *it;
    return std::nullopt;
}

void AuthManager::sortByUsername() {
    if (sortedByUsername) return;
    std::sort(engineers.begin(), engineers.end(), [](const Engineer& a, const Engineer& b) {
        return a.getUsername() < b.getUsername();
    });
    sortedByUsername = true;
}

std::optional<Engineer> AuthManager::loginBinary(const std::string& user,
                                                 const std::string& pass) {
    sortByUsername();
    auto it = std::lower_bound(engineers.begin(), engineers.end(), user,
                               [](const Engineer& e, const std::string& u) {
                                   return e.getUsername() < u;
                               });
    if (it != engineers.end() && it->getUsername() == user && it->verifyPassword(pass)) return *it;
    return std::nullopt;
}
