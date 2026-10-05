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
            auto const acc = argon::getGameAccountData();

            return base()
                .param("account_id", acc.accountId)
                .param("authtoken", std::move(token))
                .param("user_id", acc.userId)
                .param("username", acc.username);
        };

        inline arc::Future<geode::utils::web::WebRequest> withAuthCo(std::string token) {
            co_return *co_await geode::async::waitForMainThread<geode::utils::web::WebRequest>([t = std::move(token)]() {
                return withAuth(std::move(t));
            });
        };
    };

    namespace popups {
        static constexpr auto g_rulesText =
            "**Upon posting comments, you agree to the following <cr>rules</c>.**\n\n"
            "---\n\n"
            "![🛑](frame:geode.loader/info-alert.png?scale=0.375) <cr>*[Mod Comments](mod:cheeseworks.modcomments) was made for users to leave feedback on mods through an accessible in-game UI. **Mod developers are NOT responsible for providing support through comments**, please instead __contact them through their official channels__.*</c>\n\n"
            "---\n\n"
            "1. **Be civil.** - Engage in conversations that are <cg>respectful, fun, and constructive</c>. Any <co>comments made with the intention to hurt another individual or community</c> are **strictly prohibited**.\n\n"
            "2. **Don't spam.** - Avoid <co>going too off-topic in the comments</c>. Attempting to <co>overload our servers</c> or <co>attempting to bypass spam protections</c> will result in **rate-limiting & IP bans**.\n\n"
            "3. **No NSFW.** - <co>Inappropriate discussions</c> are **not allowed**. <cg>Respect boundares</c>, and keep comment sections <cg>safe and welcoming for everyone</c>!\n\n"
            "4. **Staff decisions**. - Moderators are instructed to take <cg>whatever means necessary to keep chats safe</c>. If you believe action has been <co>wrongfully taken against you</c>, you can communicate your concerns in [Cheeseworks's Discord server](https://www.dsc.gg/cheeseworks)!\n\n"
            "5. **Common sense.** - Not every rule can be written. <co>Acting in bad faith</c> because specific unethical behaviors aren't explicitly mentioned here **will still result in punishments**.\n\n"
            "---\n\n"
            "![✳️](frame:collaborationIcon_001.png) <cg>*Caught someone breaking rules? Please do report their comment(s) using the handy report button!*</c>\n\n"
            "---\n\n"
            "That's all, hope you enjoy the mod! ![<3](frame:gj_heartOn_001.png?scale=0.425)";

        void showRules();
    };

    using namespace ui;
};