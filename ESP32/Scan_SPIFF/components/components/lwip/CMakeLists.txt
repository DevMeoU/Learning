idf_build_get_property(target IDF_TARGET)

# Include Windows-specific file utilities if on Windows
if(CMAKE_HOST_WIN32)
    include(${CMAKE_CURRENT_LIST_DIR}/port/windows_file_utils.cmake)
endif()

if(CONFIG_LWIP_ENABLE)
    if(NOT ${target} STREQUAL "linux")
        # ESP platform targets share the same port folder
        set(target esp32xx)
    endif()

    set(include_dirs
        include
        include/apps
        include/apps/sntp
        lwip/src/include
        port/include
        port/freertos/include/
        port/${target}/include
        port/${target}/include/arch
        port/${target}/include/sys
        )

    set(srcs
        "apps/sntp/sntp.c"
        "lwip/src/api/api_lib.c"
        "lwip/src/api/api_msg.c"
        "lwip/src/api/err.c"
        "lwip/src/api/if_api.c"
        "lwip/src/api/netbuf.c"
        "lwip/src/api/netdb.c"
        "lwip/src/api/netifapi.c"
        "lwip/src/api/sockets.c"
        "lwip/src/api/tcpip.c"
        "lwip/src/apps/sntp/sntp.c"
        "lwip/src/apps/netbiosns/netbiosns.c"
        "lwip/src/core/def.c"
        "lwip/src/core/dns.c"
        "lwip/src/core/inet_chksum.c"
        "lwip/src/core/init.c"
        "lwip/src/core/ip.c"
        "lwip/src/core/mem.c"
        "lwip/src/core/memp.c"
        "lwip/src/core/netif.c"
        "lwip/src/core/pbuf.c"
        "lwip/src/core/raw.c"
        "lwip/src/core/stats.c"
        "lwip/src/core/sys.c"
        "lwip/src/core/tcp.c"
        "lwip/src/core/tcp_in.c"
        "lwip/src/core/tcp_out.c"
        "lwip/src/core/timeouts.c"
        "lwip/src/core/udp.c"
        "lwip/src/core/ipv4/autoip.c"
        "lwip/src/core/ipv4/dhcp.c"
        "lwip/src/core/ipv4/etharp.c"
        "lwip/src/core/ipv4/icmp.c"
        "lwip/src/core/ipv4/igmp.c"
        "lwip/src/core/ipv4/ip4.c"
        "lwip/src/core/ipv4/ip4_napt.c"
        "lwip/src/core/ipv4/ip4_addr.c"
        "lwip/src/core/ipv4/ip4_frag.c"
        "lwip/src/core/ipv6/dhcp6.c"
        "lwip/src/core/ipv6/ethip6.c"
        "lwip/src/core/ipv6/icmp6.c"
        "lwip/src/core/ipv6/inet6.c"
        "lwip/src/core/ipv6/ip6.c"
        "lwip/src/core/ipv6/ip6_addr.c"
        "lwip/src/core/ipv6/ip6_frag.c"
        "lwip/src/core/ipv6/mld6.c"
        "lwip/src/core/ipv6/nd6.c"
        "lwip/src/netif/ethernet.c"
        "lwip/src/netif/bridgeif.c"
        "lwip/src/netif/bridgeif_fdb.c"
        "lwip/src/netif/slipif.c"
        "lwip/src/netif/slipif.c"
        "lwip/src/netif/ppp/auth.c"
        "lwip/src/netif/ppp/ccp.c"
        "lwip/src/netif/ppp/chap-md5.c"
        "lwip/src/netif/ppp/chap-new.c"
        "lwip/src/netif/ppp/chap_ms.c"
        "lwip/src/netif/ppp/demand.c"
        "lwip/src/netif/ppp/eap.c"
        "lwip/src/netif/ppp/ecp.c"
        "lwip/src/netif/ppp/eui64.c"
        "lwip/src/netif/ppp/fsm.c"
        "lwip/src/netif/ppp/ipcp.c"
        "lwip/src/netif/ppp/ipv6cp.c"
        "lwip/src/netif/ppp/lcp.c"
        "lwip/src/netif/ppp/magic.c"
        "lwip/src/netif/ppp/mppe.c"
        "lwip/src/netif/ppp/multilink.c"
        "lwip/src/netif/ppp/ppp.c"
        "lwip/src/netif/ppp/pppapi.c"
        "lwip/src/netif/ppp/pppcrypt.c"
        "lwip/src/netif/ppp/pppoe.c"
        "lwip/src/netif/ppp/pppol2tp.c"
        "lwip/src/netif/ppp/pppos.c"
        "lwip/src/netif/ppp/upap.c"
        "lwip/src/netif/ppp/utils.c"
        "lwip/src/netif/ppp/vj.c"
        "port/hooks/tcp_isn_default.c"
        "port/hooks/lwip_default_hooks.c"
        "port/debug/lwip_debug.c"
        "port/sockets_ext.c"
        "port/freertos/sys_arch.c")

    if(CONFIG_LWIP_NETIF_API)
        list(APPEND srcs "port/if_index.c")
    endif()

    if(CONFIG_LWIP_PPP_SUPPORT)
        list(APPEND srcs
            "lwip/src/netif/ppp/auth.c"
            "lwip/src/netif/ppp/ccp.c"
            "lwip/src/netif/ppp/chap-md5.c"
            "lwip/src/netif/ppp/chap-new.c"
            "lwip/src/netif/ppp/chap_ms.c"
            "lwip/src/netif/ppp/demand.c"
            "lwip/src/netif/ppp/eap.c"
            "lwip/src/netif/ppp/ecp.c"
            "lwip/src/netif/ppp/eui64.c"
            "lwip/src/netif/ppp/fsm.c"
            "lwip/src/netif/ppp/ipcp.c"
            "lwip/src/netif/ppp/ipv6cp.c"
            "lwip/src/netif/ppp/lcp.c"
            "lwip/src/netif/ppp/magic.c"
            "lwip/src/netif/ppp/mppe.c"
            "lwip/src/netif/ppp/multilink.c"
            "lwip/src/netif/ppp/ppp.c"
            "lwip/src/netif/ppp/pppapi.c"
            "lwip/src/netif/ppp/pppcrypt.c"
            "lwip/src/netif/ppp/pppoe.c"
            "lwip/src/netif/ppp/pppol2tp.c"
            "lwip/src/netif/ppp/pppos.c"
            "lwip/src/netif/ppp/upap.c"
            "lwip/src/netif/ppp/utils.c"
            "lwip/src/netif/ppp/vj.c")
    endif()

    if(CONFIG_LWIP_DHCP_DOES_ARP_CHECK)
        list(APPEND srcs "port/acd_dhcp_check.c")
    elseif(CONFIG_LWIP_DHCP_DOES_ACD_CHECK)
        list(APPEND srcs "lwip/src/core/ipv4/acd.c")
    endif()

