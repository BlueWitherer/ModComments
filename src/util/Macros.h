#pragma once

#define CW_MODCOMMENTS_WEB_BASEURL "https://modcomments.cheeseworks.gay"

#ifndef CW_TESTING
#ifdef CW_WEB_BASEURL
#undef CW_WEB_BASEURL
#endif

#define CW_WEB_BASEURL CW_MODCOMMENTS_WEB_BASEURL
#endif

#include <rhatt/Macros.h>
#include <rhatt/util/WebRes.hpp>