#pragma once

#ifdef CW_TESTING
#define CW_MODCOMMENTS_WEB_BASEURL "http://localhost:6969"
#else
#define CW_MODCOMMENTS_WEB_BASEURL "https://modcomments.cheeseworks.gay"
#endif

#define CW_GEODE_ID "geode.loader"

#define CW_MODCOMMENTS_ARGON_UNWRAP(var)                                                            \
    auto tokenRes = co_await argon::startAuth();                                                    \
    if (tokenRes.isErr()) co_return WebRes(std::nullptr_t(), std::move(tokenRes).unwrapErr(), 402); \
    var = std::move(tokenRes).unwrap()