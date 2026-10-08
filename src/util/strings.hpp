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
};