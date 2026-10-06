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

bool CommentItem::init(Comment cmmt, float width, bool geodeTheme) {
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

CommentItem* CommentItem::create(Comment cmmt, float width, bool geodeTheme) {
    auto ret = new CommentItem();
    if (ret->init(std::move(cmmt), width, geodeTheme)) {
        ret->autorelease();
        return ret;
    };

    delete ret;
    return nullptr;
};

std::string CommentModNode::getModName() const {
    return m_dataOk ? m_data.versions[0].name : Loader::get()->getInstalledMod(m_id)->getName().c_str();
};

std::vector<std::string> CommentModNode::getModDevs() const {
    std::vector<std::string> out;

    if (m_dataOk) {
        out.reserve(m_data.developers.size());
        for (auto const& dev : m_data.developers) out.push_back(dev.displayName);
    } else {
        out = Loader::get()->getInstalledMod(m_id)->getDevelopers();
    };

    return out;
};

bool CommentModNode::init(std::string id, std::optional<GeodeMod> mod) {
    m_id = std::move(id);

    if (mod.has_value()) {
        m_dataOk = true;
        m_data = std::move(mod).value();
    };

    if (!CCNode::init()) return false;

    auto layout = RowLayout::create()
                      ->setGap(5.f)
                      ->setAutoScale(false)
                      ->setAutoGrowAxis(0.f)
                      ->setGrowCrossAxis(true);

    setAnchorPoint({0.5, 1});
    setLayout(layout);

    if (m_dataOk) {
        auto icon = LazySprite::create({18.75f, 18.75f});
        icon->setID("logo");
        icon->setAutoResize(true);
        icon->setAnchorPoint({0.5, 0.5});

        icon->setLoadCallback([this, icon](Result<> res) {
            if (res.isOk()) cue::rescaleToMatch(icon, 18.75f);
            updateLayout();
        });

        addChild(icon);

        icon->loadFromUrl(fmt::format("https://api.geode-sdk.org/v1/mods/{}/logo", m_id));
    } else {
        auto icon = createModLogo(Loader::get()->getInstalledMod(m_id));
        icon->setID("logo");

        cue::rescaleToMatch(icon, 18.75f);

        addChild(icon);
    };

    auto name = Label::create(getModName(), "bigFont.fnt");
    name->setID("name");
    name->setAnchorPoint({0, 0.5});
    name->setLimitLabelWidth(165.f, 0.525f);

    addChild(name);

    std::string devsText;
    auto const devList = getModDevs();

    if (devList.size() > 2) {
        devsText = fmt::format("{} + {} more", devList[0], devList.size() - 1);
    } else if (devList.size() > 1) {
        devsText = fmt::format("{} & {}", devList[0], devList[1]);
    } else {
        devsText = devList[0];
    };

    auto devs = Label::create(std::move(devsText), "goldFont.fnt");
    devs->setID("developers");
    devs->setAnchorPoint({0, 0.5});
    devs->setLimitLabelWidth(100.f, 0.425f);

    addChild(devs);

    updateLayout();

    return true;
};

CommentModNode* CommentModNode::create(std::string id, std::optional<GeodeMod> mod) {
    auto ret = new CommentModNode();
    if (ret->init(std::move(id), std::move(mod))) {
        ret->autorelease();
        return ret;
    };

    delete ret;
    return nullptr;
};