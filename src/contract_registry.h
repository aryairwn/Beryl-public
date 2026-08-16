#ifndef BERYL_CONTRACT_REGISTRY_H
#define BERYL_CONTRACT_REGISTRY_H

#include "contract.h"

#include <map>
#include <string>

namespace beryl::contract {

class ContractRegistry final
{
public:
    using RegistryMap =
        std::map<std::string, Contract>;

    bool Add(
        const std::string& address,
        const Contract& contract
    );

    bool Exists(
        const std::string& address
    ) const;

    const Contract* Get(
        const std::string& address
    ) const;

    const RegistryMap& GetAll() const;

    // Deterministic commitment terhadap seluruh
    // contract registry.
    std::string CalculateRoot() const;

private:
    RegistryMap m_contracts;
};

} // namespace beryl::contract

#endif
