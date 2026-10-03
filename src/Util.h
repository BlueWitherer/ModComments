#pragma once

#include <Geode/Geode.hpp>

#include <ui/Include.h>

#include <util/Macros.h>
#include <util/Include.h>

namespace cw::mod_cmmts {
    namespace request {
        inline auto base() {
            auto loader = geode::Loader::get();

            return geode::utils::web::WebRequest()
                .userAgent(fmt::format("ModComments/{} ({}, Geode {}, GD {})",
                    geode::Mod::get()->getVersion().toVString(false),
                    geode::utils::platform::getString(),
                    loader->getVersion(),
                    loader->getGameVersion()))
                .timeout(std::chrono::seconds(15));
        };

        inline auto withAuth(int accountId, std::string token) {
            return geode::utils::web::WebRequest()
                .param("account_id", accountId)
                .param("authtoken", std::move(token));
        };
    };

    using namespace ui;
};