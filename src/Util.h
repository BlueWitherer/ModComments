#pragma once

#include <cue/Util.hpp>

#include <argon/argon.hpp>

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

        inline auto withAuth(std::string token) {
            auto acc = argon::getGameAccountData();

            return base()
                .param("account_id", acc.accountId)
                .param("authtoken", std::move(token))
                .param("user_id", acc.userId)
                .param("username", std::move(acc.username));
        };

        inline arc::Future<geode::utils::web::WebRequest> withAuthCo(std::string token) {
            co_return *co_await geode::async::waitForMainThread<geode::utils::web::WebRequest>([t = std::move(token)]() {
                return withAuth(std::move(t));
            });
        };
    };

    namespace popups {
        void showRules();
    };

    using namespace ui;
};