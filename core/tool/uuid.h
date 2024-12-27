#pragma once

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

namespace uuid = boost::uuids;
namespace boost::uuids {
using UUID = uuid;

inline UUID newUUID()
{
    return random_generator()();
}
}

namespace std {
template <>
struct hash<uuid::UUID> {
    size_t operator()(const uuid::UUID& u) const
    {
        return uuid::hash_value(u);
    }
};
}
