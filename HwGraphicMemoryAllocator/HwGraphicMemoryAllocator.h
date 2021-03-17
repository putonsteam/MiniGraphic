//******************************************************************************
//	@file	HwcGraphicMemoryHeap.cpp
//	@brief	NVN Graphic Memory Allocator
//	@author	Hu Ke
//******************************************************************************
#ifndef _HW_GRAPHIC_MEMORY_ALLOCATOR_H_
#define _HW_GRAPHIC_MEMORY_ALLOCATOR_H_

#include "Hw/Debug/System/Platforms/Switch/HwDebugPlatformSwitch.h"

//==========================================================================
// Defines
//==========================================================================
using pAllocFunc = void*(*)(size_t, size_t);
using pDeallocFunc = void(*)(void*);

#define ENUM2INT(Enum) static_cast<std::underlying_type<decltype(Enum)>::type>(Enum)
#define MAX_HEAP_LEVEL_COUNT 3
#define GRAPHIC_MEMORY_AUTO_LOCK(Lock) cGraphicMemoryAutoLock<decltype(Lock)> _Lock(Lock)

namespace Hw
{
    //==========================================================================
    //    GRAPHIC_MEMORY_ERROR
    //==========================================================================
    enum class GRAPHIC_MEMORY_ERROR
    {
        NONE = 0,

        UNKOWN_ERROR,
        ALLOCATOR_IS_NOT_INITIALIZED,
        ALLOCATE_SIZE_ZERO_INVALID,
        ALLOCATE_SIZE_OVER_HEAP_SIZE,
        ALLOCATE_ALIGNMENT_INVALID,
        CREATE_NVN_MEMORY_POOL_FAILED,
        NVN_DEVICE_INVAILD,
        HW_HEAP_INVALID,
        POOL_INVALID,
        NO_ALLOCABLE_POOL,
        HEAP_INVALID,
        NO_ALLOCABLE_HEAP,
        OUT_OF_MEMORY,
    };

    //==========================================================================
    //    cGraphicMemorySpinLock
    //==========================================================================
    class cGraphicMemorySpinLock
    {
    public:
        cGraphicMemorySpinLock();
        ~cGraphicMemorySpinLock();

        void create(uint32_t MaxTryCount = 1000);
        void lock();
        void unlock();
        bool tryLock();
        bool isLocked();

    private:
        std::atomic<bool> m_Lock;
        uint32_t m_MaxTryCount;
    };

    //==========================================================================
    //    cGraphicMemoryAutoLock
    //==========================================================================
    template<typename T>
    class cGraphicMemoryAutoLock
    {
    public:
        cGraphicMemoryAutoLock(T& mutex)
            : m_pMutex(&mutex)
        {
            m_pMutex->lock();
        }

        ~cGraphicMemoryAutoLock()
        {
            m_pMutex->unlock();
        }
    private:
        T* m_pMutex;
    };


    //==========================================================================
    //    cGraphicMemoryLinkedNode
    //==========================================================================
    template<typename T>
    class cGraphicMemoryLinkedNode
    {
    public:
        cGraphicMemoryLinkedNode()
            : m_pPrev(nullptr)
            , m_pNext(nullptr)
        {

        }

        ~cGraphicMemoryLinkedNode()
        {

        }

        // sets & gets
        inline void setPrev(T* pPrev) { m_pPrev = pPrev; }
        inline void setNext(T* pNext) { m_pNext = pNext; }

        inline T* getPrev() { return m_pPrev; }
        inline T* getNext() { return m_pNext; }

        inline const T* getPrev() const { return m_pPrev; }
        inline const T* getNext() const { return m_pNext; }

        inline void clear()
        {
            m_pPrev = nullptr;
            m_pNext = nullptr;
        }

    private:
        T* m_pPrev;
        T* m_pNext;
    };

    //==========================================================================
    //    cGraphicMemoryLinkedNodeEx
    //==========================================================================
    template<typename T>
    class cGraphicMemoryLinkedNodeEx : public cGraphicMemoryLinkedNode<T>
    {
    public:
        cGraphicMemoryLinkedNodeEx()
            : cGraphicMemoryLinkedNode<T>()
            , m_pPrevEx(nullptr)
            , m_pNextEx(nullptr)
        {

        }

        ~cGraphicMemoryLinkedNodeEx()
        {

        }


