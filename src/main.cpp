#include <Util.h>

#include <util/base/Singleton.hpp>

#include <Geode/Geode.hpp>

#include <alphalaneous.alphas_geode_utils/include/ObjectModify.hpp>

using namespace geode::prelude;
using namespace cw::mod_cmmts;

namespace cw::mod_cmmts {
    namespace main {
        static constexpr std::string_view g_urlGeode = "https://geode-sdk.org/mods/";
        static StringSet validMods;
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
        });

    ButtonSettingPressedEvent(
        Mod::get(),
        "btn")
        .listen([](std::string_view buttonKey) {
            SelfDirector::get()->startAuth();
        })
        .leak();
};

class IndexTaskDelegate final : public cw::mod_cmmts::base::Singleton<IndexTaskDelegate>, public UploadPopupDelegate {
public:
    TaskHolder<WebRes> m_checkIndexTask;
    Ref<UploadActionPopup> m_popup = nullptr;

    Ref<UploadActionPopup>& createProgressPopup() {
        cue::resetNode(m_popup);

        m_popup = UploadActionPopup::create(this, "Checking Geode index...");
        m_popup->show();

        return m_popup;
    };

    void onClosePopup(UploadActionPopup*) override {
        m_checkIndexTask.cancel();
        cue::resetNode(m_popup);
    };

    arc::Future<WebRes> checkModIndex(std::string modID) {
        auto res = co_await request::base().get(fmt::format("https://api.geode-sdk.org/v1/mods/{}", modID));
        co_return webres::processResp(res);
    };
};

class $nodeModify(CommentsModPopup, ModPopup) {
    struct Fields final {
        std::string id;
        bool geodeTheme = false;

        void createPopup() {
            if (!mustAgreeToRules()) return CommentsPopup::create(id, geodeTheme)->show();

            RulesPopup::create(
                [this](RulesPopup* sender, bool agreed) {
                    sender->removeFromParent();

                    if (!agreed) return;

                    Mod::get()->setSavedValue("agreed-rules", true);
                    CommentsPopup::create(id, geodeTheme)->show();
                },
                geodeTheme)
                ->show();
        };

        bool mustAgreeToRules() const {
            return argon::signedIn() && Loader::get()->isModInstalled(id) && !Mod::get()->getSavedValue("agreed-rules", false);
        };
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
                        [f](auto) {
                            if (auto const it = main::validMods.find(f->id); it != main::validMods.end()) return f->createPopup();

                            auto delegate = IndexTaskDelegate::get();
                            auto& popup = delegate->createProgressPopup();

                            delegate->m_checkIndexTask.spawn(
                                delegate->checkModIndex(f->id),
                                [f, &popup](WebRes res) {
                                    if (res.isOk()) {
                                        auto metaRes = res.getPayload<GeodeMod>();
                                        if (metaRes.isErr()) {
                                            log::error("Failed to parse Geode index response: {}", metaRes.unwrapErr());
                                            popup->showFailMessage("Unknown error");

                                            return;
                                        };

                                        auto const meta = std::move(metaRes).unwrap();
                                        if (meta.versions.empty()) {
                                            popup->showFailMessage("Mod is delisted");
                                            return;
                                        };

                                        popup->removeFromParent();
                                        main::validMods.insert(f->id);

                                        f->createPopup();

                                        return;
                                    };

                                    popup->showFailMessage("Mod is unavailable");
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
};

void popups::showRules() {
    MDPopup::create(
        "Comment Rules",
        rules::fullText,
        "OK")
        ->show();
};