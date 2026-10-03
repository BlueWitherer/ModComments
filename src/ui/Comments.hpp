#pragma once

#include <util/WebRes.hpp>

#include <Geode/Geode.hpp>

namespace cw::mod_comments {
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
        std::string modID;
        std::string content;
        asp::SystemTime created;
    };

    struct CommentReport final {
        uint64_t id = 0;
        CommentUser author;
        Comment comment;
        std::string reason;
        asp::SystemTime created;
    };

    namespace ui {
        class CommentCell final : public cocos2d::CCNode {
        };

        class CommentsPopup final : public geode::Popup {
        private:
            std::string m_modID;
            geode::ScrollLayer* m_commentList = nullptr;

            static asp::Instant s_lastComment;

            geode::async::TaskHolder<WebRes> m_commentTask;

        protected:
            bool init(std::string modID, bool geodeTheme);

        public:
            static CommentsPopup* create(std::string modID, bool geodeTheme = true);
        };
    };
};

template <>
struct matjson::Serialize<cw::mod_comments::CommentUser> final {
    static geode::Result<cw::mod_comments::CommentUser> fromJson(matjson::Value const& value);
    static matjson::Value toJson(cw::mod_comments::CommentUser const& value);
};

template <>
struct matjson::Serialize<cw::mod_comments::Comment> final {
    static geode::Result<cw::mod_comments::Comment> fromJson(matjson::Value const& value);
    static matjson::Value toJson(cw::mod_comments::Comment const& value);
};

template <>
struct matjson::Serialize<cw::mod_comments::CommentReport> final {
    static geode::Result<cw::mod_comments::CommentReport> fromJson(matjson::Value const& value);
    static matjson::Value toJson(cw::mod_comments::CommentReport const& value);
};