if(NOT ${target} STREQUAL "linux")
        # Support for vfs and linker fragments only for target builds
        set(linker_fragments linker.lf)
        if(CONFIG_VFS_SUPPORT_IO)
            list(APPEND srcs "port/${target}/vfs_lwip.c")
        else()
            list(APPEND srcs "port/${target}/no_vfs_syscalls.c")
        endif()
        
        # Add Windows-specific workaround file if on Windows
        if(CMAKE_HOST_WIN32)
            list(APPEND srcs "port/${target}/windows_dep_workaround.c")
        endif()
    else()
        # This wraps some posix IO functions to conditionally pass control to lwip
        list(APPEND srcs "port/${target}/vfs_lwip.c")
    endif()

    if(CONFIG_LWIP_ICMP)
        list(APPEND srcs
            "apps/ping/esp_ping.c"
            "apps/ping/ping.c"
            "apps/ping/ping_sock.c")
    endif()

    if(NOT CONFIG_LWIP_USE_EXTERNAL_MBEDTLS)
        list(APPEND srcs
                "lwip/src/netif/ppp/polarssl/arc4.c"
                "lwip/src/netif/ppp/polarssl/des.c"
                "lwip/src/netif/ppp/polarssl/md4.c"
                "lwip/src/netif/ppp/polarssl/md5.c"
                "lwip/src/netif/ppp/polarssl/sha1.c")
    endif()

    if(CONFIG_LWIP_DHCPS)
        list(APPEND srcs "apps/dhcpserver/dhcpserver.c")
    endif()

    if(CONFIG_LWIP_DHCP_RESTORE_LAST_IP)
        list(APPEND srcs "port/esp32xx/netif/dhcp_state.c")
    endif()
endif() # CONFIG_LWIP_ENABLE

if(NOT ${target} STREQUAL "linux")
    set(priv_requires vfs)
endif()

idf_component_register(SRCS "${srcs}"
                    INCLUDE_DIRS  ${include_dirs}
                    LDFRAGMENTS   ${linker_fragments}
                    PRIV_REQUIRES ${priv_requires})

