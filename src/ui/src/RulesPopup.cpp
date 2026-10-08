#include "../RulesPopup.hpp"

#include <Util.h>

#include <cue/PlayerIcon.hpp>

#include <Geode/Geode.hpp>

using namespace geode::prelude;
using namespace cw::mod_cmmts;

bool RulesPopup::init(Callback&& cb, bool geodeTheme) {
    m_callback = std::move(cb);

    if (!Popup::init(400.f, 250.f, geodeTheme ? "geode.loader/GE_square01.png" : "GJ_square01.png")) return false;

    setID("rules"_spr);
    setTitle("Mod Comments Guidelines");

    setCloseButtonSpr(
        CircleButtonSprite::createWithSpriteFrameName(
            "geode.loader/close.png",
            0.875f,
            geodeTheme ? CircleBaseColor::DarkPurple : CircleBaseColor::Green),
        0.825f);

    m_closeBtn->setVisible(false);

    addSideArt(m_mainLayer, SideArt::All, SideArtStyle::PopupGold);

    auto label = Label::createRich("Before using <cg>Mod Comments</c>, you must agree to the <cr>rules</c>.", "chatFont.fnt");
    label->setScale(0.675f);
    label->setAnchorPoint({0.5, 1});
    label->setAlignment(Label::Alignment::Center);
    label->setMaxWidth((m_mainLayer->getScaledContentWidth() - 37.5f) * 1.425f);

    m_mainLayer->addChildAtPosition(label, Anchor::Top, {0.f, -32.5f});

    auto rulesText = MDTextArea::create(rules::fullText, {m_mainLayer->getScaledContentWidth() - 45.f, m_mainLayer->getScaledContentHeight() - 87.5f});
    rulesText->setID("rules-text-area");
    rulesText->setZOrder(9);

    m_mainLayer->addChildAtPosition(rulesText, Anchor::Center, {0.f, -3.75f});

    auto sendBtn = Button::createWithNode(
        ButtonSprite::create(
            "Proceed",
            "goldFont.fnt",
            geodeTheme ? "geode.loader/GE_button_05.png" : "GJ_button_01.png",
            0.925f),
        [this](auto) {
            createQuickPopup(
                "Confirm",
                "<cg>Agree</c> to the <cr>rules</c>?",
                "No",
                "Yes",
                [this](auto, bool ok) {
                    return m_callback(this, ok);
                });
        });
    sendBtn->setID("proceed-btn");
    sendBtn->setScale(0.925f);

    m_mainLayer->addChildAtPosition(sendBtn, Anchor::Bottom, {0.f, sendBtn->getScaledContentHeight() * 0.75f});

    auto infoBtn = Button::createWithSpriteFrameName(
        "GJ_infoIcon_001.png",
        [](auto) {
            createQuickPopup(
                "Help",
                "This is the <cg>Mod Comments rules pop-up</c>. To create comments, you must <cy>agree to the rules</c>.",
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

RulesPopup* RulesPopup::create(Callback&& cb, bool geodeTheme) {
    auto ret = new RulesPopup();
    if (ret->init(std::move(cb), geodeTheme)) {
        ret->autorelease();
        return ret;
    };

    delete ret;
    return nullptr;
};