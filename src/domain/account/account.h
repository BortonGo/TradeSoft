#pragma once
#include <QString>
#include <QDateTime>
#include <vector>

enum class AccountType {
    Demo,
    Real
};

inline QString toUiString(AccountType t) {
    switch (t){
    case AccountType::Demo: return "Demo";
    case AccountType::Real: return "Real";
    default: return "Demo";
    }
}

inline AccountType accountTypeFromUiString(const QString& s) {
    if (s.compare("Real", Qt::CaseInsensitive) == 0) {
        return AccountType::Real;
    }
    return AccountType::Demo;
}

struct AccountConfig  final {
    QString id;
    QString name;
    AccountType type;

    double makerFeePct = 0.0;
    double takerFeePct = 0.0;
    int maxLeverage = 1;
};

struct AccountState final {
    QString accountId;

    double balance = 0.0;
    double equity = 0.0;
    double marginUsed = 0.0;

    QDateTime lastUpdate;
};

// simple store
class AccountStore final {
    std::vector<AccountConfig> configs_;
    std::vector<AccountState> states_;

public:
    void add(const AccountConfig& cfg, const AccountState& st) {
        configs_.push_back(cfg);
        states_.push_back(st);
    }

    const AccountConfig* findByName(const QString& name) const {
        for (const auto& c : configs_)
            if (c.name == name)
                return &c;
        return nullptr;
    }

    AccountState* findState(const QString& id) {
        for (auto& s : states_){
            if (s.accountId == id){
                return &s;
            }
        }
        return nullptr;
    }

    const std::vector<AccountConfig>& configs() const {
        return configs_;
    }
};
