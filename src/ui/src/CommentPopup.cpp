#include "../CommentPopup.hpp"

#include <Util.h>

#include <Geode/Geode.hpp>

using namespace geode::prelude;
using namespace cw::mod_cmmts;

static constexpr auto g_commentWait = 30;

asp::Instant CommentsPopup::s_lastComment = asp::Instant();

bool CommentsPopup::init(std::string modID, bool geodeTheme) {
    m_modID = std::move(modID);

    if (!Popup::init(395.f, 265.f, geodeTheme ? "geode.loader/GE_square01.png" : "GJ_square01.png")) return false;

    setID(fmt::format("popup-{}", modID));
    setTitle("Mod Comments");

    setCloseButtonSpr(
        CircleButtonSprite::createWithSpriteFrameName(
            "geode.loader/close.png",
            0.875f,
            geodeTheme ? CircleBaseColor::DarkPurple : CircleBaseColor::Green),
        0.825f);

    auto cmmtBorder = cue::createBackground(
        {m_mainLayer->getScaledContentWidth() - 70.f, m_mainLayer->getScaledContentHeight() - 45.f},
        {
            .opacity = 255,
            .cornerRoundness = -0.75f,
            .texture = "geode.loader/black-square.png",
            .zOrder = 1,
            .id = "",
        });

    m_mainLayer->addChildAtPosition(cmmtBorder, Anchor::Center, {0.f, -12.5f});

    auto rulesBtn = Button::createWithNode(
        CircleButtonSprite::createWithSpriteFrameName(
            "geode.loader/news.png",
            0.75f,
            geodeTheme ? CircleBaseColor::DarkPurple : CircleBaseColor::Green),
        [](Button* sender) {
            popups::showRules();
        });
    rulesBtn->setID("comment-rules-btn");
    rulesBtn->setScale(0.625f);

    m_mainLayer->addChildAtPosition(rulesBtn, Anchor::BottomRight, {}, false);

    auto linkBtnMenuLayout = ColumnLayout::create()
                                 ->setGap(2.f)
                                 ->setAutoScale(false)
                                 ->setAutoGrowAxis(0.f);

    auto linkBtnMenu = CCNode::create();
    linkBtnMenu->setID("link-container");
    linkBtnMenu->setContentSize({12.5f, 1.25f});
    linkBtnMenu->setZOrder(1);
    linkBtnMenu->setLayout(linkBtnMenuLayout);

    m_mainLayer->addChildAtPosition(linkBtnMenu, Anchor::BottomLeft, {5.f, 5.f});

    auto linkBtns = std::array{
        LinkButton{
            "discord-btn",
            "gj_discordIcon_001.png",
            [](auto) {
                createQuickPopup(
                    "Discord Community",
                    "Join <cd>Cheeseworks</c>'s <cb>Discord server</c>?\n"
                    "<cs>Get help, report bugs, and chat with other players!</c>",
                    "Cancel",
                    "OK",
                    [](auto, bool ok) {
                        if (ok) web::openLinkInBrowser("https://www.dsc.gg/cheeseworks");
                    });
            },
        },
        LinkButton{
            "support-me-btn",
            "geode.loader/gift.png",
            [](auto) {
                openSupportPopup(Mod::get());
            },
        },
    };

    for (auto& linkBtn : linkBtns) {
        auto b = Button::createWithSpriteFrameName(
            linkBtn.sprite,
            std::move(linkBtn.callback));
        b->setID(std::move(linkBtn.id));
        b->setScale(0.75f);

        linkBtnMenu->addChild(b);
    };

    linkBtnMenu->updateLayout();

    auto infoBtn = Button::createWithSpriteFrameName(
        "GJ_infoIcon_001.png",
        [](auto) {
            createQuickPopup(
                "Help",
                "This is the <cg>Mod Comment Menu</c>. You can <cy>read and send comments about this mod</c> here. Remember to <cr>read and follow the rules</c>.",
                "OK",
                nullptr,
                nullptr);
        });
    infoBtn->setID("info-btn");
    infoBtn->setScale(0.75f);
    infoBtn->setZOrder(9);

    m_mainLayer->addChildAtPosition(infoBtn, Anchor::TopRight, {-15.f, -15.f});

    return true;
};

CommentsPopup* CommentsPopup::create(std::string modID, bool geodeTheme) {
    auto ret = new CommentsPopup();
    if (ret->init(std::move(modID), geodeTheme)) {
        ret->autorelease();
        return ret;
    };

    delete ret;
    return nullptr;
};