        // sets & gets
        inline void setPrevEx(T* pPrevEx) { m_pPrevEx = pPrevEx; }
        inline void setNextEx(T* pNextEx) { m_pNextEx = pNextEx; }

        inline T* getNextEx() { return m_pNextEx; }
        inline T* getPrevEx() { return m_pPrevEx; }

        inline const T* getPrevEx() const { return m_pPrevEx; }
        inline const T* getNextEx() const { return m_pNextEx; }

        inline void clearEx()
        {
            m_pPrevEx = nullptr;
            m_pNextEx = nullptr;
        }

    private:
        T* m_pPrevEx;
        T* m_pNextEx;
    };

    //==========================================================================
    //    cGraphicMemoryLinkedNodeFuncHelper
    //==========================================================================
    template<typename T, typename NT>
    class cGraphicMemoryLinkedNodeFuncHelper;

    template<typename T>
    class cGraphicMemoryLinkedNodeFuncHelper<T, cGraphicMemoryLinkedNodeEx<T>>
    {
    public:
        static void setNext(T* pNode, T* pNext) { pNode->setNextEx(pNext); }
        static void setPrev(T* pNode, T* pPrev) { pNode->setPrevEx(pPrev); }

        static T* getNext(T* pNode) { return pNode->getNextEx(); };
        static T* getPrev(T* pNode) { return pNode->getPrevEx(); };

        static const T* getNext(const T* pNode) { return pNode->getNextEx(); };
        static const T* getPrev(const T* pNode) { return pNode->getPrevEx(); };

        static void clear(T* pNode) { pNode->clearEx(); }
    };

    template<typename T>
    class cGraphicMemoryLinkedNodeFuncHelper<T, cGraphicMemoryLinkedNode<T>>
    {
    public:
        static void setNext(T* pNode, T* pNext) { pNode->setNext(pNext); }
        static void setPrev(T* pNode, T* pPrev) { pNode->setPrev(pPrev); }

        static T* getNext(T* pNode) { return pNode->getNext(); };
        static T* getPrev(T* pNode) { return pNode->getPrev(); };

        static const T* getNext(const T* pNode) { return pNode->getNext(); };
        static const T* getPrev(const T* pNode) { return pNode->getPrev(); };

        static void clear(T* pNode) { pNode->clear(); }
    };

    //==========================================================================
    //    TcGraphicMemoryLinkedList
    //==========================================================================
    template<typename T, typename NT>
    class TcGraphicMemoryLinkedList
    {
    public:
        class Iterator
        {
        public:
            Iterator(T* pNode)
                : m_pNode(pNode)
            {

            }

            Iterator()
                : m_pNode(nullptr)
            {

            }

            Iterator(const Iterator& Iter)
                : m_pNode(Iter.m_pNode)
            {

            }

            Iterator& operator++()
            {
                HW_ASSERT(m_pNode != nullptr);
                m_pNode = cGraphicMemoryLinkedNodeFuncHelper<T, NT>::getNext(m_pNode);
                return *this;
            }

            Iterator operator++(int)
            {
                Iterator temp(*this);
                operator++();
                return temp;
            }

            Iterator& operator--()
            {
                HW_ASSERT(m_pNode != nullptr);
                m_pNode = cGraphicMemoryLinkedNodeFuncHelper<T, NT>::getPrev(m_pNode);
                return *this;
            }

            Iterator operator--(int)
            {
                Iterator temp(*this);
                operator--();
                return temp;
            }

            T* operator*()
            {
                HW_ASSERT(m_pNode != nullptr);
                return m_pNode;
            }

            const T* operator*() const
            {
                HW_ASSERT(m_pNode != nullptr);
                return m_pNode;
            }

            bool operator!=(const Iterator& Iter)
            {
                return m_pNode != Iter.m_pNode;
            }

        private:
            T* m_pNode;
        };


    public:
        TcGraphicMemoryLinkedList()
            : m_pHead(nullptr)
        {

        }

        ~TcGraphicMemoryLinkedList()
        {

        }

        template<typename TConditionFunc>
        T* find(TConditionFunc ConditionFunc)
        {
            T* pCurr = m_pHead;
            while (pCurr != nullptr)
            {
                if (ConditionFunc(pCurr))
                {
                    break;
                }
                pCurr = cGraphicMemoryLinkedNodeFuncHelper<T, NT>::getNext(pCurr);
            }
            return pCurr;
        }

