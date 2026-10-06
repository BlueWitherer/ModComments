#include "../CommentsPopup.hpp"

#include <Util.h>

#include <cue/PlayerIcon.hpp>

#include <Geode/Geode.hpp>

#include <Geode/utils/ColorProvider.hpp>

using namespace geode::prelude;
using namespace cw::mod_cmmts;

namespace cw::mod_cmmts {
    namespace impl {
        static constexpr uint8_t g_commentWait = 30;
        static constexpr uint8_t g_refreshWait = 2;

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

asp::Instant CommentsPopup::s_lastComment;
asp::Instant CommentsPopup::s_lastRefresh;

StringMap<GeodeMod> CommentsPopup::s_indexedMods;

bool CommentsPopup::init(std::string modID, bool geodeTheme) {
    m_modID = std::move(modID);

    if (!Popup::init(425.f, 265.f, geodeTheme ? "geode.loader/GE_square01.png" : "GJ_square01.png")) return false;

    setID(fmt::format("popup-{}", modID));

    setCloseButtonSpr(
        CircleButtonSprite::createWithSpriteFrameName(
            "geode.loader/close.png",
            0.875f,
            geodeTheme ? CircleBaseColor::DarkPurple : CircleBaseColor::Green),
        0.825f);

    m_geodeLoading = LoadingSpinner::create(20.f);
    m_mainLayer->addChildAtPosition(m_geodeLoading, Anchor::Top, {0.f, -18.75f});

    auto addModNode = [this](CommentModNode* modNode) {
        m_mainLayer->addChildAtPosition(modNode, Anchor::Top, {0.f, -8.25f});
        m_geodeLoading->setVisible(false);
    };

    if (Loader::get()->isModInstalled(m_modID)) {
        auto modNode = CommentModNode::create(m_modID);
        addModNode(modNode);
    } else {
        if (auto const it = s_indexedMods.find(m_modID); it != s_indexedMods.end()) {
            auto modNode = CommentModNode::create(m_modID, it->second);
            addModNode(modNode);
        };

        m_geodeTask.spawn(
            getGeodeData(),
            [this, addModNode = std::move(addModNode)](WebRes res) {
                if (res.isErr()) return log::error("{}: {}", res.getCode(), res.getError());

                auto dataRes = res.getPayload<GeodeMod>();
                if (dataRes.isErr()) return log::error("Failed to serialize response: {}", dataRes.unwrapErr());

                s_indexedMods[m_modID] = std::move(dataRes).unwrap();

                auto modNode = CommentModNode::create(m_modID, s_indexedMods[m_modID]);
                addModNode(modNode);
            });
    };

    auto cmmtBorder = cue::createBackground(
        {m_mainLayer->getScaledContentWidth() - 70.f, m_mainLayer->getScaledContentHeight() - 40.f},
        {
            .opacity = 255,
            .texture = "geode.loader/black-square.png",
            .zOrder = 1,
            .id = "",
        });
    cmmtBorder->setAnchorPoint({0.5, 0});

    m_mainLayer->addChildAtPosition(cmmtBorder, Anchor::Bottom, {0.f, 8.75f});

    m_loading = LoadingSpinner::create(50.f);
    m_loading->setID("loading-circle");
    m_loading->setVisible(false);

    cmmtBorder->addChildAtPosition(m_loading, Anchor::Center);

    m_commentList = ScrollLayer::create({cmmtBorder->getScaledContentWidth() - 12.5f, cmmtBorder->getScaledContentHeight() - (showInput() ? 45.f : 12.5f)});
    m_commentList->setID("comment-list");
    m_commentList->setAnchorPoint({0.5, 1});
    m_commentList->ignoreAnchorPointForPosition(false);

    auto commentListLayout = static_cast<SimpleColumnLayout*>(ScrollLayer::createDefaultListLayout(3.75f))
                                 ->setMainAxisDirection(AxisDirection::BottomToTop);

    m_commentList->m_contentLayer->setLayout(commentListLayout);

    cmmtBorder->addChildAtPosition(m_commentList, Anchor::Top, {0.f, -5.f});

    m_commentList->m_contentLayer->updateLayout();
    m_commentList->scrollToTop();

    m_errLabel = Label::create("Something went wrong...", "goldFont.fnt");
    m_errLabel->setID("error-label");
    m_errLabel->setZOrder(9);
    m_errLabel->setScale(0.475f);
    m_errLabel->setAlignment(Label::Alignment::Center);
    m_errLabel->setVisible(false);

    cmmtBorder->addChildAtPosition(m_errLabel, Anchor::Center);

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

    m_commentMenu->setVisible(showInput());

    auto sendBtnSpr = EditorButtonSprite::createWithSpriteFrameName(
        "GJ_chatBtn_01_001.png",
        0.925f,
        geodeTheme ? EditorBaseColor::DarkGray : EditorBaseColor::Green);

    if (auto ico = sendBtnSpr->getChildByType<CCSprite>(0)) {
        ico->setFlipX(true);
        ico->setRotation(-90);
    };

    auto sendBtn = Button::createWithNode(
        sendBtnSpr,
        [this](Button* sender) {
            onSend(sender);
        });
    sendBtn->setID("send-comment-btn");

    cue::rescaleToMatch(sendBtn, 26.25f);

    m_commentMenu->addChild(sendBtn);

    m_commentMenu->updateLayout();

    addEventListener(
        KeyboardInputEvent(enumKeyCodes::KEY_Enter),
        [this, sendBtn](KeyboardInputData& data) {
            if (data.action == KeyboardInputData::Action::Press) {
                if (m_inputBox->getInputNode()->m_selected) onSend(sendBtn);
            };
        });

    auto linkBtnMenuLayout = ColumnLayout::create()
                                 ->setGap(2.f)
                                 ->setAutoScale(false)
                                 ->setAutoGrowAxis(0.f);

    auto linkBtnMenu = CCNode::create();
    linkBtnMenu->setID("link-container");
    linkBtnMenu->setContentSize({12.5f, 1.25f});
    linkBtnMenu->setZOrder(1);
    linkBtnMenu->setLayout(linkBtnMenuLayout);

    m_mainLayer->addChildAtPosition(linkBtnMenu, Anchor::BottomLeft, {7.5f, 7.5f});

    auto linkBtns = std::array{
        LinkButton{
            "comment-rules-btn",
            "accountBtn_myLists_001.png",
            [](auto) {
                popups::showRules();
            },
        },
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

        cue::rescaleToMatch(b, 22.5f);

        linkBtnMenu->addChild(b);
    };

    linkBtnMenu->updateLayout();

    m_refreshBtn = Button::createWithNode(
        CircleButtonSprite::createWithSpriteFrameName(
            "geode.loader/reload.png",
            0.975f,
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

void CommentsPopup::onSend(Button* sender) {
    m_inputBox->defocus();

    auto elapsed = asp::Instant::now().durationSince(s_lastComment).seconds();
    if (elapsed < impl::g_commentWait) {
        createQuickPopup(
            "Slow Down!",
            fmt::format("You must <co>wait {} seconds before sending your next comment</c>!", impl::g_commentWait - elapsed),
            "OK",
            nullptr,
            nullptr);

        return;
    };

    if (str::trim(m_inputBox->getString()).empty()) return Notification::create("Comment cannot be empty.", NotificationIcon::Error)->show();

    sender->setEnabled(false);
    if (auto spr = typeinfo_cast<CCSprite*>(sender->getDisplayNode())) spr->setColor({50, 50, 50});

    m_refreshBtn->setVisible(false);

    m_commentTask.spawn(
        sendComment(),
        [this, sender](WebRes res) {
            if (res.isOk()) {
                s_lastComment = asp::Instant::now();

                m_inputBox->setString("", false);
                refreshComments();
            } else if (res.isErr()) {
                m_refreshBtn->setVisible(true);
                Notification::create(fmt::format("Error {}", res.getCode()), NotificationIcon::Error)->show();
            };

            if (auto spr = typeinfo_cast<CCSprite*>(sender->getDisplayNode())) spr->setColor({255, 255, 255});
            sender->setEnabled(true);
        });
};

arc::Future<WebRes> CommentsPopup::getGeodeData() {
    co_return webres::processResp(co_await request::base().get(fmt::format("https://api.geode-sdk.org/v1/mods/{}", m_modID)));
};

arc::Future<WebRes> CommentsPopup::deleteComment(uint64_t id) {
    CW_MODCOMMENTS_ARGON_UNWRAP(auto token);

    auto req = (co_await request::withAuthCo(std::move(token)))
                   .param("comment", id);

    co_return webres::processResp(co_await req.send("DELETE", "/v1/comments/delete"_api));
};

arc::Future<WebRes> CommentsPopup::reportComment(uint64_t id, std::string reason) {
    CW_MODCOMMENTS_ARGON_UNWRAP(auto token);

    matjson::Value body;
    body["comment"] = id;
    body["reason"] = std::move(reason);

    auto req = (co_await request::withAuthCo(std::move(token)))
                   .bodyJSON(body);

    co_return webres::processResp(co_await req.post("/v1/reports/send"_api));
};

void CommentsPopup::onDelete(Comment const& cmmt) {
    createQuickPopup(
        "Delete Comment",
        "<cr>Delete</c> this comment?",
        "Cancel",
        "Yes",
        [this, &cmmt](auto, bool ok) {
            if (ok) m_commentActionTask.spawn(
                deleteComment(cmmt.id),
                [this](WebRes res) {
                    if (res.isOk()) return refreshComments();
                    Notification::create(fmt::format("Failed to delete comment ({})", res.getCode()), NotificationIcon::Error)->show();
                });
        });
};

void CommentsPopup::onReport(Comment const& cmmt) {};

arc::Future<WebRes> CommentsPopup::getComments() {
    auto req = request::base()
                   .param("mod", m_modID)
                   .param("page", m_page);

    co_return webres::processResp(co_await req.get("/v1/comments/get"_api));
};

arc::Future<WebRes> CommentsPopup::sendComment() {
    CW_MODCOMMENTS_ARGON_UNWRAP(auto token);

    matjson::Value body;
    body["mod"] = m_modID;
    body["content"] = *co_await async::waitForMainThread<std::string>([self = WeakRef(this)]() {
        if (auto s = self.lock()) return std::string{s->m_inputBox->getString()};
        return std::string{};
    });
    body["icons"] = impl::getUserIcons();

    auto req = (co_await request::withAuthCo(std::move(token)))
                   .bodyJSON(body);

    co_return webres::processResp(co_await req.post("/v1/comments/send"_api));
};

void CommentsPopup::refreshComments() {
    auto elapsed = asp::Instant::now().durationSince(s_lastRefresh).seconds();
    if (elapsed < impl::g_refreshWait) return;

    s_lastRefresh = asp::Instant::now();

    m_commentList->setVisible(false);
    m_commentMenu->setVisible(false);

    m_refreshBtn->setVisible(false);

    m_loading->setVisible(true);

    m_errLabel->setVisible(false);

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

                    auto cell = CommentItem::create(
                        std::move(cmmtRes).unwrap(),
                        m_commentList->getScaledContentWidth(),
                        m_geodeTheme);
                    cell->setActionCallback([this](CommentAction act, Comment const& cmmt) {
                        switch (act) {
                            default: return;

                            case CommentAction::Delete: return onDelete(cmmt);
                            case CommentAction::Report: return onReport(cmmt);
                        };
                    });

                    m_commentList->m_contentLayer->addChild(cell);
                };

                m_commentList->m_contentLayer->updateLayout();
                m_commentList->scrollToTop();

                m_commentList->setVisible(true);
                m_commentMenu->setVisible(showInput());
            } else {
                m_errLabel->setVisible(true);
            };

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