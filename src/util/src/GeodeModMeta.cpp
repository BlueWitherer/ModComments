#include "../GeodeMod.hpp"

#include <Util.h>

#include <Geode/Geode.hpp>

using namespace geode::prelude;
using namespace cw::mod_cmmts;

Result<GeodeModDev> matjson::Serialize<GeodeModDev>::fromJson(matjson::Value const& value) {
    GeodeModDev out;

    GEODE_UNWRAP_INTO(out.username, value["username"].asString());
    GEODE_UNWRAP_INTO(out.displayName, value["display_name"].asString());
    GEODE_UNWRAP_INTO(out.owner, value["is_owner"].asBool());

    return Ok(std::move(out));
};

matjson::Value matjson::Serialize<GeodeModDev>::toJson(GeodeModDev const& value) {
    Value out;
    out["username"] = value.username;
    out["display_name"] = value.displayName;
    out["is_owner"] = value.owner;

    return out;
};

Result<GeodeModVersion> matjson::Serialize<GeodeModVersion>::fromJson(matjson::Value const& value) {
    GeodeModVersion out;

    GEODE_UNWRAP_INTO(out.name, value["name"].asString());
    GEODE_UNWRAP_INTO(out.version, value["version"].asString());

    return Ok(std::move(out));
};

matjson::Value matjson::Serialize<GeodeModVersion>::toJson(GeodeModVersion const& value) {
    Value out;
    out["name"] = value.name;
    out["version"] = value.version;

    return out;
};

Result<GeodeMod> matjson::Serialize<GeodeMod>::fromJson(matjson::Value const& value) {
    GeodeMod out;

    GEODE_UNWRAP_INTO(out.id, value["id"].asString());

    GEODE_UNWRAP_INTO(auto const devs, value["developers"].asArray());
    out.developers.reserve(devs.size());

    for (auto const& [k, value] : devs) {
        GEODE_UNWRAP_INTO(auto dev, value.as<GeodeModDev>());
        out.developers.push_back(std::move(dev));
    };

    GEODE_UNWRAP_INTO(auto const vers, value["versions"].asArray());
    if (!vers.empty()) {
        out.versions.reserve(1);

        GEODE_UNWRAP_INTO(auto ver, vers[0].as<GeodeModVersion>());
        out.versions.push_back(std::move(ver));
    };

    return Ok(std::move(out));
};

matjson::Value matjson::Serialize<GeodeMod>::toJson(GeodeMod const& value) {
    Value out;
    out["id"] = value.id;
    out["developers"] = value.developers;
    out["versions"] = value.versions;

    return out;
};