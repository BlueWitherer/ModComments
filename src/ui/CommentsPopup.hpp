#pragma once

#include <util/Comments.hpp>
#include <util/GeodeMod.hpp>
#include <util/WebRes.hpp>

#include <Geode/Geode.hpp>

namespace cw::mod_cmmts {
    struct CommentRequest final {
        std::string modID;
        std::string content;
        UserIcons icons;
    };

    enum class CommentAction : uint8_t {
        Delete,
        Report,
    };

    enum class CommentVote : int8_t {
        Dislike = -1,
        None = 0,
        Like = 1,
    };

    namespace ui {
        class CommentModNode final : public cocos2d::CCNode {
        private:
            std::string m_id;

            GeodeMod m_data;
            bool m_dataOk = false;

            std::string getModName() const;
            std::vector<std::string> getModDevs() const;

        protected:
            bool init(std::string id, std::optional<GeodeMod> mod);

        public:
            static CommentModNode* create(std::string id, std::optional<GeodeMod> mod = std::nullopt);
        };

        class CommentsPopup final : public geode::Popup {
            struct LinkButton final {
                std::string id;
                std::string sprite;
                geode::Button::ButtonCallback callback;
            };

        private:
            std::string m_modID;
            bool m_geodeTheme = false;

            geode::LoadingSpinner* m_geodeLoading = nullptr;

            static geode::utils::StringMap<GeodeMod> s_indexedMods;

            uint16_t m_page = 1;
            uint16_t m_maxPage = m_page;

            geode::Button* m_pageNextBtn = nullptr;
            geode::Button* m_pagePrevBtn = nullptr;

            geode::ScrollLayer* m_commentList = nullptr;
            cocos2d::CCNode* m_commentMenu = nullptr;

            geode::TextInput* m_inputBox = nullptr;

            geode::Label* m_errLabel = nullptr;

            geode::Button* m_refreshBtn = nullptr;

            geode::LoadingSpinner* m_loading = nullptr;

            static asp::Instant s_lastComment;
            static asp::Instant s_lastRefresh;

            geode::async::TaskHolder<WebRes> m_commentTask;
            geode::async::TaskHolder<WebRes> m_commentActionTask;

            arc::Future<WebRes> getComments();
            arc::Future<WebRes> sendComment();

            arc::Future<WebRes> deleteComment(uint64_t id);
            arc::Future<WebRes> reportComment(uint64_t id, std::string reason);

            geode::async::TaskHolder<WebRes> m_geodeTask;

            arc::Future<WebRes> getGeodeData();

            void refreshComments();
            bool showInput() const;

        protected:
            void onDelete(Comment const& cmmt);
            void onReport(Comment const& cmmt);

            void onSend(geode::Button* sender);

            bool init(std::string modID, bool geodeTheme);

        public:
            static CommentsPopup* create(std::string modID, bool geodeTheme = true);
        };
    };
};

template <>
struct matjson::Serialize<cw::mod_cmmts::CommentRequest> final {
    static geode::Result<cw::mod_cmmts::CommentRequest> fromJson(matjson::Value const& value);
    static matjson::Value toJson(cw::mod_cmmts::CommentRequest const& value);
};