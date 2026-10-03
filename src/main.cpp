#include <Util.h>

#include <Geode/Geode.hpp>

#include <alphalaneous.alphas_geode_utils/include/ObjectModify.hpp>

using namespace geode::prelude;
using namespace cw::mod_cmmts;

static constexpr std::string_view urlGeode = "https://geode-sdk.org/mods/";

class $nodeModify(CommentsModPopup, ModPopup) {
    struct Fields final {
        std::string id;
        bool geodeTheme = false;
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
                        [f](auto sender) {
                            CommentsPopup::create(f->id, f->geodeTheme)->show();
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

                    if (utils::string::startsWith(urlStr, urlGeode)) return Ok(urlStr.erase(0, urlGeode.size()));
                    return Err("Mod ID not found");
                };

                return Err("Mod page button does not have a valid URL object");
            };

            return Err("Mod page button not found in popup");
        };

        return Err("Could not cast this to FLAlertLayer");
    };
};