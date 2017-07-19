# SHA256 => For Linux:$ sha256sum FILE-NAME
# MD5    => For Linux:$ md5sum    FILE-NAME

set(LUA_VERSION         5.3.4)
set(LUA_URL             http://www.lua.org/ftp/lua-5.3.4.tar.gz)
set(LUA_SHA256          f681aa518233bc407e23acf0f5887c884f17436f000d453b2491a9f11a52400c)
list(APPEND dep_urls    ${LUA_URL})

set(LUAJIT_VERSION      2.1.0)
set(LUAJIT_URL          http://luajit.org/download/LuaJIT-2.1.0-beta3.tar.gz)
set(LUAJIT_SHA256       1ad2e34b111c802f9d0cdf019e986909123237a28c746b21295b63c9e785d9c3)
list(APPEND dep_urls    ${LUAJIT_URL})

set(GPERF_VERSION       3.0.4)
set(GPERF_URL           http://ftp.gnu.org/pub/gnu/gperf/gperf-3.0.4.tar.gz)
set(GPERF_SHA256        767112a204407e62dbc3106647cf839ed544f3cf5d0f0523aaa2508623aad63e)
list(APPEND dep_urls    ${GPERF_URL})

set(LIBUV_VERSION       1.9.1)
set(LIBUV_URL           https://github.com/libuv/libuv/archive/v1.9.1.tar.gz)
set(LIBUV_SHA256        a6ca9f0648973d1463f46b495ce546ddcbe7cce2f04b32e802a15539e46c57ad)
list(APPEND dep_urls    ${LIBUV_URL})

set(MSGPACK_VERSION     1.0.0)
set(MSGPACK_URL         https://github.com/msgpack/msgpack-c/archive/cpp-1.0.0.tar.gz)
set(MSGPACK_SHA256      afda64ca445203bb7092372b822bae8b2539fdcebbfc3f753f393628c2bcfe7d)
list(APPEND dep_urls    ${MSGPACK_URL})

set(JEMALLOC_VERSION    4.3.1)
set(JEMALLOC_URL        https://github.com/jemalloc/jemalloc/releases/download/4.3.1/jemalloc-4.3.1.tar.bz2)
set(JEMALLOC_SHA256     f7bb183ad8056941791e0f075b802e8ff10bd6e2d904e682f87c8f6a510c278b)
list(APPEND dep_urls    ${JEMALLOC_URL})

set(LIBICONV_VERSION    1.15)
set(LIBICONV_URL        https://ftp.gnu.org/pub/gnu/libiconv/libiconv-1.15.tar.gz)
set(LIBICONV_SHA256     ccf536620a45458d26ba83887a983b96827001e92a13847b45e4925cc8913178)
list(APPEND dep_urls    ${LIBICONV_URL})

set(LIBTERMKEY_VERSION  0.19)
set(LIBTERMKEY_URL      http://www.leonerd.org.uk/code/libtermkey/libtermkey-0.19.tar.gz)
set(LIBTERMKEY_SHA256   c505aa4cb48c8fa59c526265576b97a19e6ebe7b7da20f4ecaae898b727b48b7)
if(WIN32)
set(LIBTERMKEY_VERSION  0.0)
set(LIBTERMKEY_URL      https://github.com/equalsraf/libtermkey/archive/tb-windows.zip)
set(LIBTERMKEY_SHA256   c81e33e38662b151a49847ff4feef4f8c4b2a66f3e159a28b575cbc9bcd8ffea)
endif()
list(APPEND dep_urls    ${LIBTERMKEY_URL})


set(UNIBILIUM_VERSION   1.2.0)
set(UNIBILIUM_URL       https://github.com/mauke/unibilium/archive/v1.2.0.tar.gz)
set(UNIBILIUM_SHA256    623af1099515e673abfd3cae5f2fa808a09ca55dda1c65a7b5c9424eb304ead8)
list(APPEND dep_urls    ${UNIBILIUM_URL})


set(LUV_VERSION         1.9.1-0)
set(LUV_URL             https://github.com/luvit/luv/archive/1.9.1-0.tar.gz)
set(LUV_SHA256          86a199403856018cd8e5529c8527450c83664a3d36f52d5253cbe909ea6c5a06)
list(APPEND dep_urls    ${LUV_URL})


set(LUAROCKS_VERSION    2.4.2)
set(LUAROCKS_URL        https://github.com/luarocks/luarocks/archive/v2.4.2.tar.gz)
set(LUAROCKS_SHA256     eef88c2429c715a7beb921e4b1ba571dddb7c74a250fbb0d3cc0d4be7a5865d9)
list(APPEND dep_urls    ${LUAROCKS_URL})


# orginal version: http://www.leonerd.org.uk/code/libvterm/
set(LIBVTERM_VERSION    0.0.0)
set(LIBVTERM_GIT_REPO   https://github.com/neovim/libvterm.git)

# wget --input-file=deps-wget-list --continue --directory-prefix=downloads
if(UPDATE_DEPS_WGET_LIST)
    set(deps_wget ${PROJECT_SOURCE_DIR}/deps-wget-list)
    foreach(url ${dep_urls})
        file(APPEND ${deps_wget} "${url}\n")
    endforeach()
endif()