        template<typename TConditionFunc, typename TCompareFunc>
        T* find(TConditionFunc ConditionFunc, TCompareFunc CompareFunc)
        {
            T* pCurr = m_pHead;
            T* pLastResult = nullptr;
            while (pCurr != nullptr)
            {
                if (ConditionFunc(pCurr))
                {
                    if (pLastResult == nullptr)
                    {
                        pLastResult = pCurr;
                    }
                    else
                    {
                        pLastResult = CompareFunc(pCurr, pLastResult);
                    }
                }
                pCurr = cGraphicMemoryLinkedNodeFuncHelper<T, NT>::getNext(pCurr);
            }
            return pLastResult;
        }

        void insertAfter(T* pAftter, T* pNode)
        {
            HW_ASSERT(pAftter != nullptr);
            HW_ASSERT(pNode != nullptr);

            T* pNext = cGraphicMemoryLinkedNodeFuncHelper<T, NT>::getNext(pAftter);
            cGraphicMemoryLinkedNodeFuncHelper<T, NT>::setPrev(pNode, pAftter);
            cGraphicMemoryLinkedNodeFuncHelper<T, NT>::setNext(pNode, pNext);
            cGraphicMemoryLinkedNodeFuncHelper<T, NT>::setNext(pAftter, pNode);
            if (pNext != nullptr)
            {
                cGraphicMemoryLinkedNodeFuncHelper<T, NT>::setPrev(pNext, pNode);
            }
        }

        void insertBefore(T* pBefore, T* pNode)
        {
            HW_ASSERT(pBefore != nullptr);
            HW_ASSERT(pNode != nullptr);
            T* pPrev = cGraphicMemoryLinkedNodeFuncHelper<T, NT>::getPrev(pBefore);
            cGraphicMemoryLinkedNodeFuncHelper<T, NT>::setNext(pNode, pBefore);
            cGraphicMemoryLinkedNodeFuncHelper<T, NT>::setPrev(pNode, pPrev);
            cGraphicMemoryLinkedNodeFuncHelper<T, NT>::setPrev(pBefore, pNode);
            if (pPrev != nullptr)
            {
                cGraphicMemoryLinkedNodeFuncHelper<T, NT>::setNext(pPrev, pNode);
            }
        }

        void insert(T* pNode)
        {
            if (m_pHead == nullptr)
            {
                m_pHead = pNode;
            }
            else
            {
                insertBefore(m_pHead, pNode);
                m_pHead = pNode;
            }
        }

        bool isExist(T* pNode)
        {
            T* pCurrNode = m_pHead;
            bool Found = false;
            while (pCurrNode != nullptr)
            {
                if (pCurrNode == pNode)
                {
                    Found = true;
                    break;
                }
                pCurrNode = cGraphicMemoryLinkedNodeFuncHelper<T, NT>::getNext(pCurrNode);
            }
            return Found;
        }

        void erase(T* pNode)
        {
            HW_ASSERT(pNode != nullptr);
            HW_ASSERT(isExist(pNode));


            if (m_pHead == pNode)
            {
                m_pHead = cGraphicMemoryLinkedNodeFuncHelper<T, NT>::getNext(pNode);
            }

            T* pPrev = cGraphicMemoryLinkedNodeFuncHelper<T, NT>::getPrev(pNode);
            T* pNext = cGraphicMemoryLinkedNodeFuncHelper<T, NT>::getNext(pNode);

            if (pPrev != nullptr)
            {
                cGraphicMemoryLinkedNodeFuncHelper<T, NT>::setNext(pPrev, pNext);
            }

            if (pNext != nullptr)
            {
                cGraphicMemoryLinkedNodeFuncHelper<T, NT>::setPrev(pNext, pPrev);
            }

            cGraphicMemoryLinkedNodeFuncHelper<T, NT>::clear(pNode);
        }

        T* pop()
        {
            if (m_pHead == nullptr)
            {
                return nullptr;
            }
            T* pRet = m_pHead;
            erase(m_pHead);
            return pRet;
        }

        Iterator begin()
        {
            return Iterator(m_pHead);
        }

        Iterator end()
        {
            return Iterator(nullptr);
        }

    private:
        T* m_pHead;
    };

    template<typename T>
    using cGraphicMemoryLinkedList = TcGraphicMemoryLinkedList<T, cGraphicMemoryLinkedNode<T>>;

