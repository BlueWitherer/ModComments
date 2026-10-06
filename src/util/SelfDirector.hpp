#pragma once

#include "base/Singleton.hpp"

#include "WebRes.hpp"

#include "Comments.hpp"

namespace cw::mod_cmmts {
    class SelfDirector final : public base::Singleton<SelfDirector>, public UploadPopupDelegate {
    private:
        CommentUser m_user;
        bool m_authorized = false;

        std::atomic_bool m_authOngoing{false};
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

        void authProgressing(bool inProgress);

        geode::Result<const CommentUser> getCurrentUser() const noexcept;
        bool isAuthorized() const noexcept;

        bool isReported(uint64_t id) const noexcept;
    };

    namespace user {
        inline auto getUserIcons() {
            auto gm = GameManager::sharedState();

            return UserIcons{
                static_cast<uint16_t>(gm->activeIconForType(gm->m_playerIconType)),
                gm->m_playerIconType,
                static_cast<uint8_t>(gm->getPlayerColor()),
                static_cast<uint8_t>(gm->getPlayerColor2()),
                static_cast<uint8_t>(gm->getPlayerGlowColor()),
                gm->m_playerGlow,
            };
        };
    };
};