#pragma once

#include <Geode/Geode.hpp>

namespace cw::mod_cmmts {
    struct WebRes final {
    private:
        matjson::Value m_payload;
        std::string m_error;

        uint16_t m_code = 200;

    public:
        WebRes(matjson::Value payload = std::nullptr_t(), std::string error = "", uint16_t code = 200);

        template <typename T>
        geode::Result<T> getPayload() const {
            return m_payload.as<T>();
        };

        matjson::Value& getPayloadValue() & noexcept;
        matjson::Value&& getPayloadValue() && noexcept;
        matjson::Value const& getPayloadValue() const& noexcept;

        geode::ZStringView getError() const noexcept;
        uint16_t getCode() const noexcept;

        bool isOk() const noexcept;
        bool isErr() const noexcept;
    };

    namespace webres {
        WebRes processResp(geode::utils::web::WebResponse const& res);
    };
};

template <>
struct matjson::Serialize<cw::mod_cmmts::WebRes> final {
    static geode::Result<cw::mod_cmmts::WebRes> fromJson(matjson::Value const& value);
    static matjson::Value toJson(cw::mod_cmmts::WebRes const& value);
};