    template<typename T>
    using cGraphicMemoryLinkedListEx = TcGraphicMemoryLinkedList<T, cGraphicMemoryLinkedNodeEx<T>>;

    //==========================================================================
    //    GRAPHIC_MEMORY_INTERNAL_DATA_ALLOCATOR_INIT_INFO
    //==========================================================================
    struct GRAPHIC_MEMORY_INTERNAL_DATA_ALLOCATOR_INIT_INFO
    {
        uint32_t Count;
        cHeap* pWorkHeap;
    };

    //==========================================================================
    //    cGraphicMemoryInternalDataAllocator
    //==========================================================================
    template<typename T>
    class cGraphicMemoryInternalDataAllocator
    {
    public:
        cGraphicMemoryInternalDataAllocator()
            : m_pWorkHeap(nullptr)
            , m_TotalCount(0)
            , m_AllocatedCount(0)
            , m_Mutex()
        {

        }
        ~cGraphicMemoryInternalDataAllocator()
        {

        }

        static void createInstnace(const GRAPHIC_MEMORY_INTERNAL_DATA_ALLOCATOR_INIT_INFO& Info)
        {
            void* pMem = Info.pWorkHeap->alloc(sizeof(cGraphicMemoryInternalDataAllocator<T>), alignof(cGraphicMemoryInternalDataAllocator<T>));
            s_pInstance = new (pMem) cGraphicMemoryInternalDataAllocator<T>();
            GRAPHIC_MEMORY_ERROR error = s_pInstance->create(Info);
            HW_ASSERT(error == GRAPHIC_MEMORY_ERROR::NONE);
        }

        static void releaseInstance()
        {
            s_pInstance->release();
            s_pInstance->m_pWorkHeap->dealloc(s_pInstance);
            s_pInstance = nullptr;
        }

        static cGraphicMemoryInternalDataAllocator* getInstance()
        {
            HW_ASSERT(s_pInstance);
            return s_pInstance;
        }

        GRAPHIC_MEMORY_ERROR create(const GRAPHIC_MEMORY_INTERNAL_DATA_ALLOCATOR_INIT_INFO& Info)
        {
            if (Info.pWorkHeap == nullptr)
            {
                return GRAPHIC_MEMORY_ERROR::HW_HEAP_INVALID;
            }

            m_pWorkHeap = Info.pWorkHeap;

            T* m_pData = (T*)m_pWorkHeap->alloc(sizeof(T) * Info.Count, alignof(T));
            for (int i = 0; i < Info.Count; ++i)
            {
                new (&m_pData[i]) T();
                m_FreeList.insert(&m_pData[i]);
            }
            m_TotalCount = Info.Count;

            m_Mutex.create();

            return GRAPHIC_MEMORY_ERROR::NONE;
        }

        void release()
        {
            if (m_pData != nullptr)
            {
                m_pWorkHeap->dealloc(m_pData);
                m_pData = nullptr;
            }
        }

        T* pop()
        {
            HW_ASSERT(m_AllocatedCount < m_TotalCount);
            GRAPHIC_MEMORY_AUTO_LOCK(m_Mutex);
            ++m_AllocatedCount;
            return new (m_FreeList.pop()) T();
        }

        void push(T* pData)
        {
            GRAPHIC_MEMORY_AUTO_LOCK(m_Mutex);
            m_FreeList.insert(pData);
            --m_AllocatedCount;
        }

    private:
        cHeap* m_pWorkHeap;

        T* m_pData;
        cGraphicMemoryLinkedList<T> m_FreeList;
        uint32_t m_TotalCount;
        uint32_t m_AllocatedCount;

        cGraphicMemorySpinLock m_Mutex;

        static cGraphicMemoryInternalDataAllocator* s_pInstance;
    };

    template<typename T>
    cGraphicMemoryInternalDataAllocator<T>* cGraphicMemoryInternalDataAllocator<T>::s_pInstance = nullptr;

    //==========================================================================
    //    GRAPHIC_MEMORY_RESOURCE_USAGE
    //==========================================================================
    enum class GRAPHIC_MEMORY_RESOURCE_USAGE
    {
        SHADER_CODE,
        COMMANDS,

        SAMPLED_TEXTURE,
        STORAGE_TEXTURE,
        COLOR_TARGET,
        DEPTH_STENCIL,

        UNIFORM_BUFFER,
        STORAGE_BUFFER,
        INDEX_BUFFER,
        VERTEX_BUFFER,
        INDIRECT_BUFFER,
        QUERY_BUFFER,

