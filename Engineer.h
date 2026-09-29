// Engineer.h - Section 1: AI Agent Authentication & Access Control
#pragma once
#include <optional>
#include <string>
#include <vector>

enum class Clearance { Low = 0, Medium = 1, High = 2 };

std::string clearanceToString(Clearance c);
Clearance   clearanceFromInt(int value);

// ---------------------------------------------------------------------------
// Engineer: one city engineer account. All data members are private
// (encapsulation); the outside world uses the public interface below.
// ---------------------------------------------------------------------------
class Engineer {
private:
    std::string engineerID;         // e.g. ENG001
    std::string username;
    std::string encryptedPassword;  // never stored in plain text
    Clearance   clearance;

public:
    Engineer();
    Engineer(std::string id, std::string user, std::string encryptedPwd, Clearance level);

    // Convenience factory: encrypts the plain password for you.
    static Engineer createWithPlainPassword(const std::string& id, const std::string& user,
                                            const std::string& plainPassword, Clearance level);

    // Accessors
    const std::string& getID() const { return engineerID; }
    const std::string& getUsername() const { return username; }
    const std::string& getEncryptedPassword() const { return encryptedPassword; }
    Clearance getClearance() const { return clearance; }

    // Behaviour
    bool verifyPassword(const std::string& plain) const;
    void setPassword(const std::string& plain);
    void setClearance(Clearance level) { clearance = level; }
    std::string describe() const;

    // XOR-with-key + hex encoding: a reversible "encryption" suitable for a training
    // simulator. (A real system would use a salted hash such as bcrypt/Argon2.)
    static std::string encrypt(const std::string& plain);

    // Persistence: "ENG001|username|encryptedPwd|clearanceInt"
    std::string toRecord() const;
    static Engineer fromRecord(const std::string& line);   // throws std::invalid_argument
};

// ---------------------------------------------------------------------------
// AuthManager: owns the vector<Engineer> and implements both login strategies.
// ---------------------------------------------------------------------------
class AuthManager {
private:
    std::vector<Engineer> engineers;
    bool sortedByUsername = false;

public:
    bool addEngineer(const Engineer& e);                    // false if username/ID already used
    void setAll(std::vector<Engineer> list);                // used when loading from file
    const std::vector<Engineer>& all() const { return engineers; }
    std::size_t size() const { return engineers.size(); }
    std::string nextEngineerID() const;                     // ENG001, ENG002, ...

    // Linear search with std::find_if  -> O(n), works on unsorted data.
    std::optional<Engineer> loginLinear(const std::string& user, const std::string& pass) const;

    // Binary search with std::lower_bound -> O(log n) per lookup, but needs sorted data.
    // The vector is sorted lazily (O(n log n)) the first time it is needed.
    std::optional<Engineer> loginBinary(const std::string& user, const std::string& pass);

    void sortByUsername();
};
