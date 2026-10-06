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
            static CommentItem* create(Comment cmmt, float width, bool geodeTheme);

            void setActionCallback(Callback&& cb);

            Comment const& getComment() const noexcept;
        };

        class CommentModNode final : public cocos2d::CCNode {
        private:
            std::string m_id;

            GeodeMod m_data;
            bool m_dataOk = false;

            std::string getModName() const;
            std::string getModVersion() const;
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

            uint16_t m_page = 1;
            uint16_t m_maxPage = m_page;

            geode::ScrollLayer* m_commentList = nullptr;
            cocos2d::CCNode* m_commentMenu = nullptr;

            geode::TextInput* m_inputBox = nullptr;

            geode::Label* m_errLabel = nullptr;

            geode::Button* m_refreshBtn = nullptr;

            geode::LoadingSpinner* m_loading = nullptr;

            static asp::Instant s_lastComment;

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