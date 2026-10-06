#pragma once

#include <ui/CommentsPopup.hpp>

#include <util/Comments.hpp>
#include <util/GeodeMod.hpp>
#include <util/WebRes.hpp>

#include <Geode/Geode.hpp>

namespace cw::mod_cmmts {
    namespace ui {
        class CommentReportPopup final : public geode::Popup {
        protected:
            bool init(Comment const& cmmt);

        public:
            static CommentReportPopup* create(Comment const& cmmt);
        };

        class CommentItem final : public cocos2d::CCNode {
            using Callback = geode::Function<void(CommentAction, Comment const&)>;

        private:
            Comment m_comment;
            Callback m_callback = nullptr;

            geode::Label* m_contentLabel = nullptr;

            geode::Button* m_likeBtn = nullptr;
            geode::Button* m_dislikeBtn = nullptr;

            geode::Ref<geode::Label> m_likeLabel = nullptr;
            geode::Ref<geode::Label> m_dislikeLabel = nullptr;

            geode::async::TaskHolder<WebRes> m_voteTask;

            arc::Future<WebRes> sendVote(CommentVote vote);

            void addVoteNodes(cocos2d::CCNode* to, geode::Button*& btn, geode::Ref<geode::Label>& label, CommentVote type);

        protected:
            void onLike();
            void onDislike();

            void voteCallback(CommentVote t);

            bool isSelf() const noexcept;

            bool init(Comment cmmt, float width, bool geodeTheme);

        public:
            static CommentItem* create(Comment cmmt, float width, bool geodeTheme = false);

            void setActionCallback(Callback&& cb);

            Comment const& getComment() const noexcept;
        };
    };
};