#pragma once

#include "CommentsPopup.hpp"

#include <Geode/Geode.hpp>

namespace cw::mod_cmmts {
    namespace ui {
        class StaffBanPopup final : public geode::Popup {
            using Callback = geode::Function<void(asp::Duration)>;

        private:
            CommentUser m_user;
            Callback m_callback = nullptr;

        protected:
            bool init(CommentUser user, Callback&& cb);

        public:
            static StaffBanPopup* create(CommentUser user, Callback&& cb);
        };

        class StaffPopup final : public geode::Popup {
            using Callback = geode::Function<void(CommentAction, Comment const&, std::optional<std::string>, std::optional<asp::Duration>)>;

        private:
            Comment m_comment;
            Callback m_callback = nullptr;

        protected:
            bool init(Comment cmmt, Callback&& cb);

        public:
            static StaffPopup* create(Comment cmmt, Callback&& cb);
        };
    };
};