        COUNT,
        NONE = -1,
    };

    //==========================================================================
    //    GRAPHIC_MEMORY_POOL_TYPE
    //==========================================================================
    enum class GRAPHIC_MEMORY_POOL_TYPE
    {
        SHADER_CODE = 0,
        COMMANDS,
        TEXTURE,
        RENDER_TARGET,
        BUFFER,
        GEOMETRY,
        MISC,
        COUNT,
        NONE = -1,
        DEDICATED = -2,
    };

    struct GRAPHIC_MEMORY_CONFIG
    {
        size_t TotalSize;
        size_t Alignment;
        size_t MinimumBlockSize;
        bool AllowMultiLevelBlocks;
        uint32_t MaxCleanUpFrameCount;
        bool AllowDedicated;
    };

    struct GRAPHIC_MEMORY_BUDGET
    {
       std::atomic<size_t> Total;
       std::atomic<size_t> Used;
    };

    //==========================================================================
    //    GRAPHIC_MEMORY_BLOCK_TAG
    //==========================================================================
    enum class GRAPHIC_MEMORY_BLOCK_TAG
    {
        INVALID,
        FREE,
        ALLOCATED,
        TAIL,
    };

    class cGraphicMemoryAllocation;
    class cGraphicMemoryAllocator;
    class cIGraphicMemoryPool;
    class cGraphicMemoryPool;
    class cGraphicMemoryDedicatedPool;
    class cGraphicMemoryHeap;
    class cGraphicMemoryBlock;


    struct cGraphicMemoryAllocationRequest
    {
        cGraphicMemoryHeap* pTargetHeap;
        cGraphicMemoryBlock* pTargetBlock;
        size_t Size;
        size_t Alignment;
        GRAPHIC_MEMORY_POOL_TYPE PoolType;
        GRAPHIC_MEMORY_RESOURCE_USAGE ResourceUsage;
    };

    //==========================================================================
    //    cGraphicMemoryBlock
    //==========================================================================
    class cGraphicMemoryBlock : public cGraphicMemoryLinkedNodeEx<cGraphicMemoryBlock>
    {
    public:
        cGraphicMemoryBlock();
        ~cGraphicMemoryBlock();

        // generic
        void create(ptrdiff_t Offset, GRAPHIC_MEMORY_BLOCK_TAG Tag);
        void release();

        // info
        size_t getSize();
        inline ptrdiff_t getOffset() const { return m_Offset; }
        inline void setOffset(ptrdiff_t Offset) { m_Offset = Offset; }
        bool checkAllocableSize(size_t Size, size_t Alignment);

        // tag
        inline void setTag(GRAPHIC_MEMORY_BLOCK_TAG Tag) { m_Tag = Tag; }
        inline bool isInvalid() const { return m_Tag == GRAPHIC_MEMORY_BLOCK_TAG::INVALID; }
        inline bool isFree() const { return m_Tag == GRAPHIC_MEMORY_BLOCK_TAG::FREE; }
        inline bool isAllocated() const { return m_Tag == GRAPHIC_MEMORY_BLOCK_TAG::ALLOCATED; }
        inline bool isTail() const { return m_Tag == GRAPHIC_MEMORY_BLOCK_TAG::TAIL; }

        inline size_t getFragmentSize() const { return m_FragmentSize; }
        inline void setFragmentSize(size_t Size) { m_FragmentSize = Size; }

    private:
        GRAPHIC_MEMORY_BLOCK_TAG m_Tag;
        ptrdiff_t m_Offset;
        size_t m_FragmentSize;
    };

    //==========================================================================
    //    cGraphicMemoryHeap
    //==========================================================================
    class cGraphicMemoryHeap : public cGraphicMemoryLinkedNode<cGraphicMemoryHeap>
    {
    public:
        friend class cGraphicMemoryPool;
        friend class cGraphicMemoryDedicatedPool;

        cGraphicMemoryHeap();
        ~cGraphicMemoryHeap();

