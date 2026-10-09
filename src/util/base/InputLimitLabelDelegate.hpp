#pragma once

#include <Geode/UI.hpp>

namespace cw::mod_cmmts {
    namespace base {
        class InputLimitLabelDelegate {
        private:
            geode::Label* m_inputBoxCharLabel = nullptr;

            uint16_t m_inputMaxChars = 512;

            uint16_t m_inputModerateWarn = 380;
            uint16_t m_inputMildWarn = 256;

            std::string getCharCountText(std::string_view input) const;

        protected:
            geode::TextInput* m_inputBox;

            geode::Label* createInputLimitLabel(uint16_t limit);

            void setModerateLimitWarning(uint16_t threshold);
            void setMildLimitWarning(uint16_t threshold);
        };
    };
};