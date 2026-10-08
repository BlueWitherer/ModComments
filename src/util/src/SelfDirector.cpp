#include "../SelfDirector.hpp"

#include <Util.h>

#include <Geode/Geode.hpp>

using namespace geode::prelude;
using namespace cw::mod_cmmts;

void SelfDirector::onClosePopup(UploadActionPopup*) {
    if (m_authTask.isPending()) {
        Notification::create("Task cancelled", NotificationIcon::Error)->show();
        m_authTask.cancel();
    };

    cue::resetNode(m_authProgPopup);
};

void SelfDirector::setReport(uint64_t id, bool unset) {
    if (unset) {
        if (auto const it = m_reportedAds.find(id); it != m_reportedAds.end()) m_reportedAds.erase(it);
        return;
    };

    m_reportedAds.insert(id);
};

void SelfDirector::startAuth() {
    if (m_authTask.isPending()) return;

    m_authProgPopup = UploadActionPopup::create(this, "Authorizing...");
    m_authProgPopup->show();

    m_authTask.spawn(
        authorize(),
        [this](WebRes res) {
            if (res.isErr()) {
                log::error("{}: {}", res.getCode(), res.getError());
                m_authProgPopup->showFailMessage(fmt::format("Failed to authorize ({})", res.getCode()));

                return;
            };

            auto userRes = res.getPayload<CommentUser>();
            if (userRes.isErr()) return m_authProgPopup->showFailMessage("Unknown error");

            setCurrentUser(std::move(userRes).unwrap());

            auto getRes = getCurrentUser();
            if (getRes.isErr()) return m_authProgPopup->showFailMessage("Unknown error");

            m_authProgPopup->showSuccessMessage(fmt::format("Authorized as {}", getRes.unwrap().username));
        });
};

arc::Future<WebRes> SelfDirector::authorize() {
    log::warn("Authorizing user...");

    CW_MODCOMMENTS_ARGON_UNWRAP(auto token);

    matjson::Value body;
    body = user::getUserIcons();

    auto req = (co_await request::withAuthCo(std::move(token)))
                   .bodyJSON(body);

    co_return webres::processResp(co_await req.post("/v1/me"_api));
};

void SelfDirector::setCurrentUser(CommentUser user) {
    m_authorized = (user.id == GJAccountManager::sharedState()->m_accountID);
    m_user = std::move(user);
};

Result<const CommentUser> SelfDirector::getCurrentUser() const noexcept {
    if (isAuthorized()) return Ok(m_user);
    return Err("User is not logged in");
};

bool SelfDirector::isAuthorized() const noexcept {
    return argon::signedIn() && m_authorized;
};

bool SelfDirector::isReported(uint64_t id) const noexcept {
    auto const it = m_reportedAds.find(id);
    return it != m_reportedAds.end();
};