#include "../CommentUI.hpp"

#include <Util.h>

#include <cue/PlayerIcon.hpp>

#include <Geode/Geode.hpp>

#include <Geode/utils/ColorProvider.hpp>

using namespace geode::prelude;
using namespace cw::mod_cmmts;

namespace cw::mod_cmmts {
    namespace impl {
        static bool isStaff() {
            auto userRes = SelfDirector::get()->getCurrentUser();
            if (userRes.isErr()) return false;

            return userRes.unwrap().staff;
        };

    };
};

bool CommentReportPopup::init(Comment const& cmmt, bool geodeTheme) {
    if (!Popup::init({300.f, 185.f}, geodeTheme ? "geode.loader/GE_square01.png" : "GJ_square01.png")) return false;

    setID("report-popup"_spr);
    setTitle(fmt::format("Report {}", cmmt.author.username));

    setCloseButtonSpr(
        CircleButtonSprite::createWithSpriteFrameName(
            "geode.loader/close.png",
            0.875f,
            geodeTheme ? CircleBaseColor::DarkPurple : CircleBaseColor::Green),
        0.825f);

    auto cmmtNode = Ref(CommentItem::create(cmmt, m_mainLayer->getScaledContentWidth() * 0.925f, false, geodeTheme));
    cmmtNode->setAnchorPoint({0.5, 1});

    m_mainLayer->addChildAtPosition(cmmtNode, Anchor::Top, {0.f, -40.f});

    auto wip = Label::create("work in progress!!!", "chatFont.fnt");
    wip->setScale(0.625f);
    wip->setAlignment(Label::Alignment::Center);

    m_mainLayer->addChildAtPosition(wip, Anchor::Bottom, {0.f, 37.5f});

    return true;
};

CommentReportPopup* CommentReportPopup::create(Comment const& cmmt, bool geodeTheme) {
    auto ret = new CommentReportPopup();
    if (ret->init(cmmt, geodeTheme)) {
        ret->autorelease();
        return ret;
    };

    delete ret;
    return nullptr;
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

    to->updateLayout();
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

        Button* actionBtn = nullptr;

        if (isSelf() || impl::isStaff()) {
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

    return true;
};

void CommentItem::voteCallback(CommentVote type) {
    auto like = type == CommentVote::Like;

    auto prevVote = m_comment.myVote;

    auto prevLikes = m_comment.likes;
    auto prevDislikes = m_comment.dislikes;

    if (prevVote == (like ? 1 : -1)) type = CommentVote::None;

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

CommentItem* CommentItem::create(Comment cmmt, float width, bool buttons, bool geodeTheme) {
    auto ret = new CommentItem();
    if (ret->init(std::move(cmmt), width, buttons, geodeTheme)) {
        ret->autorelease();
        return ret;
    };

    delete ret;
    return nullptr;
};