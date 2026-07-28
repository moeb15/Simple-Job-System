#pragma once

#include "defines.hpp"
#include <utility>
#include <type_traits>

// game engine gems volume 3 chapter 13

template <typename T>
class TFunction;

template <typename RetVal, typename... Args>
class TFunction<RetVal(Args...)>
{
private:
    typedef RetVal(*ProxyFunc)(void*, Args...);

    template <RetVal(*Func)(Args...)>
    static inline RetVal FunctionProxy(void*, Args... args)
    {
        return Func(std::forward<Args>(args)...);
    }

    template <typename C, RetVal(*Func)(Args...)>
    static inline RetVal MethodProxy(void* instance, Args... args)
    {
        return (static_cast<C*>(instance)->*Func)(std::forward<Args>(args)...);
    }

    template <typename C, RetVal(*Func)(Args...)>
    static inline RetVal ConstMethodProxy(void* instance, Args... args)
    {
        return (static_cast<const C*>(instance)->*Func)(std::forward<Args>(args)...);
    }

public:
    TFunction() : m_Instance(nullptr), m_Proxy(nullptr) {}
    ~TFunction() {}

    TFunction(const TFunction& o)
    {
        m_Instance = o.m_Instance;
        m_Proxy = o.m_Proxy;
    }

    TFunction& operator=(const TFunction& o)
    {
        if (this != &o)
        {
            m_Instance = o.m_Instance;
            m_Proxy = o.m_Proxy;
        }

        return *this;
    }

    TFunction(TFunction&& o) noexcept
    {
        m_Instance = o.m_Instance;
        m_Proxy = o.m_Proxy;

        o.m_Instance = nullptr;
        o.m_Proxy = nullptr;
    }

    TFunction& operator=(TFunction&& o) noexcept
    {
        if (this != &o)
        {
            m_Instance = o.m_Instance;
            m_Proxy = o.m_Proxy;

            o.m_Instance = nullptr;
            o.m_Proxy = nullptr;
        }

        return *this;
    }

    template <RetVal(*Func)(Args...)>
    void Bind(void)
    {
        m_Instance = nullptr;
        m_Proxy = &FunctionProxy<Func>;
    }

    template <typename C, RetVal(C::* Func)(Args...)>
    void Bind(C* instance)
    {
        m_Instance = instance;
        m_Proxy = &MethodProxy<C, Func>;
    }

    template <typename C, RetVal(C::* Func)(Args...)>
    void Bind(const C* instance)
    {
        m_Instance = const_cast<C*>(instance);
        m_Proxy = &ConstMethodProxy<C, Func>;
    }

    RetVal Invoke(Args... args) const
    {
        JS_ASSERT(m_Proxy != nullptr);
        return m_Proxy(m_Instance, std::forward<Args>(args)...);
    }

    template <typename U = RetVal>
    std::enable_if<!std::is_same<U, void>::value>::type InvokeIfBound(Args... args) const
    {
        if (m_Proxy != nullptr)
        {
            return m_Proxy(m_Instance, std::forward<Args>(args)...);
        }
        return U{};
    }

    template <typename U = RetVal>
    std::enable_if<std::is_same<U, void>::value>::type InvokeIfBound(Args... args) const
    {
        if (m_Proxy != nullptr)
        {
            return m_Proxy(m_Instance, std::forward<Args>(args)...);
        }
        return;
    }

private:
    void* m_Instance{ nullptr };
    ProxyFunc m_Proxy{ nullptr };
};