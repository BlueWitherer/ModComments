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
        static constexpr auto g_refreshWait = 3;

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

void CommentItem::addVoteNodes(CCNode* to, Button*& btn, Ref<Label>& label, CommentVote type) {
    auto like = type == CommentVote::Like;

    label = Label::create(fmt::format("{}", like ? m_comment.likes : m_comment.dislikes), "bigFont.fnt");
    label->setScale(0.375f);

    to->addChild(label);

    btn = Button::createWithSpriteFrameName(
        like ? "GJ_likesIcon_001.png" : "GJ_dislikesIcon_001.png",
        [this, label, t = type, like](auto) {
            auto type = t;

            log::trace("my vote is {}", m_comment.myVote);
            if (m_comment.myVote == (like ? 1 : -1)) type = CommentVote::None;

            m_likeBtn->setEnabled(false);
            m_dislikeBtn->setEnabled(false);

            if ((like ? m_comment.likes : m_comment.dislikes) > 0) (like ? m_likeLabel : m_dislikeLabel)->setText(numToString((like ? m_comment.likes : m_comment.dislikes) + 1));
            if ((like ? m_comment.dislikes : m_comment.likes) > 0) (like ? m_dislikeLabel : m_likeLabel)->setText(numToString((like ? m_comment.dislikes : m_comment.likes) - 1));

            m_voteTask.spawn(
                sendVote(type),
                [this, label, like](WebRes res) {
                    auto const fallback = [this, res, &label, like](std::string_view err) {
                        log::error("{}: {}", res.getCode(), err);
                        label->setText(numToString((like ? m_comment.likes : m_comment.dislikes)));
                    };

                    if (res.isErr()) return fallback(res.getError());

                    auto cmmtRes = res.getPayload<Comment>();
                    if (cmmtRes.isErr()) return fallback(cmmtRes.unwrapErr());

                    m_comment = std::move(cmmtRes).unwrap();

                    m_likeLabel->setText(numToString(m_comment.likes));
                    m_dislikeLabel->setText(numToString(m_comment.dislikes));

                    m_likeBtn->setEnabled(true);
                    m_dislikeBtn->setEnabled(true);
                });
        });
    btn->setID(like ? "like-btn" : "dislike-btn");
    btn->setScale(0.625f);

    to->addChild(btn);

    to->updateLayout();
};

bool CommentItem::init(Comment cmmt, float width, bool geodeTheme) {
    m_comment = std::move(cmmt);

    if (!CCNode::init()) return false;

    setAnchorPoint({0.5, 0});
    setContentSize({width, 47.5f});

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
    m_contentLabel->setAlignment(Label::Alignment::Left);

    addChildAtPosition(m_contentLabel, Anchor::TopLeft, {27.5f, -25.f});

    auto actionMenuLayout = RowLayout::create()
                                ->setGap(2.5f)
                                ->setAutoScale(false)
                                ->setAxisReverse(true)
                                ->setAxisAlignment(AxisAlignment::End)
                                ->setAutoGrowAxis(0.f)
                                ->setGrowCrossAxis(true);

    auto actionMenu = CCNode::create();
    actionMenu->setID("action-container");
    actionMenu->setAnchorPoint({1, 1});
    actionMenu->setLayout(actionMenuLayout);

    addChildAtPosition(actionMenu, Anchor::TopRight, {-3.75f, -3.75f});

    Button* actionBtn = nullptr;

    if (isSelf()) {
        actionBtn = Button::createWithSpriteFrameName(
            "GJ_trashBtn_001.png",
            [this](auto) {
                m_callback(CommentAction::Delete, m_comment);
            });
        actionBtn->setID("delete-comment-btn");
    } else {
        actionBtn = Button::createWithNode(
            CircleButtonSprite::createWithSpriteFrameName(
                "geode.loader/exclamation-red.png",
                0.875f,
                geodeTheme ? CircleBaseColor::DarkPurple : CircleBaseColor::Green),
            [this](auto) {
                m_callback(CommentAction::Report, m_comment);
            });
        actionBtn->setID("report-comment-btn");
    };
    actionBtn->setScale(0.925f);

    cue::rescaleToMatch(actionBtn, 20.f);

    actionMenu->addChild(actionBtn);

    addVoteNodes(actionMenu, m_dislikeBtn, m_dislikeLabel, CommentVote::Dislike);
    addVoteNodes(actionMenu, m_likeBtn, m_likeLabel, CommentVote::Like);

    auto const timePosted = *asp::SystemTime::now().durationSince(m_comment.created);
    std::string timeTxt = (timePosted.seconds() < 3) ? "Just now" : fmt::format("{} ago", timePosted.toHumanString());

    auto time = Label::create(std::move(timeTxt), "chatFont.fnt");
    time->setID("date-created-label");
    time->setScale(0.5f);
    time->setAnchorPoint({1, 0});
    time->setAlignment(Label::Alignment::Right);
    time->setColor({200, 200, 200});
    time->setOpacity(200);

    addChildAtPosition(time, Anchor::BottomRight, {-5.f, 5.f});

    return true;
};

arc::Future<WebRes> CommentItem::sendVote(CommentVote vote) {
    CW_MODCOMMENTS_ARGON_UNWRAP(auto token);

    matjson::Value body;
    body["comment"] = m_comment.id;
    body["vote"] = static_cast<int8_t>(vote);

    auto req = (co_await request::withAuthCo(std::move(token)))
                   .bodyJSON(body);

    co_return webres::processResp(co_await req.put("/v1/comments/vote"_api));
};

bool CommentItem::isSelf() const noexcept {
    return GJAccountManager::sharedState()->m_accountID == m_comment.author.id;
};

void CommentItem::setActionCallback(Callback&& cb) {
    m_callback = std::move(cb);
};

Comment const& CommentItem::getComment() const noexcept {
    return m_comment;
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

asp::Instant CommentsPopup::s_lastComment;

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