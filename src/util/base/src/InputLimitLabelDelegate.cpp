#include "../InputLimitLabelDelegate.hpp"

#include <Util.h>

#include <Geode/Geode.hpp>

using namespace geode::prelude;
using namespace cw::mod_cmmts;

using namespace cw::mod_cmmts::base;

std::string InputLimitLabelDelegate::getCharCountText(std::string_view input) const {
    return fmt::format("{} / {}", input.size(), m_inputMaxChars);
};

Label* InputLimitLabelDelegate::createInputLimitLabel(uint16_t limit) {
    m_inputMaxChars = limit;

    m_inputBoxCharLabel = Label::create(getCharCountText(m_inputBox->getString()), "chatFont.fnt");
    m_inputBoxCharLabel->setScale(0.5f);
    m_inputBoxCharLabel->setOpacity(125);
    m_inputBoxCharLabel->setAnchorPoint({1, 0});
    m_inputBoxCharLabel->setAlignment(Label::Alignment::Right);

    m_inputBox->addChildAtPosition(m_inputBoxCharLabel, Anchor::BottomRight, {-2.f, 2.f}, false);

    m_inputBox->setCallback([this](std::string const& input) {
        m_inputBoxCharLabel->setText(getCharCountText(input));

        auto size = input.size();
        if (size > m_inputMaxChars) {
            m_inputBoxCharLabel->setColor({225, 75, 75});
        } else if (size > m_inputModerateWarn) {
            m_inputBoxCharLabel->setColor({225, 150, 75});
        } else if (size > m_inputMildWarn) {
            m_inputBoxCharLabel->setColor({225, 200, 100});
        } else {
            m_inputBoxCharLabel->setColor({225, 225, 225});
        };
    });

    return m_inputBoxCharLabel;
};

void InputLimitLabelDelegate::setModerateLimitWarning(uint16_t threshold) {
    m_inputModerateWarn = threshold;
};

void InputLimitLabelDelegate::setMildLimitWarning(uint16_t threshold) {
    m_inputMildWarn = threshold;
};