        // generic
        GRAPHIC_MEMORY_ERROR create(const GRAPHIC_MEMORY_CONFIG& Config, cIGraphicMemoryPool* pOwnerPool, nvn::MemoryPool* pNVNMemoryPool, void* pVramAddress, bool IsDedicated = false);
        GRAPHIC_MEMORY_ERROR release();
        GRAPHIC_MEMORY_ERROR allocMemory(const cGraphicMemoryAllocationRequest& Reqest, cGraphicMemoryAllocation* pAllocation);
        GRAPHIC_MEMORY_ERROR freeMemory(cGraphicMemoryAllocation* pBlock);
        cGraphicMemoryBlock* findBlock(size_t Size, size_t Alignment);
        inline bool checkAllocableSize(size_t Size) { return (m_TotalSize - m_AllocatedSize) >= Size; }
        GRAPHIC_MEMORY_ERROR updateUnusedFrameCount();

        // statistics
        inline size_t getTotalSize() const { return m_TotalSize; }
        inline size_t getAllocatedSize() const { return m_AllocatedSize; }
        inline size_t getFragmentSize() const { return m_FragmentSize; }
        inline uint32_t getUnusedFrameCount() const { return m_UnusedFrameCount; }
        inline uint32_t getCleanupFrameCount() const { return m_MaxCleanupFrameCount; }

        inline void lock() { m_SpinLock.lock(); }
        inline void unlock() { m_SpinLock.unlock(); }
        inline bool tryLock() { return m_SpinLock.tryLock(); }
        inline bool isLocked() { return m_SpinLock.isLocked(); }

        // debug
        bool checkIsOverlap();

        // nvn interface
        nvn::MemoryPool* getNVNMemoryPool() { return m_pMemoryPool; }

    private:
        void addToFreeBlockList(cGraphicMemoryBlock* pBlock);
        void removeFromFreeBlockList(cGraphicMemoryBlock* pBlock);
        
    private:
        size_t m_TotalSize;
        size_t m_AllocatedSize;
        size_t m_MinimumBlockSize;
        size_t m_FragmentSize;
        cGraphicMemorySpinLock m_SpinLock;

        cIGraphicMemoryPool* m_pOwnerPool;
        cGraphicMemoryLinkedList<cGraphicMemoryBlock> m_BlockList;
        cGraphicMemoryLinkedListEx<cGraphicMemoryBlock> m_FreeBlockLists;

        uint32_t m_UnusedFrameCount;
        uint32_t m_MaxCleanupFrameCount;

        nvn::MemoryPool* m_pMemoryPool;
        void* m_pVramAddress;
        
        bool m_IsDedicated;
    };

    //==========================================================================
    //    cIGraphicMemoryPool
    //==========================================================================
    class cIGraphicMemoryPool
    {
    public:
        cIGraphicMemoryPool() = default;
        virtual ~cIGraphicMemoryPool() = default;

        virtual GRAPHIC_MEMORY_ERROR allocMemory(GRAPHIC_MEMORY_RESOURCE_USAGE Usage, size_t Size, size_t Alignment, cGraphicMemoryAllocation* pAllocation) = 0;
        virtual GRAPHIC_MEMORY_ERROR freeMemory(cGraphicMemoryAllocation* pAllocation) = 0;

        // internal 
        inline cGraphicMemoryAllocator* getAllocator() { return m_pAllocator; }

    protected:
        cGraphicMemoryAllocator* m_pAllocator{ nullptr };
    };

    //==========================================================================
    //    cGraphicMemoryPool
    //==========================================================================
    class cGraphicMemoryPool : public cIGraphicMemoryPool
    {
    public:
        typedef cGraphicMemoryLinkedList<cGraphicMemoryHeap> HeapLinkedList;

        cGraphicMemoryPool();
        ~cGraphicMemoryPool();

        // generic
        GRAPHIC_MEMORY_ERROR create(GRAPHIC_MEMORY_POOL_TYPE Type, cGraphicMemoryAllocator* pAllocator, bool EnableMultiLevelHeaps);
        GRAPHIC_MEMORY_ERROR release();
        virtual GRAPHIC_MEMORY_ERROR allocMemory(GRAPHIC_MEMORY_RESOURCE_USAGE Usage, size_t Size, size_t Alignment, cGraphicMemoryAllocation* pAllocation);
        virtual GRAPHIC_MEMORY_ERROR freeMemory(cGraphicMemoryAllocation* pAllocation) { return GRAPHIC_MEMORY_ERROR::NONE; };
        GRAPHIC_MEMORY_ERROR shrink();

        // statistics
        size_t getTotalSize();
        size_t getAllocatedSize();
        size_t getFragmentSize();
        inline uint32_t getHeapCount() const { return m_HeapCount; }

