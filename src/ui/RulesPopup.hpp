#pragma once

#include <Geode/Geode.hpp>

namespace cw::mod_cmmts {
    namespace ui {
        class RulesPopup final : public geode::Popup {
            using Callback = geode::Function<void(bool)>;

        private:
            Callback m_callback = nullptr;
            bool m_agreed = false;

        protected:
            void keyBackClicked() override;

            bool init(Callback&& cb, bool geodeTheme);

        public:
            static RulesPopup* create(Callback&& cb, bool geodeTheme = false);
        };
    };
};