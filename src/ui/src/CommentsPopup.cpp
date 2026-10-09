#include "../CommentsPopup.hpp"

#include <Util.h>

#include <cue/PlayerIcon.hpp>

#include <Geode/Geode.hpp>

using namespace geode::prelude;
using namespace cw::mod_cmmts;

namespace cw::mod_cmmts {
    namespace impl {
        static constexpr uint8_t commentWait = 30;
        static constexpr uint8_t refreshWait = 2;

        static constexpr uint8_t maxCommentChars = 128;
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

asp::Instant CommentsPopup::s_lastComment;
asp::Instant CommentsPopup::s_lastRefresh;

StringMap<GeodeMod> CommentsPopup::s_indexedMods;

bool CommentsPopup::showInput() const {
    return argon::signedIn() && Loader::get()->isModInstalled(m_modID);
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

arc::Future<WebRes> CommentsPopup::getComments() {
    auto req = request::base()
                   .param("mod", m_modID)
                   .param("page", m_page);

    co_return webres::processResp(co_await req.get("/v1/comments/get"_api));
};

arc::Future<WebRes> CommentsPopup::sendComment(std::string content) {
    CW_MODCOMMENTS_ARGON_UNWRAP(auto token);

    matjson::Value body;
    body["mod"] = m_modID;
    body["content"] = std::move(content);
    body["icons"] = co_await players::getUserIconsCo();

    auto req = (co_await request::withAuthCo(std::move(token)))
                   .bodyJSON(body);

    co_return webres::processResp(co_await req.post("/v1/comments/send"_api));
};

void CommentsPopup::updatePageLabel() {
    auto count = m_commentList->m_contentLayer->getChildrenCount();
    m_pageLabel->setText(fmt::format("Page {}, {} {}", m_page, count, plurals::appendS("comment", count)));
};

void CommentsPopup::refreshComments(bool skipCooldown) {
    if (!skipCooldown) {
        auto elapsed = asp::Instant::now().durationSince(s_lastRefresh).seconds();
        if (elapsed < impl::refreshWait) return;
    };

    s_lastRefresh = asp::Instant::now();

    m_commentList->setVisible(false);
    m_commentMenu->setVisible(false);

    m_pageNextBtn->setEnabled(false);
    m_pagePrevBtn->setEnabled(false);

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

                std::vector<Comment> comments;
                comments.reserve(array.size());

                for (auto const& val : array) {
                    auto cmmtRes = val.as<Comment>();
                    if (cmmtRes.isErr()) {
                        log::error("Failed: {}", cmmtRes.unwrapErr());
                        continue;
                    };

                    comments.push_back(std::move(cmmtRes).unwrap());
                };

                if (comments.empty() && m_page > 1) {
                    m_page--;
                    m_maxPage = m_page;

                    return refreshComments(true);
                };

                m_maxPage = (array.size() == 15) ? m_page + 1 : m_page;

                for (auto& comment : comments) {
                    auto cell = CommentItem::create(
                        std::move(comment),
                        m_commentList->getScaledContentWidth(),
                        true,
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

                m_pageNextBtn->setVisible(m_page < m_maxPage);
                m_pagePrevBtn->setVisible(m_page > 1);

                m_pageNextBtn->setEnabled(true);
                m_pagePrevBtn->setEnabled(true);

                m_commentList->m_contentLayer->updateLayout();
                m_commentList->scrollToTop();

                m_commentList->setVisible(true);
                m_commentMenu->setVisible(showInput());
            } else {
                m_errLabel->setVisible(true);
            };

            updatePageLabel();

            m_refreshBtn->setVisible(true);

            m_loading->setVisible(false);
        });
};

bool CommentsPopup::init(std::string modID, bool geodeTheme) {
    m_modID = std::move(modID);
    m_geodeTheme = geodeTheme;

    if (!Popup::init(430.f, 270.f, m_geodeTheme ? "geode.loader/GE_square01.png" : "GJ_square01.png")) return false;

    setID(fmt::format("popup-{}", modID));

    setCloseButtonSpr(
        CircleButtonSprite::createWithSpriteFrameName(
            "geode.loader/close.png",
            0.875f,
            m_geodeTheme ? CircleBaseColor::DarkPurple : CircleBaseColor::Green),
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
        {m_mainLayer->getScaledContentWidth() - 75.f, m_mainLayer->getScaledContentHeight() - 40.f},
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

    auto isShowInput = showInput();

    m_commentList = ScrollLayer::create({cmmtBorder->getScaledContentWidth() - 12.5f, cmmtBorder->getScaledContentHeight() - (isShowInput ? 45.f : 25.f)});
    m_commentList->setID("comment-list");
    m_commentList->setAnchorPoint({0.5, 1});
    m_commentList->ignoreAnchorPointForPosition(false);

    m_commentList->m_contentLayer->setLayout(ScrollLayer::createDefaultListLayout(3.25f));

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

    static constexpr auto pageBtnSprName = "GJ_arrow_02_001.png";

    m_pageNextBtn = Button::createWithSpriteFrameName(
        pageBtnSprName,
        [this](Button* sender) {
            if (m_page < m_maxPage) m_page++;
            if (m_page > m_maxPage) m_page = m_maxPage;

            sender->setVisible(m_page < m_maxPage);

            refreshComments(true);
        });
    m_pageNextBtn->setID("next-page-btn");
    m_pageNextBtn->setScale(0.875f);
    m_pageNextBtn->setVisible(false);

    if (auto spr = typeinfo_cast<CCSprite*>(m_pageNextBtn->getDisplayNode())) spr->setFlipX(true);

    m_pagePrevBtn = Button::createWithSpriteFrameName(
        pageBtnSprName,
        [this](Button* sender) {
            if (m_page > 1) m_page--;

            sender->setVisible(m_page > 1);

            refreshComments(true);
        });
    m_pagePrevBtn->setID("previous-page-btn");
    m_pagePrevBtn->setScale(0.875f);
    m_pagePrevBtn->setVisible(false);

    m_mainLayer->addChildAtPosition(m_pageNextBtn, Anchor::Right, {-18.75f, -3.75f});
    m_mainLayer->addChildAtPosition(m_pagePrevBtn, Anchor::Left, {18.75f, -3.75f});

    m_pageLabel = Label::create("Loading...", "goldFont.fnt");
    m_pageLabel->setID("page-label");
    m_pageLabel->setScale(0.5f);
    m_pageLabel->setAnchorPoint({1, 1});
    m_pageLabel->setAlignment(Label::Alignment::Right);

    m_mainLayer->addChildAtPosition(m_pageLabel, Anchor::BottomRight, {-15.f, -1.25f});

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
    m_inputBox->setMaxCharCount(impl::maxCommentChars);
    m_inputBox->setTextAlign(TextInputAlign::Left);
    m_inputBox->setCommonFilter(CommonFilter::Any);

    createInputLimitLabel(impl::maxCommentChars);

    setMildLimitWarning(64);
    setModerateLimitWarning(96);

    m_commentMenu->addChild(m_inputBox);

    m_commentMenu->setVisible(isShowInput);

    if (!isShowInput) {
        auto install = Label::createRich(
            Loader::get()->isModInstalled(m_modID)
                ? (argon::signedIn()
                          ? "<cr>Unknown error</c>"  // idk let me have fun with my ternaries >:C
                          : "<cy>Log in</c> to <cg>post comments</c>!")
                : "<cy>Install this mod</c> if you'd like to <cg>post comments</c> here!",
            "chatFont.fnt");
        install->setScale(0.675f);
        install->setAnchorPoint({0.5, 0});
        install->setAlignment(Label::Alignment::Center);

        cmmtBorder->addChildAtPosition(install, Anchor::Bottom, {0.f, 4.25f});
    };

    auto sendBtnSpr = EditorButtonSprite::createWithSpriteFrameName(
        m_geodeTheme ? "send_icon_geode.png"_spr : "send_icon_gd.png"_spr,
        0.875f,
        m_geodeTheme ? EditorBaseColor::DarkGray : EditorBaseColor::Green);

    auto sendBtn = Button::createWithNode(
        sendBtnSpr,
        [this, isShowInput](Button* sender) {
            if (isShowInput) onSend(sender);
        });
    sendBtn->setID("send-comment-btn");
    sendBtn->setEnabled(isShowInput);

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
            m_geodeTheme ? CircleBaseColor::DarkPurple : CircleBaseColor::Green),
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
    if (!showInput()) return;

    m_inputBox->defocus();

    auto elapsed = asp::Instant::now().durationSince(s_lastComment).seconds();
    if (elapsed < impl::commentWait) {
        auto remaining = impl::commentWait - elapsed;

        createQuickPopup(
            "Slow Down!",
            fmt::format("You must <co>wait {} {} before sending your next comment</c>!", remaining, plurals::appendS("second", remaining)),
            "OK",
            nullptr,
            nullptr);

        return;
    };

    auto inputStr = str::trim(m_inputBox->getString());

    if (inputStr.size() > impl::maxCommentChars) return Notification::create(fmt::format("Comment exceeds {} characters", impl::maxCommentChars), NotificationIcon::Warning)->show();
    if (inputStr.empty()) return Notification::create("Comment cannot be empty.", NotificationIcon::Error)->show();

    sender->setEnabled(false);
    if (auto spr = typeinfo_cast<CCSprite*>(sender->getDisplayNode())) spr->setColor({65, 65, 65});

    m_refreshBtn->setVisible(false);

    m_commentTask.spawn(
        sendComment(std::move(inputStr)),
        [this, sender](WebRes res) {
            if (res.isOk()) {
                s_lastComment = asp::Instant::now();

                m_page = 1;
                m_inputBox->setString("", true);
                refreshComments();
            } else if (res.isErr()) {
                m_refreshBtn->setVisible(true);
                Notification::create(fmt::format("Error {}", res.getCode()), NotificationIcon::Error)->show();
            };

            if (auto spr = typeinfo_cast<CCSprite*>(sender->getDisplayNode())) spr->setColor({255, 255, 255});
            sender->setEnabled(true);
        });
};

void CommentsPopup::onDelete(Comment const& cmmt) {
    if (m_commentActionTask.isPending()) return;

    createQuickPopup(
        "Delete Comment",
        "<cr>Delete</c> this comment?",
        "Cancel",
        "Yes",
        [this, &cmmt](auto, bool ok) {
            if (ok) m_commentActionTask.spawn(
                deleteComment(cmmt.id),
                [this](WebRes res) {
                    if (res.isOk()) return refreshComments(true);
                    Notification::create(fmt::format("Failed to delete comment ({})", res.getCode()), NotificationIcon::Error)->show();
                });
        });
};

void CommentsPopup::onReport(Comment const& cmmt) {
    if (m_commentActionTask.isPending()) return;

    if (SelfDirector::get()->isReported(cmmt.id)) return Notification::create("You already reported this comment", NotificationIcon::Warning)->show();

    CommentReportPopup::create(
        cmmt,
        [this](Comment const& cmmt, std::string reason) {
            if (auto popup = CCScene::get()->getChildByType<CommentReportPopup>()) cue::resetNode(popup);
            Notification::create("Reporting comment...", NotificationIcon::Loading)->show();

            m_commentActionTask.spawn(
                reportComment(cmmt.id, std::move(reason)),
                [&cmmt](WebRes res) {
                    if (res.isErr()) return Notification::create(fmt::format("Failed to report comment ({})", res.getCode()), NotificationIcon::Error)->show();

                    SelfDirector::get()->setReport(cmmt.id);
                    Notification::create("Reported successfully", NotificationIcon::Success)->show();
                });
        },
        m_geodeTheme)
        ->show();
};

void CommentsPopup::onExit() {
    if (auto popup = CCScene::get()->getChildByType<RulesPopup>()) popup->removeFromParent();
    Popup::onExit();
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