        // debug
        bool checkIsOverlap();

    private:
        cGraphicMemoryHeap* createHeap(uint32_t Level);
        GRAPHIC_MEMORY_ERROR createAllocationRequest(cGraphicMemoryAllocationRequest& Request, size_t Size, size_t Alignment);

        inline uint32_t getMaxLevelCount() { return m_EnableMultiLevelHeaps ? MAX_HEAP_LEVEL_COUNT : 1; }
        inline uint32_t getHeapLevel(size_t Size);

    private:
        HeapLinkedList* m_HeapList;
        uint32_t m_HeapCount;
        bool m_EnableMultiLevelHeaps;
        GRAPHIC_MEMORY_POOL_TYPE m_Type;
        cGraphicMemorySpinLock m_Mutex;
    };

    class cGraphicMemoryDedicatedPool : public cIGraphicMemoryPool
    {
    public:
        typedef cGraphicMemoryLinkedList<cGraphicMemoryHeap> HeapLinkedList;

        cGraphicMemoryDedicatedPool();
        ~cGraphicMemoryDedicatedPool();

        // generic
        GRAPHIC_MEMORY_ERROR create(cGraphicMemoryAllocator* pAllocator);
        GRAPHIC_MEMORY_ERROR release();
        virtual GRAPHIC_MEMORY_ERROR allocMemory(GRAPHIC_MEMORY_RESOURCE_USAGE Usage, size_t Size, size_t Alignment, cGraphicMemoryAllocation* pAllocation);
        virtual GRAPHIC_MEMORY_ERROR freeMemory(cGraphicMemoryAllocation* pAllocation);
        GRAPHIC_MEMORY_ERROR update();

        // statistics
        size_t getTotalSize();
        size_t getAllocatedSize();
        size_t getFragmentSize();
        inline uint32_t getHeapCount() const { return m_HeapCount; }


    private:
        GRAPHIC_MEMORY_POOL_TYPE m_Type;
        HeapLinkedList* m_HeapList;
        uint32_t m_HeapCount;

        cGraphicMemorySpinLock m_Mutex;
    };

    //==========================================================================
    //    GRAPHIC_MEMORY_ALLOCATOR_INIT_INFO
    //==========================================================================
    struct GRAPHIC_MEMORY_ALLOCATOR_INIT_INFO
    {
        nvn::Device* pNVNDevice;
        cHeap* pWorkHeap;
        cHeap* pVramHeap;
    };

    //==========================================================================
    //    cGraphicMemoryAllocator
    //==========================================================================
    class cGraphicMemoryAllocator
    {
    public:
        friend class cGraphicMemoryDedicatedPool;

        cGraphicMemoryAllocator();
        ~cGraphicMemoryAllocator();

        // generic
        GRAPHIC_MEMORY_ERROR create(const GRAPHIC_MEMORY_ALLOCATOR_INIT_INFO& Info);
        GRAPHIC_MEMORY_ERROR release();
        GRAPHIC_MEMORY_ERROR allocMemory(GRAPHIC_MEMORY_RESOURCE_USAGE Usage, size_t Size, size_t Alignment, cGraphicMemoryAllocation* pAllocation);
        GRAPHIC_MEMORY_ERROR beginRender();
        GRAPHIC_MEMORY_ERROR endRender();

        // cpu alloc/dealloc
        void* workAlloc(size_t Size, size_t Alignment) { return m_pWorkHeap->alloc(Size, Alignment); }
        void workDealloc(void* Ptr) { m_pWorkHeap->dealloc(Ptr); }
        void* vramAlloc(size_t Size, size_t Alignment) { return m_pVramHeap->alloc(Size, Alignment); }
        void vramDealloc(void* Ptr) { m_pVramHeap->dealloc(Ptr); }

        // statistics
        size_t getTotalSize();
        size_t getAllocatedSize();
        size_t getFragmentSize();

        size_t getTotalSizeByPoolType(GRAPHIC_MEMORY_POOL_TYPE Type);
        size_t getAllocatedSizeByPoolType(GRAPHIC_MEMORY_POOL_TYPE Type);

        void dump();

        // debug
        bool checkIsOverlap();

