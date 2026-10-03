#pragma once

#include <Geode/Geode.hpp>

#include <Geode/ui/GeodeUI.hpp>

namespace cw::mod_comments {
    namespace ui {
        class TabSprite final : public cocos2d::CCNode {  // thank u gode uwu
        private:
            geode::NineSlice* m_deselectedBG;
            geode::NineSlice* m_selectedBG;

            cocos2d::CCSprite* m_icon;
            geode::Label* m_label;

        protected:
            bool init(geode::ZStringView iconFrame, std::string text, float width, bool altColor);

        public:
            static TabSprite* create(geode::ZStringView iconFrame, std::string text, float width, bool altColor = false);

            void select(bool selected);
            void disable(bool disabled);
        };
    };
};