#pragma once

#define DECLARE_SINGLE(ClassName) \
    public:\
        static ClassName* GetInstance()\
        {\
            if(NULL == m_pInstance)\
            {\
                m_pInstance = new ClassName(); \
            }\
            return m_pInstance;\
        }\
    private:\
        static ClassName* m_pInstance;\

#define IMPLEMENT_SINGLE(ClassName)\
    ClassName* ClassName::m_pInstance = nullptr;

#define CALL_SINGLE(ClassName)  ClassName::GetInstance();