#pragma once

#include <util/Comments.hpp>
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

    namespace ui {
        class CommentItem final : public cocos2d::CCNode {
            using Callback = geode::Function<void(CommentAction, Comment const&)>;

        private:
            Comment m_comment;
            Callback m_callback = nullptr;

            geode::Label* m_contentLabel = nullptr;

        protected:
            bool isSelf() const noexcept;

            bool init(Comment cmmt, float width, bool geodeTheme);

        public:
            static CommentItem* create(Comment cmmt, float width, bool geodeTheme);

            void setActionCallback(Callback&& cb);
        };

        class CommentsPopup final : public geode::Popup {
        private:
            std::string m_modID;
            bool m_geodeTheme = false;

            uint16_t m_page = 1;
            uint16_t m_maxPage = m_page;

            geode::ScrollLayer* m_commentList = nullptr;
            cocos2d::CCNode* m_commentMenu = nullptr;

            geode::TextInput* m_inputBox = nullptr;

            geode::Button* m_refreshBtn = nullptr;

            geode::LoadingSpinner* m_loading = nullptr;

            static asp::Instant s_lastComment;

            geode::async::TaskHolder<WebRes> m_commentTask;

            arc::Future<WebRes> getComments();
            arc::Future<WebRes> sendComment();
            arc::Future<WebRes> deleteComment(uint64_t id);

            void refreshComments();
            bool showInput() const;

        protected:
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