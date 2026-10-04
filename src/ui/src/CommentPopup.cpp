#include "../CommentPopup.hpp"

#include <Util.h>

#include <cue/PlayerIcon.hpp>

#include <Geode/Geode.hpp>

#include <Geode/utils/ColorProvider.hpp>

using namespace geode::prelude;
using namespace cw::mod_cmmts;

#define CW_MODCOMMENTS_ARGON_UNWRAP(var)                                                            \
    auto tokenRes = co_await argon::startAuth();                                                    \
    if (tokenRes.isErr()) co_return WebRes(std::nullptr_t(), std::move(tokenRes).unwrapErr(), 402); \
    var = std::move(tokenRes).unwrap()

static constexpr auto g_commentWait = 30;

bool CommentItem::init(Comment cmmt, float width) {
    m_comment = std::move(cmmt);

    if (!CCNode::init()) return false;

    setAnchorPoint({0.5, 0});
    setContentSize({width, 45.f});

    auto colors = ColorProvider::get();

    auto bg = cue::createBackground(
        getScaledContentSize(),
        {
            .opacity = static_cast<uint8_t>(isSelf() ? 125 : 100),
            .texture = "geode.loader/white-square.png",
            .zOrder = -1,
            .id = "",
        });
    bg->setColor(colors->color3b(isSelf() ? "geode.loader/mod-developer-item-bg" : "geode.loader/mod-list-tab-selected-bg"));

    addChildAtPosition(bg, Anchor::Center);

    auto userMenuLayout = RowLayout::create()
                              ->setGap(5.f)
                              ->setAutoScale(false)
                              ->setAxisAlignment(AxisAlignment::Start)
                              ->setAutoGrowAxis(0.f)
                              ->setGrowCrossAxis(true);

    auto userMenu = CCNode::create();
    userMenu->setID("username-container");
    userMenu->setAnchorPoint({0, 1});
    userMenu->setLayout(userMenuLayout);

    addChildAtPosition(userMenu, Anchor::TopLeft, {5.f, -5.f});

    auto& user = m_comment.author;

    auto icon = cue::PlayerIcon::create(user.iconType, user.icon, user.color1, user.color2, user.useGlow ? user.colorGlow : -1);
    icon->setID("player-icon");

    cue::rescaleToMatch(icon, 20.f);

    userMenu->addChild(icon);

    auto username = Button::createWithLabel(
        user.username,
        "goldFont.fnt",
        [id = user.id, self = isSelf()](auto) {
            ProfilePage::create(id, self)->show();
        });
    username->setID("view-player-btn");
    username->setScale(0.625f);

    userMenu->addChild(username);

    userMenu->updateLayout();

    m_contentLabel = Label::create(m_comment.content, "geode.loader/mdFont.fnt");
    m_contentLabel->setID("comment-content-label");
    m_contentLabel->setScale(0.625f);
    m_contentLabel->setAnchorPoint({0, 1});
    m_contentLabel->setMaxWidth(getScaledContentWidth() - 20.f);

    addChildAtPosition(m_contentLabel, Anchor::TopLeft, {32.5f, -25.f});

    Button* actionBtn = nullptr;

    if (isSelf()) {
        actionBtn = Button::createWithSpriteFrameName(
            "GJ_trashBtn_001.png",
            [](auto) {
                log::info("delete button!");
            });
        actionBtn->setID("delete-comment-btn");
    } else {
        actionBtn = Button::createWithSpriteFrameName(
            "geode.loader/info-alert.png",
            [](auto) {
                log::warn("report button!");
            });
        actionBtn->setID("report-comment-btn");
    };

    cue::rescaleToMatch(actionBtn, 20.f);

    addChildAtPosition(actionBtn, Anchor::TopRight, actionBtn->getScaledContentSize() * -0.625f);

    return true;
};

bool CommentItem::isSelf() const noexcept {
    return GJAccountManager::sharedState()->m_accountID == m_comment.author.id;
};

CommentItem* CommentItem::create(Comment cmmt, float width) {
    auto ret = new CommentItem();
    if (ret->init(std::move(cmmt), width)) {
        ret->autorelease();
        return ret;
    };

    delete ret;
    return nullptr;
};

asp::Instant CommentsPopup::s_lastComment = asp::Instant();

bool CommentsPopup::init(std::string modID, bool geodeTheme) {
    m_modID = std::move(modID);

    if (!Popup::init(415.f, 265.f, geodeTheme ? "geode.loader/GE_square01.png" : "GJ_square01.png")) return false;

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
            .texture = "geode.loader/black-square.png",
            .zOrder = 1,
            .id = "",
        });

    m_mainLayer->addChildAtPosition(cmmtBorder, Anchor::Center, {0.f, -12.5f});

    m_commentList = ScrollLayer::create(cmmtBorder->getScaledContentSize() * 0.925f);
    m_commentList->setID("comment-list");
    m_commentList->setAnchorPoint({0.5, 0.5});
    m_commentList->ignoreAnchorPointForPosition(false);

    m_commentList->m_contentLayer->setLayout(ScrollLayer::createDefaultListLayout());

    cmmtBorder->addChildAtPosition(m_commentList, Anchor::Center, {}, false);

    m_commentList->m_contentLayer->addChild(CommentItem::create(
        Comment{
            1,
            CommentUser{
                UserIcons{
                    28,
                    IconType::Cube,
                    94,
                    98,
                    12,
                    true,
                },
                6408873,
                "Cheeseworks",
                true,
                ModLevel::None,
                asp::SystemTime::now(),
            },
            m_modID,
            "i hate animated fire!!!",
            asp::SystemTime::now(),
        },
        m_commentList->m_contentLayer->getScaledContentWidth()));
    m_commentList->m_contentLayer->addChild(CommentItem::create(
        Comment{
            1,
            CommentUser{
                UserIcons{
                    104,
                    IconType::Cube,
                    21,
                    3,
                    3,
                    true,
                },
                1941705,
                "RayDeeUx",
                false,
                ModLevel::None,
                asp::SystemTime::now(),
            },
            m_modID,
            "professional vibecoder :3c",
            asp::SystemTime::now(),
        },
        m_commentList->m_contentLayer->getScaledContentWidth()));
    m_commentList->m_contentLayer->addChild(CommentItem::create(
        Comment{
            1,
            CommentUser{
                UserIcons{
                    102,
                    IconType::Cube,
                    12,
                    12,
                    1,
                    false,
                },
                11535118,
                "alk1m123",
                false,
                ModLevel::None,
                asp::SystemTime::now(),
            },
            m_modID,
            "alk1m123",
            asp::SystemTime::now(),
        },
        m_commentList->m_contentLayer->getScaledContentWidth()));
    m_commentList->m_contentLayer->addChild(CommentItem::create(
        Comment{
            1,
            CommentUser{
                UserIcons{
                    296,
                    IconType::Cube,
                    0,
                    13,
                    1,
                    false,
                },
                20255553,
                "TeamSEX",
                false,
                ModLevel::None,
                asp::SystemTime::now(),
            },
            m_modID,
            "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",
            asp::SystemTime::now(),
        },
        m_commentList->m_contentLayer->getScaledContentWidth()));
    m_commentList->m_contentLayer->updateLayout();

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

arc::Future<WebRes> CommentsPopup::getComments() {
    CW_MODCOMMENTS_ARGON_UNWRAP(auto token);

    auto req = (co_await request::withAuthCo(std::move(token)))
                   .param("mod", m_modID);

    co_return webres::processResp(co_await req.get("/v1/comments/get"_api));
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