#pragma once

#include <util/Comments.hpp>
#include <util/WebRes.hpp>

#include <Geode/Geode.hpp>

namespace cw::mod_cmmts {
    namespace ui {
        class CommentItem final : public cocos2d::CCNode {
        private:
            Comment m_comment;

        protected:
            bool init(Comment cmmt);

        public:
            static CommentItem* create(Comment cmmt);
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

        protected:
            bool init(std::string modID, bool geodeTheme);

        public:
            static CommentsPopup* create(std::string modID, bool geodeTheme = true);
        };
    };
};