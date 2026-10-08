#pragma once

#include <Geode/Geode.hpp>

namespace cw::mod_cmmts {
    struct GeodeModDev final {
        std::string username;
        std::string displayName;
        bool owner = false;
    };

    struct GeodeModVersion final {
        std::string name;
        std::string version;
    };

    struct GeodeMod final {
        std::string id;
        std::vector<GeodeModDev> developers;
        asp::SmallVec<GeodeModVersion, 1> versions;
    };
};

template <>
struct matjson::Serialize<cw::mod_cmmts::GeodeModDev> final {
    static geode::Result<cw::mod_cmmts::GeodeModDev> fromJson(matjson::Value const& value);
    static matjson::Value toJson(cw::mod_cmmts::GeodeModDev const& value);
};

template <>
struct matjson::Serialize<cw::mod_cmmts::GeodeModVersion> final {
    static geode::Result<cw::mod_cmmts::GeodeModVersion> fromJson(matjson::Value const& value);
    static matjson::Value toJson(cw::mod_cmmts::GeodeModVersion const& value);
};

template <>
struct matjson::Serialize<cw::mod_cmmts::GeodeMod> final {
    static geode::Result<cw::mod_cmmts::GeodeMod> fromJson(matjson::Value const& value);
    static matjson::Value toJson(cw::mod_cmmts::GeodeMod const& value);
};