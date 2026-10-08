#pragma once

#include <ui/CommentsPopup.hpp>

#include <util/Comments.hpp>
#include <util/GeodeMod.hpp>
#include <util/WebRes.hpp>

#include <Geode/Geode.hpp>

namespace cw::mod_cmmts {
    enum class CommentUserStatus : uint8_t {
        Owner,
        Staff,
    };

    namespace ui {
        class CommentReportPopup final : public geode::Popup {
            using Callback = geode::Function<void(Comment const&, std::string)>;

        private:
            geode::TextInput* m_inputBox = nullptr;

            Callback m_callback = nullptr;

        protected:
            bool init(Comment const& cmmt, Callback&& cb, bool geodeTheme);

        public:
            static CommentReportPopup* create(Comment const& cmmt, Callback&& cb, bool geodeTheme = false);
        };

        class CommentItem final : public cocos2d::CCNode {
            using Callback = geode::Function<void(CommentAction, Comment const&)>;

        private:
            Comment m_comment;
            Callback m_callback = nullptr;

            cocos2d::CCNode* m_userMenu = nullptr;
            geode::Label* m_contentLabel = nullptr;

            geode::Button* m_likeBtn = nullptr;
            geode::Button* m_dislikeBtn = nullptr;

            geode::Ref<geode::Label> m_likeLabel = nullptr;
            geode::Ref<geode::Label> m_dislikeLabel = nullptr;

            geode::async::TaskHolder<WebRes> m_voteTask;

            arc::Future<WebRes> sendVote(CommentVote vote);

            void addBadge(CommentUserStatus type);
            void addVoteNodes(cocos2d::CCNode* to, geode::Button*& btn, geode::Ref<geode::Label>& label, CommentVote type);

        protected:
            void onLike();
            void onDislike();

            void voteCallback(CommentVote t);

            bool isSelf() const noexcept;

            bool init(Comment cmmt, float width, bool buttons, bool geodeTheme);

        public:
            static CommentItem* create(Comment cmmt, float width, bool buttons = true, bool geodeTheme = false);

            void setActionCallback(Callback&& cb);

            Comment const& getComment() const noexcept;
        };
    };
};