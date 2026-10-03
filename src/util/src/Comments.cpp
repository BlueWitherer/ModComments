#include "../Comments.hpp"

#include <Util.h>

#include <Geode/Geode.hpp>

using namespace geode::prelude;
using namespace cw::mod_cmmts;

Result<CommentUser> matjson::Serialize<CommentUser>::fromJson(matjson::Value const& value) {
    CommentUser out;

    GEODE_UNWRAP_INTO(out.id, value["id"].asInt());
    GEODE_UNWRAP_INTO(out.username, value["username"].asString());
    GEODE_UNWRAP_INTO(out.staff, value["staff"].asBool());
    GEODE_UNWRAP_INTO(out.icon, value["icon"].asUInt());
    GEODE_UNWRAP_INTO(auto iconType, value["icon_type"].asInt());
    GEODE_UNWRAP_INTO(out.color1, value["color1"].asUInt());
    GEODE_UNWRAP_INTO(out.color2, value["color2"].asUInt());
    GEODE_UNWRAP_INTO(out.colorGlow, value["color_glow"].asUInt());
    GEODE_UNWRAP_INTO(out.useGlow, value["use_glow"].asBool());

    out.iconType = static_cast<IconType>(iconType);

    return Ok(std::move(out));
};

matjson::Value matjson::Serialize<CommentUser>::toJson(CommentUser const& value) {
    Value out;
    out["id"] = value.id;
    out["username"] = value.username;
    out["staff"] = value.staff;
    out["icon"] = value.icon;
    out["icon_type"] = static_cast<int8_t>(value.iconType);
    out["color1"] = value.color1;
    out["color2"] = value.color2;
    out["color_glow"] = value.colorGlow;
    out["use_glow"] = value.useGlow;

    return out;
};

Result<Comment> matjson::Serialize<Comment>::fromJson(matjson::Value const& value) {
    Comment out;

    GEODE_UNWRAP_INTO(out.id, value["id"].asUInt());
    GEODE_UNWRAP_INTO(out.author, value["author"].as<CommentUser>());
    GEODE_UNWRAP_INTO(out.modID, value["mod"].asString());
    GEODE_UNWRAP_INTO(out.content, value["content"].asString());
    GEODE_UNWRAP_INTO(auto uTime, value["created"].asInt());

    out.created = asp::SystemTime::fromUnix(uTime);

    return Ok(std::move(out));
};

matjson::Value matjson::Serialize<Comment>::toJson(Comment const& value) {
    Value out;
    out["id"] = value.id;
    out["author"] = value.author;
    out["mod"] = value.modID;
    out["content"] = value.content;
    out["created"] = value.created.timeSinceEpoch().seconds();

    return out;
};

Result<CommentReport> matjson::Serialize<CommentReport>::fromJson(matjson::Value const& value) {
    CommentReport out;

    GEODE_UNWRAP_INTO(out.id, value["id"].asUInt());
    GEODE_UNWRAP_INTO(out.author, value["author"].as<CommentUser>());
    GEODE_UNWRAP_INTO(out.comment, value["comment"].as<Comment>());
    GEODE_UNWRAP_INTO(out.reason, value["reason"].asString());
    GEODE_UNWRAP_INTO(auto uTime, value["created"].asInt());

    out.created = asp::SystemTime::fromUnix(uTime);

    return Ok(std::move(out));
};

matjson::Value matjson::Serialize<CommentReport>::toJson(CommentReport const& value) {
    Value out;
    out["id"] = value.id;
    out["author"] = value.author;
    out["comment"] = value.comment;
    out["reason"] = value.reason;
    out["created"] = value.created.timeSinceEpoch().seconds();

    return out;
};