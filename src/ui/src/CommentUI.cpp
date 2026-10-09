#include "../CommentUI.hpp"

#include <Util.h>

#include <cue/PlayerIcon.hpp>

#include <Geode/Geode.hpp>

#include <Geode/utils/ColorProvider.hpp>

using namespace geode::prelude;
using namespace cw::mod_cmmts;

namespace cw::mod_cmmts {
    struct CommentUserStatusData final {
        std::string name;
        std::string description;
        std::string badgeSprite;
    };

    namespace impl {
        static constexpr uint8_t maxReportChars = 48;

        static bool isStaff() {
            auto userRes = SelfDirector::get()->getCurrentUser();
            if (userRes.isErr()) return false;

            auto staff = userRes.unwrap().staff;
            log::trace("Current user {} staff", staff ? "is" : "is NOT");
            return staff;
        };

        static CommentUserStatusData const& getDataForStatus(CommentUserStatus status) noexcept {
            static auto const owner = CommentUserStatusData{
                "Mod Comments Owner",
                "is the <cg>Mod Comments owner</c>. They own and actively develop this mod.",
                "badge_owner.png"_spr,
            };

            static auto const staff = CommentUserStatusData{
                "Mod Comments Staff",
                "is a <cg>Comment moderator</c>. They oversee comment sections and review user reports to keep conversations safe for everyone.",
                "badge_staff.png"_spr,
            };

            switch (status) {
                default: [[fallthrough]];

                case CommentUserStatus::Staff: return staff;
                case CommentUserStatus::Owner: return owner;
            };
        };
    };
};

bool CommentReportPopup::init(Comment const& cmmt, Callback&& cb, bool geodeTheme) {
    m_callback = std::move(cb);

    if (!Popup::init(300.f, 185.f, geodeTheme ? "geode.loader/GE_square01.png" : "GJ_square01.png")) return false;

    setID("report-popup"_spr);
    setTitle(fmt::format("Report {}", cmmt.author.username));

    setCloseButtonSpr(
        CircleButtonSprite::createWithSpriteFrameName(
            "geode.loader/close.png",
            0.875f,
            geodeTheme ? CircleBaseColor::DarkPurple : CircleBaseColor::Green),
        0.825f);

    auto cmmtNode = CommentItem::create(cmmt, m_mainLayer->getScaledContentWidth() * 0.925f, false, geodeTheme);
    m_mainLayer->addChildAtPosition(cmmtNode, Anchor::Center, {0.f, -12.5f});

    auto label = Label::createRich("If you believe this user's comment <cr>breaks our rules</c>, <cy>describe why using the text box below</c>. Thank you!", "chatFont.fnt");
    label->setScale(0.675f);
    label->setAnchorPoint({0.5, 1});
    label->setAlignment(Label::Alignment::Center);
    label->setMaxWidth((m_mainLayer->getScaledContentWidth() - 37.5f) * 1.425f);

    m_mainLayer->addChildAtPosition(label, Anchor::Top, {0.f, -32.5f});

    m_inputBox = TextInput::create(m_mainLayer->getScaledContentWidth() - 25.f, "Tell us about this commment...", "chatFont.fnt");
    m_inputBox->setID("description-input");
    m_inputBox->setMaxCharCount(impl::maxReportChars);
    m_inputBox->setCommonFilter(CommonFilter::Alphanumeric);
    m_inputBox->setContentHeight(m_inputBox->getScaledContentHeight() * 1.5f);

    createInputLimitLabel(impl::maxReportChars);

    setMildLimitWarning(34);
    setModerateLimitWarning(42);

    m_mainLayer->addChildAtPosition(m_inputBox, Anchor::Center, {0.f, -27.5f});

    auto warning = Label::createRich("<co>False reports</c> will most likely result in <cr>action taken on your own account</c>, please be mindful of the reports you send!", "chatFont.fnt");
    warning->setScale(0.5f);
    warning->setAnchorPoint({0.5, 1});
    warning->setAlignment(Label::Alignment::Center);
    warning->setMaxWidth((m_mainLayer->getScaledContentWidth() - 32.5f) * 1.75f);

    m_mainLayer->addChildAtPosition(warning, Anchor::Center, {0.f, -57.5f});

    auto sendBtn = Button::createWithNode(
        ButtonSprite::create(
            "Submit",
            "goldFont.fnt",
            geodeTheme ? "geode.loader/GE_button_05.png" : "GJ_button_01.png",
            0.875f),
        [this, &cmmt](Button* sender) {
            auto inputStr = str::trim(m_inputBox->getString());

            if (inputStr.size() > impl::maxReportChars) return Notification::create(fmt::format("Comment exceeds {} characters", impl::maxReportChars), NotificationIcon::Warning)->show();
            if (inputStr.empty()) return Notification::create("Comment cannot be empty.", NotificationIcon::Error)->show();

            m_callback(cmmt, std::move(inputStr));
        });
    sendBtn->setID("submit-idea-btn");
    sendBtn->setScale(0.75f);

    m_mainLayer->addChildAtPosition(sendBtn, Anchor::Bottom);

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

    return true;
};

