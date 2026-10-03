#include "../CommentPopup.hpp"

#include <Util.h>

#include <Geode/Geode.hpp>

using namespace geode::prelude;
using namespace cw::mod_cmmts;

static constexpr auto g_commentWait = 60;

asp::Instant CommentsPopup::s_lastComment = asp::Instant();

bool CommentsPopup::init(std::string modID, bool geodeTheme) {
    m_modID = std::move(modID);

    if (!Popup::init(375.f, 250.f, geodeTheme ? "geode.loader/GE_square01.png" : "GJ_square01.png")) return false;

    setID(fmt::format("popup-{}", modID));
    setTitle("Mod Comments");

    setCloseButtonSpr(
        CircleButtonSprite::createWithSpriteFrameName(
            "geode.loader/close.png",
            0.875f,
            geodeTheme ? CircleBaseColor::DarkPurple : CircleBaseColor::Green),
        0.825f);

    return true;
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