#pragma once

#include "base/Singleton.hpp"

#include "WebRes.hpp"

#include "Comments.hpp"

namespace cw::mod_cmmts {
    class SelfDirector final : public base::Singleton<SelfDirector>, public UploadPopupDelegate {
    private:
        CommentUser m_user;
        bool m_authorized = false;

        geode::Ref<UploadActionPopup> m_authProgPopup = nullptr;

        geode::async::TaskHolder<WebRes> m_authTask;

        std::unordered_set<uint64_t> m_reportedAds;

    protected:
        void onClosePopup(UploadActionPopup*) override;

    public:
        void startAuth();

        arc::Future<WebRes> authorize();

        void setCurrentUser(CommentUser user);
        void setReport(uint64_t id, bool unset = false);

        geode::Result<const CommentUser> getCurrentUser() const noexcept;
        bool isAuthorized() const noexcept;

        bool isReported(uint64_t id) const noexcept;
    };

    namespace players {
        UserIcons getUserIcons();
        arc::Future<UserIcons> getUserIconsCo();
    };
};