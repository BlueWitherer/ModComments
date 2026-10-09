#pragma once

#include <Geode/Geode.hpp>

namespace cw::mod_cmmts {
    enum class ModLevel : uint8_t {
        None,
        Mod,
        Elder,
        Leaderboard,
    };

    struct UserIcons {
        uint16_t icon = 1;
        IconType iconType = IconType::Cube;
        uint8_t color1 = 1;
        uint8_t color2 = 1;
        uint8_t colorGlow = 1;
        bool useGlow = false;
    };

    struct CommentUser final : UserIcons {
        int id = 0;
        std::string username;
        bool staff;
        ModLevel gdMod;
        asp::SystemTime created;
    };

    struct Comment final {
        uint64_t id = 0;
        CommentUser author;
        std::string modId;
        std::string content;
        asp::SystemTime created;
        uint64_t likes = 0;
        uint64_t dislikes = 0;
        int8_t myVote = 0;
    };

    struct CommentReport final {
        uint64_t id = 0;
        CommentUser author;
        Comment comment;
        std::string reason;
        asp::SystemTime created;
    };
};

template <>
struct matjson::Serialize<cw::mod_cmmts::UserIcons> final {
    static geode::Result<cw::mod_cmmts::UserIcons> fromJson(matjson::Value const& value);
    static matjson::Value toJson(cw::mod_cmmts::UserIcons const& value);
};

template <>
struct matjson::Serialize<cw::mod_cmmts::CommentUser> final {
    static geode::Result<cw::mod_cmmts::CommentUser> fromJson(matjson::Value const& value);
    static matjson::Value toJson(cw::mod_cmmts::CommentUser const& value);
};

template <>
struct matjson::Serialize<cw::mod_cmmts::Comment> final {
    static geode::Result<cw::mod_cmmts::Comment> fromJson(matjson::Value const& value);
    static matjson::Value toJson(cw::mod_cmmts::Comment const& value);
};

template <>
struct matjson::Serialize<cw::mod_cmmts::CommentReport> final {
    static geode::Result<cw::mod_cmmts::CommentReport> fromJson(matjson::Value const& value);
    static matjson::Value toJson(cw::mod_cmmts::CommentReport const& value);
};