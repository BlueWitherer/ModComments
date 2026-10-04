#pragma once

#include <util/Comments.hpp>
#include <util/WebRes.hpp>

#include <Geode/Geode.hpp>

namespace cw::mod_cmmts {
    namespace ui {
        class CommentItem final : public cocos2d::CCNode {
        private:
            Comment m_comment;

            geode::Label* m_contentLabel = nullptr;

        protected:
            bool isSelf() const noexcept;

            bool init(Comment cmmt, float width);

        public:
            static CommentItem* create(Comment cmmt, float width);
        };

        class CommentsPopup final : public geode::Popup {
            struct LinkButton final {
                std::string id;
                std::string sprite;
                geode::Button::ButtonCallback callback;
            };

        private:
            std::string m_modID;
            geode::ScrollLayer* m_commentList = nullptr;

            static asp::Instant s_lastComment;

            geode::async::TaskHolder<WebRes> m_commentTask;

            arc::Future<WebRes> getComments();

        protected:
            bool init(std::string modID, bool geodeTheme);

        public:
            static CommentsPopup* create(std::string modID, bool geodeTheme = true);
        };
    };
};