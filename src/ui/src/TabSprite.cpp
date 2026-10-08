#include "../TabSprite.hpp"

#include <Util.h>

#include <Geode/Geode.hpp>

#include <Geode/utils/ColorProvider.hpp>

using namespace geode::prelude;
using namespace cw::mod_cmmts;

bool TabSprite::init(ZStringView iconFrame, std::string text, float width, bool altColor) {
    if (!CCNode::init()) return false;

    CCSize const itemSize = {width, 35.f};
    CCSize const iconSize = {18.f, 18.f};

    setContentSize(itemSize);
    setAnchorPoint({0.5, 0.5});

    auto colors = ColorProvider::get();

    m_deselectedBG = NineSlice::createWithSpriteFrameName("geode.loader/tab-bg.png");
    m_deselectedBG->setScale(0.8f);
    m_deselectedBG->setContentSize(itemSize / 0.8f);
    m_deselectedBG->setColor(colors->color3b("geode.loader/mod-list-tab-deselected-bg"));

    addChildAtPosition(m_deselectedBG, Anchor::Center);

    m_selectedBG = NineSlice::createWithSpriteFrameName("geode.loader/tab-bg.png");
    m_selectedBG->setScale(0.8f);
    m_selectedBG->setContentSize(itemSize / 0.8f);
    m_selectedBG->setColor(
        colors->color3b(
            altColor
                ? "geode.loader/mod-list-tab-selected-bg-alt"
                : "geode.loader/mod-list-tab-selected-bg"));

    addChildAtPosition(m_selectedBG, Anchor::Center);

    m_icon = CCSprite::createWithSpriteFrameName(iconFrame.c_str());
    limitNodeSize(m_icon, iconSize, 3.f, 0.1f);

    addChildAtPosition(m_icon, Anchor::Left, {16.f, 0.f}, false);

    m_label = Label::create(std::move(text), "bigFont.fnt");
    m_label->setLimitLabelWidth(getScaledContentWidth() - 45.f);

    addChildAtPosition(m_label, Anchor::Left, {(itemSize.width - iconSize.width) / 2.f + iconSize.width, 0.f}, false);

    return true;
};

TabSprite* TabSprite::create(ZStringView iconFrame, std::string text, float width, bool altColor) {
    auto ret = new TabSprite();
    if (ret->init(iconFrame, std::move(text), width, altColor)) {
        ret->autorelease();
        return ret;
    };

    delete ret;
    return nullptr;
};

void TabSprite::select(bool selected) {
    m_deselectedBG->setVisible(!selected);
    m_selectedBG->setVisible(selected);
};

void TabSprite::disable(bool disabled) {
    auto color = disabled ? ccc3(95, 95, 95) : ccc3(255, 255, 255);

    m_deselectedBG->setColor(color);
    m_selectedBG->setColor(color);

    m_icon->setColor(color);
    m_label->setColor(color);
};