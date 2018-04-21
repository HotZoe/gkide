function env_check_pre_build() 
{
    echo "GKIDE build env checking ..."
    
    if test "${BUILD_TARGET_32BIT}" = ON ; then
        SHARED_CMAKE_BUILD_FLAGS="${SHARED_CMAKE_BUILD_FLAGS} -DCMAKE_FLAGS_32BIT=ON"
        QT5_INSTALL_PREFIX="/usr/lib/i386-linux-gnu"
    else
        QT5_INSTALL_PREFIX="/usr/lib/x86_64-linux-gnu"
    fi
}

