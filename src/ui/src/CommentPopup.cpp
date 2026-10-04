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

namespace cw::mod_cmmts {
    namespace impl {
        static constexpr auto g_commentWait = 30;

        static auto getUserIcons() {
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

Result<CommentRequest> matjson::Serialize<CommentRequest>::fromJson(matjson::Value const& value) {
    CommentRequest out;

    GEODE_UNWRAP_INTO(out.modID, value["mod"].asString());
    GEODE_UNWRAP_INTO(out.content, value["content"].asString());
    GEODE_UNWRAP_INTO(out.icons, value["icons"].as<UserIcons>());

    return Ok(std::move(out));
};

matjson::Value matjson::Serialize<CommentRequest>::toJson(CommentRequest const& value) {
    Value out;
    out["mod"] = value.modID;
    out["content"] = value.content;
    out["icons"] = value.icons;

    return out;
};

bool CommentItem::init(Comment cmmt, float width, bool geodeTheme) {
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
    bg->setColor(colors->color3b(
        isSelf()  // :3c
            ? geodeTheme
                  ? "geode.loader/mod-developer-item-bg"
                  : "geode.loader/mod-list-tab-selected-bg"
            : geodeTheme
                  ? "geode.loader/mod-list-tab-selected-bg"
                  : "geode.loader/mod-developer-item-bg"));

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

    cue::rescaleToMatch(icon, 15.f);

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
    m_contentLabel->setScale(0.5f);
    m_contentLabel->setAnchorPoint({0, 1});
    m_contentLabel->setMaxWidth(getScaledContentWidth() - 20.f);

    addChildAtPosition(m_contentLabel, Anchor::TopLeft, {27.5f, -25.f});

    Button* actionBtn = nullptr;

    if (isSelf()) {
        actionBtn = Button::createWithSpriteFrameName(
            "GJ_trashBtn_001.png",
            [](auto) {
                log::info("delete button!");
            });
        actionBtn->setID("delete-comment-btn");
    } else {
        actionBtn = Button::createWithNode(
            CircleButtonSprite::createWithSpriteFrameName(
                "geode.loader/exclamation-red.png",
                0.875f,
                geodeTheme ? CircleBaseColor::DarkPurple : CircleBaseColor::Green),
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

CommentItem* CommentItem::create(Comment cmmt, float width, bool geodeTheme) {
    auto ret = new CommentItem();
    if (ret->init(std::move(cmmt), width, geodeTheme)) {
        ret->autorelease();
        return ret;
    };

    delete ret;
    return nullptr;
};

// asp::Instant CommentsPopup::s_lastComment;
// StringSet CommentsPopup::s_validMods;

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
        {m_mainLayer->getScaledContentWidth() - 25.f, m_mainLayer->getScaledContentHeight() - 45.f},
        {
            .opacity = 255,
            .texture = "geode.loader/black-square.png",
            .zOrder = 1,
            .id = "",
        });

    m_mainLayer->addChildAtPosition(cmmtBorder, Anchor::Center, {0.f, -12.5f});

    m_loading = LoadingSpinner::create(50.f);
    m_loading->setID("loading-circle");
    m_loading->setVisible(false);

    cmmtBorder->addChildAtPosition(m_loading, Anchor::Center);

    m_commentList = ScrollLayer::create({cmmtBorder->getScaledContentWidth() - 12.5f, cmmtBorder->getScaledContentHeight() - 45.f});
    m_commentList->setID("comment-list");
    m_commentList->setAnchorPoint({0.5, 1});
    m_commentList->ignoreAnchorPointForPosition(false);

    m_commentList->m_contentLayer->setLayout(ScrollLayer::createDefaultListLayout());

    cmmtBorder->addChildAtPosition(m_commentList, Anchor::Top, {0.f, -5.f});

    m_commentList->m_contentLayer->updateLayout();

    auto sendMenuLayout = RowLayout::create()
                              ->setAutoScale(false)
                              ->setAxisAlignment(AxisAlignment::Between);

    m_commentMenu = CCNode::create();
    m_commentMenu->setID("send-container");
    m_commentMenu->setAnchorPoint({0.5, 0});
    m_commentMenu->setContentSize({m_commentList->getScaledContentWidth(), 25.f});
    m_commentMenu->setLayout(sendMenuLayout);

    cmmtBorder->addChildAtPosition(m_commentMenu, Anchor::Bottom, {0.f, 5.f});

    m_inputBox = TextInput::create(m_commentMenu->getScaledContentWidth() - 2.5f, "Share your thoughts...", "chatFont.fnt");
    m_inputBox->setID("comment-text-input");
    m_inputBox->setScale(0.925f);
    m_inputBox->setTextAlign(TextInputAlign::Left);
    m_inputBox->setCommonFilter(CommonFilter::Any);

    m_commentMenu->addChild(m_inputBox);

    auto sendBtn = Button::createWithNode(
        EditorButtonSprite::createWithSpriteFrameName(
            "GJ_chatBtn_01_001.png",
            0.925f,
            geodeTheme ? EditorBaseColor::DarkGray : EditorBaseColor::Green),
        [this](Button* sender) {
            sender->setEnabled(false);
            if (auto spr = typeinfo_cast<CCSprite*>(sender->getDisplayNode())) spr->setColor({50, 50, 50});

            m_refreshBtn->setVisible(false);

            m_commentTask.spawn(
                sendComment(),
                [this, sender](WebRes res) {
                    if (res.isOk()) refreshComments();
                    if (res.isErr()) {
                        m_refreshBtn->setVisible(true);

                        Notification::create(fmt::format("Error {}", res.getCode()), NotificationIcon::Error)->show();
                    };

                    if (auto spr = typeinfo_cast<CCSprite*>(sender->getDisplayNode())) spr->setColor({255, 255, 255});
                    sender->setEnabled(true);
                });
        });
    sendBtn->setID("send-comment-btn");

    cue::rescaleToMatch(sendBtn, 26.25f);

    m_commentMenu->addChild(sendBtn);

    m_commentMenu->updateLayout();

    m_commentMenu->setVisible(showInput());

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

    m_mainLayer->addChildAtPosition(rulesBtn, Anchor::BottomLeft, {}, false);

    m_refreshBtn = Button::createWithNode(
        CircleButtonSprite::createWithSpriteFrameName(
            "geode.loader/reload.png",
            0.925f,
            geodeTheme ? CircleBaseColor::DarkPurple : CircleBaseColor::Green),
        [this](Button* sender) {
            refreshComments();
        });
    m_refreshBtn->setID("refresh-comments-btn");
    m_refreshBtn->setScale(0.625f);

    m_mainLayer->addChildAtPosition(m_refreshBtn, Anchor::BottomRight, {}, false);

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

    refreshComments();

    return true;
};

arc::Future<WebRes> CommentsPopup::getComments() {
    CW_MODCOMMENTS_ARGON_UNWRAP(auto token);

    auto req = request::base()
                   .param("mod", m_modID);

    co_return webres::processResp(co_await req.get("/v1/comments/get"_api));
};

arc::Future<WebRes> CommentsPopup::sendComment() {
    CW_MODCOMMENTS_ARGON_UNWRAP(auto token);

    matjson::Value body;
    body["mod"] = m_modID;
    body["content"] = *co_await async::waitForMainThread<std::string>([self = WeakRef(this)]() {
        if (auto s = self.lock()) return s->m_inputBox->getString();
        return std::string{};
    });
    body["icons"] = impl::getUserIcons();

    auto req = (co_await request::withAuthCo(std::move(token)))
                   .bodyJSON(body);

    co_return webres::processResp(co_await req.post("/v1/comments/send"_api));
};

void CommentsPopup::refreshComments() {
    m_commentList->setVisible(false);
    m_commentMenu->setVisible(false);

    m_refreshBtn->setVisible(false);

    m_loading->setVisible(true);

    m_commentList->m_contentLayer->removeAllChildren();

    m_commentTask.spawn(
        getComments(),
        [this](WebRes res) {
            if (res.isOk()) {
                auto const fallback = [code = res.getCode()](std::string_view err) {
                    Notification::create(fmt::format("Comments failed to load ({})", code), NotificationIcon::Error);
                    log::error("{}: {}", code, err);
                };

                auto arrayRes = std::move(res).getPayloadValue().asArray();
                if (arrayRes.isErr()) return fallback(arrayRes.unwrapErr());

                auto const array = std::move(arrayRes).unwrap();

                for (auto const& val : array) {
                    auto cmmtRes = val.as<Comment>();
                    if (cmmtRes.isErr()) {
                        log::error("Failed: {}", cmmtRes.unwrapErr());
                        continue;
                    };

                    m_commentList->m_contentLayer->addChild(
                        CommentItem::create(
                            std::move(cmmtRes).unwrap(),
                            m_commentList->getScaledContentWidth(),
                            m_geodeTheme));
                };

                m_commentList->m_contentLayer->updateLayout();
            };

            m_commentList->setVisible(true);
            m_commentMenu->setVisible(showInput());

            m_refreshBtn->setVisible(true);

            m_loading->setVisible(false);
        });
};

bool CommentsPopup::showInput() const {
    return argon::signedIn() && Loader::get()->isModInstalled(m_modID);
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