        inline nvn::Device* getNVNDevice() { return m_pNVNDevice; }
        GRAPHIC_MEMORY_ERROR createNVNMemoryPool(size_t Size, size_t Alignment, GRAPHIC_MEMORY_POOL_TYPE Type, nvn::MemoryPool** ppMemoryPool, void** ppVramAddress);
        GRAPHIC_MEMORY_ERROR destroyNVNMemoryPool(nvn::MemoryPool* ppMemoryPool, void* ppVramAddress);

        inline void increaseBudget(GRAPHIC_MEMORY_RESOURCE_USAGE Usage, size_t Size) { m_Budgets[ENUM2INT(Usage)].Used += Size; }
        inline void decreaseBudget(GRAPHIC_MEMORY_RESOURCE_USAGE Usage, size_t Size) { HW_ASSERT(m_Budgets[ENUM2INT(Usage)].Used >= Size); m_Budgets[ENUM2INT(Usage)].Used -= Size; }
        
        static cGraphicMemoryAllocator* GetInstance() { return ms_Instance; }

    private:
        GRAPHIC_MEMORY_POOL_TYPE getPoolType(GRAPHIC_MEMORY_RESOURCE_USAGE ResourceUsage, size_t aquireSize, bool isAllowDedicated = true);
        void setupMemoryPoolBuilder(nvn::MemoryPoolBuilder* Builder, size_t Size, void* ppVramAddress, GRAPHIC_MEMORY_POOL_TYPE Type);

    private:
        bool m_pInitialized;
        cHeap* m_pWorkHeap;
        cHeap* m_pVramHeap;
        nvn::Device* m_pNVNDevice;
        cGraphicMemoryPool m_Pools[ENUM2INT(GRAPHIC_MEMORY_POOL_TYPE::COUNT)];
        cGraphicMemoryDedicatedPool m_DedicatedPool;
        GRAPHIC_MEMORY_BUDGET m_Budgets[ENUM2INT(GRAPHIC_MEMORY_RESOURCE_USAGE::COUNT)];
        
        static cGraphicMemoryAllocator* ms_Instance;
    };

    //==========================================================================
    //    cGraphicMemoryAllocation
    //==========================================================================
    class cGraphicMemoryAllocation
    {
    public:
        friend class cGraphicMemoryBlock;
        friend class cGraphicMemoryHeap;
        friend class cGraphicMemoryPool;
        friend class cGraphicMemoryDedicatedPool;
        friend class cGraphicMemoryAllocator;


        cGraphicMemoryAllocation();
        ~cGraphicMemoryAllocation();

        inline bool isValid() { return m_pHeap != nullptr && (m_PoolType == GRAPHIC_MEMORY_POOL_TYPE::DEDICATED || m_pBlock != nullptr); }

        inline nvn::MemoryPool* getNVNMemoryPool() { HW_ASSERT(isValid()); return m_pHeap->getNVNMemoryPool(); }
        inline nvn::BufferAddress getNVNBufferAddress() { HW_ASSERT(isValid()); return m_pHeap->getNVNMemoryPool()->GetBufferAddress() + getOffset(); }

        inline void* map() 
        {
            HW_ASSERT(isValid());
            return  reinterpret_cast<void*>(
            reinterpret_cast<intptr_t>(m_pHeap->getNVNMemoryPool()->Map()) + getOffset()
            ); 
        }

        inline void unmap() 
        {
            HW_ASSERT(isValid());
            m_pHeap->getNVNMemoryPool()->FlushMappedRange(getOffset(), m_PoolType == GRAPHIC_MEMORY_POOL_TYPE::DEDICATED ? m_pHeap->getTotalSize() : m_pBlock->getSize());
        }

        inline ptrdiff_t getOffset() 
        { 
            HW_ASSERT(isValid()); 
            return m_PoolType == GRAPHIC_MEMORY_POOL_TYPE::DEDICATED ? 0 : m_pBlock->getOffset();
        }

        void release();

    private:
        void create(cGraphicMemoryHeap* pHeap, cGraphicMemoryBlock* pBlock, GRAPHIC_MEMORY_POOL_TYPE PoolType, GRAPHIC_MEMORY_RESOURCE_USAGE Usage);

    private:
        cGraphicMemoryHeap* m_pHeap;
        cGraphicMemoryBlock* m_pBlock;
        GRAPHIC_MEMORY_POOL_TYPE m_PoolType;
        GRAPHIC_MEMORY_RESOURCE_USAGE m_ResourceUsage;
    };
};
#endif //_HW_GRAPHIC_MEMORY_ALLOCATOR_H_ 