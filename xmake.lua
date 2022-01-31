add_rules("mode.debug", "mode.release", "mode.profile")
set_languages("cxx17")

-- set_warnings("all", "error")

-- 如果当前编译模式是debug
if is_mode("debug") then
    -- 添加DEBUG编译宏
    add_defines("DEBUG")
    -- 启用调试符号
    set_symbols("debug")
    -- 禁用优化
    set_optimize("none")
end

-- 如果是release或者profile模式
if is_mode("release", "profile") then

    set_optimize("fastest")

    -- 如果是release模式
    if is_mode("release") then
        -- 隐藏符号
        set_symbols("hidden")
        -- strip所有符号
        set_strip("all")
        -- -- 忽略帧指针
        -- if has_tool("cxx", "gcc", "gxx") then
        --     add_cxflags("-fomit-frame-pointer")
        --     add_mxflags("-fomit-frame-pointer")
        -- end
        -- https://docs.microsoft.com/zh-cn/cpp/build/reference/oy-frame-pointer-omission
        -- Ox 自带

    -- 如果是profile模式
    else
        -- 启用调试符号
        set_symbols("debug")
    end

    -- 添加扩展指令集
    add_vectorexts("sse2", "sse3", "ssse3", "mmx", "avx")
end

add_repositories("my-repo myrepo")
add_requires("myopenmp")

target("main")
    set_kind("binary")
    add_files("main.cpp")
    -- add_vectorexts("avx")

    add_packages("myopenmp")