CommentReportPopup* CommentReportPopup::create(Comment const& cmmt, Callback&& cb, bool geodeTheme) {
    auto ret = new CommentReportPopup();
    if (ret->init(cmmt, std::move(cb), geodeTheme)) {
        ret->autorelease();
        return ret;
    };

    delete ret;
    return nullptr;
};

void CommentItem::addBadge(CommentUserStatus type) {
    auto const& info = impl::getDataForStatus(type);

    auto btn = Button::createWithSpriteFrameName(
        info.badgeSprite,
        [this, &info](auto) {
            createQuickPopup(
                info.name.c_str(),
                fmt::format("<cy>{}</c> {}", m_comment.author.username, info.description),
                "OK",
                nullptr,
                nullptr);
        });
    btn->setID("badge-info-btn");

    cue::rescaleToMatch(btn, 12.5f);

    m_userMenu->addChild(btn);
    m_userMenu->updateLayout();
};

void CommentItem::addVoteNodes(CCNode* to, Button*& btn, Ref<Label>& label, CommentVote type) {
    auto like = type == CommentVote::Like;

    label = Label::create(fmt::format("{}", like ? m_comment.likes : m_comment.dislikes), "bigFont.fnt");
    label->setScale(0.375f);

    to->addChild(label);

    btn = Button::createWithSpriteFrameName(
        like ? "GJ_likesIcon_001.png" : "GJ_dislikesIcon_001.png",
        [this, type](auto) {
            log::debug("current vote is {}", m_comment.myVote);
            voteCallback((m_comment.myVote == static_cast<int8_t>(type)) ? CommentVote::None : type);
        });
    btn->setID(like ? "like-btn" : "dislike-btn");
    btn->setScale(0.625f);

    to->addChild(btn);

    btn->setEnabled(argon::signedIn());

    to->updateLayout();
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

bool CommentItem::init(Comment cmmt, float width, bool buttons, bool geodeTheme) {
    m_comment = std::move(cmmt);

    if (!CCNode::init()) return false;

    setAnchorPoint({0.5, 0});
    setContentSize({width, 47.5f});

    auto bg = cue::createBackground(
        getScaledContentSize(),
        {
            .opacity = static_cast<uint8_t>(isSelf() ? 125 : 100),
            .texture = "geode.loader/white-square.png",
            .zOrder = -1,
            .id = "",
        });
    bg->setColor(ColorProvider::get()->color3b(
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

    m_userMenu = CCNode::create();
    m_userMenu->setID("username-container");
    m_userMenu->setAnchorPoint({0, 1});
    m_userMenu->setLayout(userMenuLayout);

    addChildAtPosition(m_userMenu, Anchor::TopLeft, {5.f, -5.f});

    auto& user = m_comment.author;

    auto icon = cue::PlayerIcon::create(user.iconType, user.icon, user.color1, user.color2, user.useGlow ? user.colorGlow : -1);
    icon->setID("player-icon");

    cue::rescaleToMatch(icon, 15.f);

    m_userMenu->addChild(icon);

    auto username = Button::createWithLabel(
        user.username,
        "goldFont.fnt",
        [id = user.id, self = isSelf()](auto) {
            ProfilePage::create(id, self)->show();
        });
    username->setID("view-player-btn");
    username->setScale(0.625f);

    m_userMenu->addChild(username);

    m_userMenu->updateLayout();

    m_contentLabel = Label::create(m_comment.content, "geode.loader/mdFont.fnt");
    m_contentLabel->setID("comment-content-label");
    m_contentLabel->setScale(0.5f);
    m_contentLabel->setAnchorPoint({0, 1});
    m_contentLabel->setBreakWords(true);

    buttons
        ? m_contentLabel->setMaxWidth(getScaledContentWidth() * 1.375f)  // <- cuts off way too early for some reason
        : m_contentLabel->setLimitLabelWidth(getScaledContentWidth() - 32.5f, 0.5f);

    m_contentLabel->setAlignment(Label::Alignment::Left);

    addChildAtPosition(m_contentLabel, Anchor::TopLeft, {27.5f, -25.f});

    setContentHeight(getScaledContentHeight() + m_contentLabel->getScaledContentHeight() - 12.5f);
    bg->setContentSize(getScaledContentSize());

    updateLayout();

    if (buttons) {
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

        if (argon::signedIn()) {
            Button* actionBtn = nullptr;

            if (impl::isStaff() || isSelf()) {
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
        };

        addVoteNodes(actionMenu, m_dislikeBtn, m_dislikeLabel, CommentVote::Dislike);
        addVoteNodes(actionMenu, m_likeBtn, m_likeLabel, CommentVote::Like);
    };

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

    if (m_comment.author.id == CW_MODCOMMENTS_OWNER) {
        addBadge(CommentUserStatus::Owner);
    } else if (m_comment.author.staff) {
        addBadge(CommentUserStatus::Staff);
    };

    return true;
};

void CommentItem::voteCallback(CommentVote type) {
    auto like = (type == CommentVote::Like);

    auto prevVote = m_comment.myVote;

    if (prevVote == (like ? 1 : -1)) type = CommentVote::None;

    auto prevLikes = m_comment.likes;
    auto prevDislikes = m_comment.dislikes;

    switch (type) {
        case CommentVote::Like: {
            if (prevVote == 1) {
                if (prevLikes > 0) prevLikes--;
            } else {
                if (prevVote == -1 && prevDislikes > 0) prevDislikes--;
                prevLikes++;
            };
        } break;

        case CommentVote::Dislike: {
            if (prevVote == -1) {
                if (prevDislikes > 0) prevDislikes--;
            } else {
                if (prevVote == 1 && prevLikes > 0) prevLikes--;
                prevDislikes++;
            };
        } break;

        case CommentVote::None: {
            if (prevVote == 1 && prevLikes > 0) {
                prevLikes--;
            } else if (prevVote == -1 && prevDislikes > 0) {
                prevDislikes--;
            };
        } break;
    };

    m_likeLabel->setText(numToAbbreviatedString(prevLikes));
    m_dislikeLabel->setText(numToAbbreviatedString(prevDislikes));

    m_likeBtn->setEnabled(false);
    m_dislikeBtn->setEnabled(false);

    m_voteTask.spawn(
        sendVote(type),
        [this, prevLikes, prevDislikes, prevVote, like](WebRes res) {
            auto const completed = [this]() {
                m_likeLabel->setText(numToAbbreviatedString(m_comment.likes));
                m_dislikeLabel->setText(numToAbbreviatedString(m_comment.dislikes));

                m_likeBtn->setEnabled(true);
                m_dislikeBtn->setEnabled(true);
            };

            auto const fallback = [&completed, res](std::string_view err) {
                log::error("{}: {}", res.getCode(), err);
                completed();
            };

            if (res.isErr()) return fallback(res.getError());

            auto cmmtRes = res.getPayload<Comment>();
            if (cmmtRes.isErr()) return fallback(cmmtRes.unwrapErr());

            auto cmmt = std::move(cmmtRes).unwrap();
            log::debug("setting current voted status to {}({}/{})", cmmt.myVote, cmmt.likes, cmmt.dislikes);

            m_comment = std::move(cmmt);

            completed();
        });
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

CommentItem* CommentItem::create(Comment cmmt, float width, bool buttons, bool geodeTheme) {
    auto ret = new CommentItem();
    if (ret->init(std::move(cmmt), width, buttons, geodeTheme)) {
        ret->autorelease();
        return ret;
    };

    delete ret;
    return nullptr;
};