#pragma once

#include <Geode/UI.hpp>

namespace cw::mod_cmmts {
    namespace base {
        class InputLimiter {  // idk :P
        private:
            geode::Label* m_inputBoxCharLabel = nullptr;

            uint8_t m_inputMaxChars = UINT8_MAX;

            uint8_t m_inputModerateWarn = 225;
            uint8_t m_inputMildWarn = 180;

            std::string getCharCountText(std::string_view input) const;

        protected:
            geode::TextInput* m_inputBox;

            geode::Label* createInputLimitLabel(uint8_t limit = UINT8_MAX);

            void setModerateLimitWarning(uint8_t threshold);
            void setMildLimitWarning(uint8_t threshold);
        };
    };
};