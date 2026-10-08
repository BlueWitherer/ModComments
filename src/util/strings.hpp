#pragma once

#include "Macros.h"

#include <Geode/utils/string.hpp>

namespace cw::mod_cmmts {
    namespace str = geode::utils::string;

    namespace url {
        static constexpr auto apiBase = CW_MODCOMMENTS_WEB_BASEURL;

        std::string apiEndpoint(std::string_view path);
    };

    namespace plurals {
        std::string appendS(std::string_view word, uint64_t amount, bool uppercase = false);
        std::string versatile(std::string_view singular, std::string_view plural, uint64_t amount);
    };

    inline std::string operator""_api(const char* str, size_t len) {
        return url::apiEndpoint(std::string_view{str, len});
    };

    namespace rules {
        static constexpr auto fullText =
            "**Upon posting comments, you agree to adhere to the following <cr>rules</c>.**\n\n"
            "---\n\n"
            "![🛑](frame:geode.loader/info-alert.png?scale=0.375) <cr>*[Mod Comments](mod:cheeseworks.modcomments) was made for users to leave feedback on mods. **Mod developers are NOT responsible for providing user support through comments**, please __instead contact them through their own official channels__.*</c>\n\n"
            "---\n\n"
            "1. **Be Civil** - Engage in conversations that are <cg>respectful, fun, and constructive</c>. Any <co>comments made with the intention to hurt another individual or community</c> are **strictly prohibited**. Swearing is fine as long as it's kept at a minimum.\n\n"
            "2. **Don't Spam** - Avoid <co>going too off-topic in the comments</c>. Attempting to <co>overload our servers or bypass spam protections</c> will result in **rate-limiting & IP bans**. Other means of spam such as self-promotion are not tolerated.\n\n"
            "3. **No NSFW** - <co>Inappropriate discussions</c> are **not allowed**. <cg>Respect boundares</c>, and keep comment sections <cg>safe and welcoming for everyone</c>!\n\n"
            "4. **Staff Decisions** - Moderators are instructed to take <cg>whatever means necessary to keep chats safe</c>. If you believe action has been <co>wrongfully taken against you</c>, you can communicate your concerns in [our Discord server](https://www.dsc.gg/cheeseworks)!\n\n"
            "5. **Common Sense** - Not every rule can be written. <co>Acting in bad faith</c> because specific destructive behaviors aren't explicitly mentioned here **will still result in punishments**.\n\n"
            "---\n\n"
            "![🗨️](frame:gj_discordIcon_001.png?scale=0.375) **If you need help, join my [support Discord server](https://www.dsc.gg/cheeseworks) and ask! :)**";
    };
};