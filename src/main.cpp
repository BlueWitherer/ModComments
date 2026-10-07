#include <Util.h>

#include <Geode/Geode.hpp>

#include <alphalaneous.alphas_geode_utils/include/ObjectModify.hpp>

using namespace geode::prelude;
using namespace cw::mod_cmmts;

namespace cw::mod_cmmts {
    namespace main {
        static constexpr std::string_view g_urlGeode = "https://geode-sdk.org/mods/";
        static StringSet g_validMods;
    };
};

$on_game(Loaded) {
    log::debug("Using web API url: {}", url::apiBase);

    auto sd = SelfDirector::get();

    async::spawn(
        sd->authorize(),
        [sd](WebRes res) {
            if (res.isErr()) return log::error("{}: {}", res.getCode(), res.getError());

            auto userRes = res.getPayload<CommentUser>();
            if (userRes.isErr()) return log::error("Failed to parse payload: {}", userRes.unwrapErr());

            sd->setCurrentUser(std::move(userRes).unwrap());
            sd->authProgressing(false);
        });

    ButtonSettingPressedEvent(
        Mod::get(),
        "btn")
        .listen([](std::string_view buttonKey) {
            SelfDirector::get()->startAuth();
        })
        .leak();
};

class $nodeModify(CommentsModPopup, ModPopup) {
    struct Fields final {
        std::string id;
        bool geodeTheme = false;

        TaskHolder<WebRes> checkIndexTask;
    };

    void modify() {
        auto f = m_fields.self();
        f->geodeTheme = Loader::get()->getLoadedMod(CW_GEODE_ID)->getSettingValue<std::string>("used-theme") != "Geometry Dash";

        if (auto res = getThisID(); res.isOk()) f->id = std::move(res).unwrap();

        if (auto self = reinterpret_cast<FLAlertLayer*>(this)) {
            if (auto displayNode = self->m_mainLayer->getChildByType<CCNode*>(2)) {
                if (auto tabsMenu = displayNode->querySelector("right-column > tabs-menu")) {
                    tabsMenu->setPosition({0.f, tabsMenu->getPositionY() - 2.f});
                    tabsMenu->setAnchorPoint({0, 1});
                    tabsMenu->setContentWidth(tabsMenu->getContentWidth() - 21.25f);

                    auto tabSprite = TabSprite::create("geode.loader/message.png", "Comments", 140.f);
                    tabSprite->select(false);

                    auto tab = CCMenuItemExt::createSpriteExtra(
                        tabSprite,
                        [this, f](auto) {
                            if (auto const it = main::g_validMods.find(f->id); it != main::g_validMods.end()) return CommentsPopup::create(f->id, f->geodeTheme)->show();

                            auto popup = UploadActionPopup::create(nullptr, fmt::format("Checking Geode index...", f->id));
                            popup->show();

                            f->checkIndexTask.spawn(
                                checkModIndex(f->id),
                                [f, p = WeakRef(popup)](WebRes res) {
                                    if (res.isOk()) {
                                        auto metaRes = res.getPayload<GeodeMod>();
                                        if (metaRes.isErr()) {
                                            log::error("Failed to parse Geode index response: {}", metaRes.unwrapErr());
                                            if (auto popup = p.lock()) popup->showFailMessage("Unknown error");

                                            return;
                                        };

                                        auto const meta = std::move(metaRes).unwrap();
                                        if (meta.versions.empty()) {
                                            if (auto popup = p.lock()) popup->showFailMessage("Mod is delisted");
                                            return;
                                        };

                                        if (auto popup = p.lock()) popup->removeFromParent();
                                        CommentsPopup::create(f->id, f->geodeTheme)->show();

                                        main::g_validMods.insert(f->id);

                                        return;
                                    };

                                    if (auto popup = p.lock()) popup->showFailMessage("Mod is unavailable");
                                });
                        });
                    tab->setID("comments-btn"_spr);
                    tab->setTag(tabsMenu->getChildrenCount());

                    tabsMenu->addChild(tab);
                    tabsMenu->updateLayout();
                };
            };
        };
    };

    Result<std::string> getThisID() {
        if (auto self = reinterpret_cast<FLAlertLayer*>(this)) {
            log::trace("Searching for mod page button in popup");

            if (auto modPageBtn = self->m_buttonMenu->getChildByID("mod-online-page-button")) {
                log::trace("Mod page button found");

                if (auto url = typeinfo_cast<CCString*>(modPageBtn->getUserObject("url"))) {
                    log::trace("URL string object found in mod page button");

                    std::string urlStr = url->getCString();

                    if (utils::string::startsWith(urlStr, main::g_urlGeode)) return Ok(urlStr.erase(0, main::g_urlGeode.size()));
                    return Err("Mod ID not found");
                };

                return Err("Mod page button does not have a valid URL object");
            };

            return Err("Mod page button not found in popup");
        };

        return Err("Could not cast this to FLAlertLayer");
    };

    arc::Future<WebRes> checkModIndex(std::string modID) {
        auto res = co_await request::base().get(fmt::format("https://api.geode-sdk.org/v1/mods/{}", modID));
        co_return webres::processResp(res);
    };
};

void popups::showRules() {
    MDPopup::create(
        "Comment Rules",
        popups::g_rulesText,
        "OK")
        ->show();
};