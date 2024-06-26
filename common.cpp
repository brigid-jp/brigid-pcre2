#include "common.hpp"

namespace brigid {
  namespace {
    void impl_get_version(lua_State* L) {
      static constexpr const char* version =
#include "brigid-pcre2-version"
      ;
      lua_pushstring(L, version);
    }

    void impl_config(lua_State* L) {
      auto what = luaL_checkinteger(L, 1);
      auto size = check(pcre2_config(what, nullptr));

      switch (what) {
        case PCRE2_CONFIG_JITTARGET:
        case PCRE2_CONFIG_UNICODE_VERSION:
        case PCRE2_CONFIG_VERSION:
          {
            std::array<char, 128> buffer;
            if (size > static_cast<int>(buffer.size())) {
              throw pcre2_error(PCRE2_ERROR_NOMEMORY);
            }
            size = check(pcre2_config(what, buffer.data()));
            lua_pushlstring(L, buffer.data(), size - 1);
          }
          return;
      }

      std::uint32_t value = 0;
      if (size != sizeof(value)) {
        throw pcre2_error(PCRE2_ERROR_NOMEMORY);
      }
      check(pcre2_config(what, &value));
      lua_pushinteger(L, value);
    }

    void impl_compile(lua_State* L) {
      auto pattern = checkstring(L, 1);
      auto options = luaL_optinteger(L, 2, 0);
      int error_code = 0;
      std::size_t error_offset = 0;

      auto result = code_t::make_unique(pcre2_compile(
          pattern.data_u8(),
          pattern.size(),
          options,
          &error_code,
          &error_offset,
          nullptr));
      if (!result) {
        std::ostringstream out;
        out << get_error_message(error_code) << " at offset " << error_offset + 1;
        throw pcre2_error(error_code, out.str());
      }
      code_t::newuserdata(L, std::move(result));
    }
  }

  void initialize_common(lua_State* L) {
    setfield(L, -1, "get_version", function<impl_get_version>());
    setfield(L, -1, "config", function<impl_config>());
    setfield(L, -1, "compile", function<impl_compile>());
  }
}
