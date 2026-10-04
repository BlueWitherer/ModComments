#include "../WebRes.hpp"

#include <Util.h>

#include <Geode/Geode.hpp>

using namespace geode::prelude;
using namespace cw::mod_cmmts;

Result<WebRes> matjson::Serialize<WebRes>::fromJson(matjson::Value const& value) {
    GEODE_UNWRAP_INTO(std::string error, value["error"].asString());

    uint16_t code = 200;
    GEODE_UNWRAP_INTO_IF_OK(code, value["code"].asUInt());

    return Ok(WebRes(value["payload"], std::move(error), code));
};

matjson::Value matjson::Serialize<WebRes>::toJson(WebRes const& value) {
    matjson::Value obj;

    obj["payload"] = value.getPayloadValue();
    obj["error"] = value.getError();
    obj["code"] = value.getCode();

    return obj;
};

WebRes::WebRes(matjson::Value payload, std::string error, uint16_t code) : m_payload(std::move(payload)), m_error(std::move(error)), m_code(code) {};

matjson::Value& WebRes::getPayloadValue() & noexcept {
    return m_payload;
};

matjson::Value&& WebRes::getPayloadValue() && noexcept {
    return std::move(m_payload);
};

matjson::Value const& WebRes::getPayloadValue() const& noexcept {
    return m_payload;
};

ZStringView WebRes::getError() const noexcept {
    return m_error;
};

uint16_t WebRes::getCode() const noexcept {
    return m_code;
};

bool WebRes::isOk() const noexcept {
    return !m_payload.isNull() && m_error.empty();
};

bool WebRes::isErr() const noexcept {
    return m_payload.isNull() || !m_error.empty();
};

WebRes webres::processResp(geode::utils::web::WebResponse const& res) {
    auto const fallback = [&res](std::string err) {
        log::error("Failed to sync game settings: {}", err);
        return WebRes(std::nullptr_t(), std::move(err), res.code());
    };

    auto jsonRes = res.json();
    if (jsonRes.isErr()) return fallback(std::move(jsonRes).unwrapErr());

    auto json = std::move(jsonRes).unwrap();
    if (!json.contains("code") || !json["code"].isNumber()) json["code"] = res.code();

    auto resp = std::move(json).as<WebRes>();
    if (resp.isErr()) return fallback(std::move(resp).unwrapErr());

    return std::move(resp).unwrap();
};