if(CONFIG_LWIP_ENABLE)
    # lots of LWIP source files evaluate macros that check address of stack variables
    target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-address)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE ESP_LWIP_COMPONENT_BUILD)

    # Windows-specific fix for file locking issues during build
    if(CMAKE_HOST_WIN32)
        # Add Windows-specific compiler flags to help with file locking issues
        # The /MP flag enables multi-processor compilation which can help reduce file contention
        # The /FS flag forces synchronous PDB writes to avoid file locking conflicts
        if(MSVC)
            target_compile_options(${COMPONENT_LIB} PRIVATE /FS /MP)
        endif()
        
        # For GCC/Clang on Windows, we can add flags to help with file handling
        if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
            # Add flags to help with file handling on Windows
            target_compile_options(${COMPONENT_LIB} PRIVATE -fno-keep-inline-dllexport)
            
            # Add Windows-specific dependency handling flags
            # These flags help avoid file locking issues with dependency files
            target_compile_options(${COMPONENT_LIB} PRIVATE 
                # Write dependency info to a temporary file first
                "-MMD" "-MP" "-MF" "${CMAKE_CURRENT_BINARY_DIR}/tmp_deps/$<TARGET_FILE_BASE_NAME:${COMPONENT_LIB}>.d.tmp"
            )
            
            # Create the temporary directory for dependency files
            file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/tmp_deps")
        endif()
        
        # Configure CMake to use our custom batch script for handling compilation
        # This script helps manage file locking issues with dependency files
        set_property(TARGET ${COMPONENT_LIB} PROPERTY RULE_LAUNCH_COMPILE 
            "cmd.exe /C \"${CMAKE_CURRENT_LIST_DIR}/port/${target}/windows_build_helper.bat compile\"")
        
        # Set special properties for the Windows dependency workaround file
        set_source_files_properties(
            port/${target}/windows_dep_workaround.c
            PROPERTIES COMPILE_FLAGS
            "-MD -MF NUL" # Disable dependency file generation for this file
        )
        
        # Copy our custom Ninja rules file to the build directory
        # This provides Windows-friendly rules for file operations
        configure_file(
            "${CMAKE_CURRENT_LIST_DIR}/port/${target}/windows_ninja_workaround.ninja"
            "${CMAKE_BINARY_DIR}/windows_ninja_workaround.ninja"
            COPYONLY
        )
        
        # Add a custom command to include our Ninja rules
        add_custom_command(
            TARGET ${COMPONENT_LIB} PRE_BUILD
            COMMAND ${CMAKE_COMMAND} -E echo "subninja windows_ninja_workaround.ninja" >> "${CMAKE_BINARY_DIR}/build.ninja"
            COMMENT "Adding Windows-specific Ninja rules"
        )
    endif()

    set_source_files_properties(
        lwip/src/netif/ppp/pppos.c
        PROPERTIES COMPILE_FLAGS
        -Wno-type-limits
        )
    # "comparison is always false due to limited range of data type" warning
    # when setting CONFIG_LWIP_TCP_WND_DEFAULT to 65535
    set_source_files_properties(
        lwip/src/core/tcp.c
        PROPERTIES COMPILE_FLAGS
        -Wno-type-limits
    )

    # ignore some declaration mismatches
    set_source_files_properties(
        lwip/src/netif/ppp/chap_ms.c
        PROPERTIES COMPILE_FLAGS
        -Wno-array-parameter
    )

    if(CONFIG_OPENTHREAD_ENABLED)
        idf_component_optional_requires(PRIVATE openthread)
    endif()

    if(CONFIG_LWIP_DHCP_RESTORE_LAST_IP)
        idf_component_optional_requires(PRIVATE nvs_flash)
    endif()

    if(CONFIG_LWIP_USE_EXTERNAL_MBEDTLS)
        idf_component_optional_requires(PRIVATE mbedtls)
    endif()

    if(${target} STREQUAL "linux")
        set(THREADS_PREFER_PTHREAD_FLAG ON)
        find_package(Threads REQUIRED)
        target_link_libraries(${COMPONENT_LIB} PRIVATE Threads::Threads)
        set(WRAP_FUNCTIONS      select
                                read
                                fcntl
                                write
                                close)
        foreach(wrap ${WRAP_FUNCTIONS})
                    target_link_libraries(${COMPONENT_LIB} INTERFACE "-Wl,--wrap=${wrap}")
                    target_link_libraries(${COMPONENT_LIB} INTERFACE "-u __wrap_${wrap}")
        endforeach()
    endif()
endif()
