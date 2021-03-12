//******************************************************************************
// @file HwcGraphicMemoryHeap.cpp
// @brief NVN Graphic Memory Allocator
// @author Hu Ke
//******************************************************************************
#include <Hw/Hw.h>
#include "HwGraphicMemoryAllocator.h"

#define KB(x) (x * 1024) 
#define MB(x) (x * 1024 * 1024) 

#define MAX_INTERNAL_DATA_COUNT 65535
#define GET_PREV_LEVEL_SIZE(v) (v >> 2)
#define AVERAGE_BLOCK_COUNT_IN_EACH_HEAP 128
#define MAX_DEFERRED_BLOCKS 32
#define GENERIC_HEAP_ID (1)
#define MINI_HEAP_ID (0)
#define DEDICATED_THRESHOLD (MB(16))

#if (HW_DEBUG)
#define ENABLE_MEMORY_POOL_DEBUG_NAME
#endif

using namespace Hw;

static const GRAPHIC_MEMORY_CONFIG scGraphicMemoryConfigs[ENUM2INT(GRAPHIC_MEMORY_POOL_TYPE::COUNT)] =
{
    //                      TotalSize       Alignment     MinimumBlockSize  AllowMultiLevelBlocks   MaxCleanUpFrameCount    AllowDedicated
    /*SHADER_CODE*/         { MB(8),        KB(4),        256,              false ,                 5,                      false           },
    /*COMMANDS*/            { MB(4),        KB(4),        16,               false ,                 5,                      false           },
    /*TEXTURE*/             { MB(16),       KB(4),        4096,             true ,                  5,                      true            },
    /*RENDER_TARGET*/       { MB(32),       KB(4),        4096,             false ,                 5,                      true            },
    /*BUFFER*/              { MB(16),       KB(4),        1024,             true ,                  5,                      true            },
    /*GEOMETRY*/            { MB(16),       KB(4),        256,              false ,                 5,                      true            },
    /*MISC*/                { MB(16),       KB(4),        256,              false ,                 5,                      false           },
};  
    
static const nvn::MemoryPoolFlags sNVNMemoryPoolFlags[ENUM2INT(GRAPHIC_MEMORY_POOL_TYPE::COUNT)] =
{
    //                     
    /*SHADER_CODE*/         nvn::MemoryPoolFlags::GPU_CACHED | nvn::MemoryPoolFlags::CPU_UNCACHED | nvn::MemoryPoolFlags::SHADER_CODE,
    /*COMMANDS*/            nvn::MemoryPoolFlags::GPU_UNCACHED | nvn::MemoryPoolFlags::CPU_UNCACHED ,
    /*TEXTURE*/             nvn::MemoryPoolFlags::GPU_CACHED | nvn::MemoryPoolFlags::CPU_UNCACHED ,
    /*RENDER_TARGET*/       nvn::MemoryPoolFlags::GPU_CACHED | nvn::MemoryPoolFlags::CPU_NO_ACCESS | nvn::MemoryPoolFlags::COMPRESSIBLE,
    /*BUFFER*/              nvn::MemoryPoolFlags::GPU_CACHED | nvn::MemoryPoolFlags::CPU_UNCACHED ,
    /*GEOMETRY*/            nvn::MemoryPoolFlags::GPU_CACHED | nvn::MemoryPoolFlags::CPU_UNCACHED ,
    /*MISC*/                nvn::MemoryPoolFlags::GPU_CACHED | nvn::MemoryPoolFlags::CPU_CACHED ,
};

//------------------------------------------------------------------------------
//! isPow2
//------------------------------------------------------------------------------
inline bool isPow2(size_t x)
{
    return (x & (x - 1)) == 0;
}

//------------------------------------------------------------------------------
//! isAligned
//------------------------------------------------------------------------------
inline bool isAligned(size_t x, size_t align)
{
    return (x & (align - 1)) == 0;
}

//------------------------------------------------------------------------------
//! alignUp
//------------------------------------------------------------------------------
inline size_t alignUp(size_t x, size_t align)
{
    return (x + align - 1) & ~(align - 1);
}

//------------------------------------------------------------------------------
//! nextPow2
//------------------------------------------------------------------------------
static inline uint64_t nextPow2(uint64_t v)
{
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v |= v >> 32;
    v++;
    return v;
}

const char* getMemoryPoolNameByType(GRAPHIC_MEMORY_POOL_TYPE Type)
{
    switch (Type)
    {
    case Hw::GRAPHIC_MEMORY_POOL_TYPE::SHADER_CODE:
        return "SHADER_CODE";
    case Hw::GRAPHIC_MEMORY_POOL_TYPE::COMMANDS:
        return "COMMANDS";
    case Hw::GRAPHIC_MEMORY_POOL_TYPE::TEXTURE:
        return "TEXTURE";
    case Hw::GRAPHIC_MEMORY_POOL_TYPE::RENDER_TARGET:
        return "RENDER_TARGET";
    case Hw::GRAPHIC_MEMORY_POOL_TYPE::BUFFER:
        return "BUFFER";
    case Hw::GRAPHIC_MEMORY_POOL_TYPE::GEOMETRY:
        return "GEOMETRY";
    case Hw::GRAPHIC_MEMORY_POOL_TYPE::MISC:
        return "MISC";
    default:
        HW_ASSERT(false);
        break;
    }

    return nullptr;
}

const char* getMemoryUsageNameByType(GRAPHIC_MEMORY_RESOURCE_USAGE Usage)
{
    switch (Usage)
    {
    case GRAPHIC_MEMORY_RESOURCE_USAGE::SHADER_CODE:
        return "SHADER_CODE";
    case GRAPHIC_MEMORY_RESOURCE_USAGE::COMMANDS:
        return "COMMANDS";
    case GRAPHIC_MEMORY_RESOURCE_USAGE::SAMPLED_TEXTURE:
        return "SAMPLED_TEXTURE";
    case GRAPHIC_MEMORY_RESOURCE_USAGE::STORAGE_TEXTURE:
        return "STORAGE_TEXTURE";
    case GRAPHIC_MEMORY_RESOURCE_USAGE::COLOR_TARGET:
        return "COLOR_TARGET";
    case GRAPHIC_MEMORY_RESOURCE_USAGE::DEPTH_STENCIL:
        return "DEPTH_STENCIL";
    case GRAPHIC_MEMORY_RESOURCE_USAGE::UNIFORM_BUFFER:
        return "UNIFORM_BUFFER";
    case GRAPHIC_MEMORY_RESOURCE_USAGE::STORAGE_BUFFER:
        return "STORAGE_BUFFER";
    case GRAPHIC_MEMORY_RESOURCE_USAGE::INDEX_BUFFER:
        return "INDEX_BUFFER";
    case GRAPHIC_MEMORY_RESOURCE_USAGE::VERTEX_BUFFER:
        return "VERTEX_BUFFER";
    case GRAPHIC_MEMORY_RESOURCE_USAGE::INDIRECT_BUFFER:
        return "INDIRECT_BUFFER";
    case GRAPHIC_MEMORY_RESOURCE_USAGE::QUERY_BUFFER:
        return "QUERY_BUFFER";
    default:
        HW_ASSERT(false);
        break;
    }

    return nullptr;
}


//------------------------------------------------------------------------------
//! cGraphicMemorySpinLock
//------------------------------------------------------------------------------
cGraphicMemorySpinLock::cGraphicMemorySpinLock()
    : m_Lock(false)
    , m_MaxTryCount(0)
{

}

//------------------------------------------------------------------------------
//! ~cGraphicMemorySpinLock
//------------------------------------------------------------------------------
cGraphicMemorySpinLock::~cGraphicMemorySpinLock()
{

}

//------------------------------------------------------------------------------
//! ~cGraphicMemorySpinLock
//------------------------------------------------------------------------------
void cGraphicMemorySpinLock::create(uint32_t MaxTryCount /*= 1000*/)
{
    m_MaxTryCount = MaxTryCount;
    m_Lock.store(false);
}

//------------------------------------------------------------------------------
//! lock
//------------------------------------------------------------------------------
void cGraphicMemorySpinLock::lock()
{
    bool state = false;
    int TryCount = 0;
    while (!m_Lock.compare_exchange_strong(state, true, std::memory_order::memory_order_seq_cst))
    {
        state = false;
        if (TryCount >= m_MaxTryCount)
        {
            nn::os::YieldThread();
            TryCount = 0;
        }
        ++TryCount;
    }
}

//------------------------------------------------------------------------------
//! unlock
//------------------------------------------------------------------------------
void cGraphicMemorySpinLock::unlock()
{
    HW_ASSERT(m_Lock.load() == true);
    bool state = true;
    m_Lock.compare_exchange_strong(state, false, std::memory_order::memory_order_seq_cst);
}

//------------------------------------------------------------------------------
//! tryLock
//------------------------------------------------------------------------------
bool cGraphicMemorySpinLock::tryLock()
{
    bool state = false;
    return m_Lock.compare_exchange_strong(state, true, std::memory_order::memory_order_seq_cst);
}

//------------------------------------------------------------------------------
//! isLocked
//------------------------------------------------------------------------------
bool cGraphicMemorySpinLock::isLocked()
{
    return (m_Lock.load() == true);
}


//------------------------------------------------------------------------------
//! cGraphicMemoryHeap
//------------------------------------------------------------------------------
cGraphicMemoryHeap::cGraphicMemoryHeap()
    : m_TotalSize(0)
    , m_AllocatedSize(0)
    , m_MinimumBlockSize(0)
    , m_FragmentSize(0)
    , m_SpinLock()
    , m_pOwnerPool(0)
    , m_BlockList()
    , m_FreeBlockLists()
{

}

//------------------------------------------------------------------------------
//! ~cGraphicMemoryHeap
//------------------------------------------------------------------------------
cGraphicMemoryHeap::~cGraphicMemoryHeap()
{

}

//------------------------------------------------------------------------------
//! cGraphicMemoryHeap::create
//------------------------------------------------------------------------------
GRAPHIC_MEMORY_ERROR cGraphicMemoryHeap::create(const GRAPHIC_MEMORY_CONFIG& Config, cIGraphicMemoryPool* pOwnerPool, nvn::MemoryPool* pNVNMemoryPool, void* pVramAddress, bool IsDedicated /*= false*/)
{
    m_TotalSize = Config.TotalSize;
    m_MinimumBlockSize = Config.MinimumBlockSize;
    m_AllocatedSize = 0;
    m_pOwnerPool = pOwnerPool;
    m_pMemoryPool = pNVNMemoryPool;
    m_pVramAddress = pVramAddress;
    m_UnusedFrameCount = 0;
    m_MaxCleanupFrameCount = 1;
    m_IsDedicated = IsDedicated;

    m_SpinLock.create();

    cGraphicMemoryBlock* pHeadBlock = cGraphicMemoryInternalDataAllocator<cGraphicMemoryBlock>::getInstance()->pop();
    pHeadBlock->create(0, GRAPHIC_MEMORY_BLOCK_TAG::FREE);

    cGraphicMemoryBlock* pTailBlock = cGraphicMemoryInternalDataAllocator<cGraphicMemoryBlock>::getInstance()->pop();
    pTailBlock->create(m_TotalSize, GRAPHIC_MEMORY_BLOCK_TAG::TAIL);

    m_BlockList.insert(pHeadBlock);
    m_BlockList.insertAfter(pHeadBlock, pTailBlock);

    addToFreeBlockList(pHeadBlock);



    return GRAPHIC_MEMORY_ERROR::NONE;
}

//------------------------------------------------------------------------------
//! cGraphicMemoryHeap::release
//------------------------------------------------------------------------------
GRAPHIC_MEMORY_ERROR cGraphicMemoryHeap::release()
{
    for (cGraphicMemoryBlock* pBlock : m_BlockList)
    {
        HW_ASSERT(pBlock->isFree() || pBlock->isTail());
        pBlock->release();
    }

    if (m_pMemoryPool != nullptr && m_pVramAddress != nullptr)
    {
        m_pOwnerPool->getAllocator()->destroyNVNMemoryPool(m_pMemoryPool, m_pVramAddress);
        m_pMemoryPool = nullptr;
        m_pVramAddress = nullptr;
    }

    return GRAPHIC_MEMORY_ERROR::NONE;
}

//------------------------------------------------------------------------------
//! cGraphicMemoryHeap::allocMemory
//------------------------------------------------------------------------------
GRAPHIC_MEMORY_ERROR cGraphicMemoryHeap::allocMemory(const cGraphicMemoryAllocationRequest& Reqest, cGraphicMemoryAllocation* pAllocation)
{
    HW_ASSERT(Reqest.pTargetHeap == this);
    HW_ASSERT(Reqest.pTargetBlock);
    HW_ASSERT(Reqest.pTargetBlock->isFree());
    HW_ASSERT(!m_IsDedicated);

    cGraphicMemoryBlock* pCurrBlock = Reqest.pTargetBlock;
    
    // check alignment
    if (!isAligned(pCurrBlock->getOffset(), Reqest.Alignment))
    {
        ptrdiff_t AlignedOffset = alignUp(pCurrBlock->getOffset(), Reqest.Alignment);
        size_t FragmentSize = AlignedOffset - pCurrBlock->getOffset();
        pCurrBlock->setOffset(AlignedOffset);
        if (pCurrBlock->getPrev())
        {
            if (pCurrBlock->getPrev()->isAllocated())
            {
                m_FragmentSize += FragmentSize;
                m_AllocatedSize += FragmentSize;
                pCurrBlock->getPrev()->setFragmentSize(FragmentSize);
            }
        }
    }

    // check split
    if (pCurrBlock->getSize() - Reqest.Size >= m_MinimumBlockSize)
    {
        cGraphicMemoryBlock* pSplitBlock = cGraphicMemoryInternalDataAllocator<cGraphicMemoryBlock>::getInstance()->pop();
        pSplitBlock->create(pCurrBlock->getOffset() + Reqest.Size, GRAPHIC_MEMORY_BLOCK_TAG::FREE);
        m_BlockList.insertAfter(pCurrBlock, pSplitBlock);

        addToFreeBlockList(pSplitBlock);
    }
    else if (pCurrBlock->getSize() > Reqest.Size)
    {
        size_t FragmentSize = pCurrBlock->getSize() - Reqest.Size;
        m_FragmentSize += FragmentSize;
        pCurrBlock->setFragmentSize(FragmentSize);
    }
    
    pCurrBlock->setTag(GRAPHIC_MEMORY_BLOCK_TAG::ALLOCATED);
    m_AllocatedSize += pCurrBlock->getSize();
    pAllocation->create(this, pCurrBlock, Reqest.PoolType, Reqest.ResourceUsage);

    m_pOwnerPool->getAllocator()->increaseBudget(Reqest.ResourceUsage, pCurrBlock->getSize());

    return GRAPHIC_MEMORY_ERROR::NONE;
}

//------------------------------------------------------------------------------
//! cGraphicMemoryHeap::freeMemory
//------------------------------------------------------------------------------
GRAPHIC_MEMORY_ERROR cGraphicMemoryHeap::freeMemory(cGraphicMemoryAllocation* pAllocation)
{
    if (!m_IsDedicated)
    {
        GRAPHIC_MEMORY_AUTO_LOCK(m_SpinLock);
        HW_ASSERT(pAllocation);
        HW_ASSERT(pAllocation->m_pBlock);
        HW_ASSERT(pAllocation->m_pBlock->isAllocated());
        cGraphicMemoryBlock* pCurrBlock = pAllocation->m_pBlock;
        HW_ASSERT(m_AllocatedSize >= pCurrBlock->getSize());
        m_AllocatedSize -= pCurrBlock->getSize();
        HW_ASSERT(m_FragmentSize >= pCurrBlock->getFragmentSize());
        m_pOwnerPool->getAllocator()->decreaseBudget(pAllocation->m_ResourceUsage, pAllocation->m_pBlock->getSize() - pCurrBlock->getFragmentSize());
        pCurrBlock->setFragmentSize(0);

        //merge prev free block
        {
            cGraphicMemoryBlock* pPrevBlock = pCurrBlock->getPrev();
            while (pPrevBlock && pPrevBlock->isFree())
            {
                removeFromFreeBlockList(pPrevBlock);
                m_BlockList.erase(pCurrBlock);
                pCurrBlock->release();
                cGraphicMemoryInternalDataAllocator<cGraphicMemoryBlock>::getInstance()->push(pCurrBlock);
                pCurrBlock = pPrevBlock;
                pPrevBlock = pCurrBlock->getPrev();
            }
        }

        // merge next free block
        {
            cGraphicMemoryBlock* pNextBlock = pCurrBlock->getNext();
            while (pNextBlock && pNextBlock->isFree())
            {
                removeFromFreeBlockList(pNextBlock);
                m_BlockList.erase(pNextBlock);
                pNextBlock->release();
                cGraphicMemoryInternalDataAllocator<cGraphicMemoryBlock>::getInstance()->push(pNextBlock);
                pNextBlock = pCurrBlock->getNext();
            }
        }

        pCurrBlock->setTag(GRAPHIC_MEMORY_BLOCK_TAG::FREE);
        addToFreeBlockList(pCurrBlock);
    }
    else
    {
        m_pOwnerPool->getAllocator()->decreaseBudget(pAllocation->m_ResourceUsage, pAllocation->m_pHeap->getTotalSize());
    }

    m_pOwnerPool->freeMemory(pAllocation);


    return GRAPHIC_MEMORY_ERROR::NONE;
}

//------------------------------------------------------------------------------
//! cGraphicMemoryHeap::findBlock
//------------------------------------------------------------------------------
cGraphicMemoryBlock* cGraphicMemoryHeap::findBlock(size_t Size, size_t Alignment)
{
    cGraphicMemoryBlock* pRet = nullptr;
    
    pRet = m_FreeBlockLists.find(
        [Size, Alignment](cGraphicMemoryBlock* pBlock)
        {
            HW_ASSERT(pBlock->isFree());
            if (pBlock->checkAllocableSize(Size, Alignment))
            {
                return true;
            }
            return false;
        }
    );

    if (pRet != nullptr)
    {
        removeFromFreeBlockList(pRet);
    }
    
    return pRet;
}

//------------------------------------------------------------------------------
//! cGraphicMemoryHeap::update
//------------------------------------------------------------------------------
GRAPHIC_MEMORY_ERROR cGraphicMemoryHeap::updateUnusedFrameCount()
{
    if (m_AllocatedSize == 0)
    {
        ++m_UnusedFrameCount;
    }
    else
    {
        m_UnusedFrameCount = 0;
    }
    return GRAPHIC_MEMORY_ERROR::NONE;
}

//------------------------------------------------------------------------------
//! cGraphicMemoryHeap::addToFreeBlockList
//------------------------------------------------------------------------------
void cGraphicMemoryHeap::addToFreeBlockList(cGraphicMemoryBlock* pBlock)
{
    m_FreeBlockLists.insert(pBlock);
}

//------------------------------------------------------------------------------
//! cGraphicMemoryHeap::removeFromFreeBlockList
//------------------------------------------------------------------------------
void cGraphicMemoryHeap::removeFromFreeBlockList(cGraphicMemoryBlock* pBlock)
{
    HW_ASSERT(m_FreeBlockLists.isExist(pBlock));
    m_FreeBlockLists.erase(pBlock);
}

//------------------------------------------------------------------------------
//! cGraphicMemoryHeap::checkOverlap
//------------------------------------------------------------------------------
bool cGraphicMemoryHeap::checkIsOverlap()
{
    GRAPHIC_MEMORY_AUTO_LOCK(m_SpinLock);
    for (cGraphicMemoryBlock* pBlock : m_BlockList)
    {
        if (pBlock && pBlock->getNext())
        {
            if (pBlock->getOffset() >= pBlock->getNext()->getOffset())
            {
                return true;
            }
        }
    }
    return false;
}

//------------------------------------------------------------------------------
//! cGraphicMemoryBlock
//------------------------------------------------------------------------------
cGraphicMemoryBlock::cGraphicMemoryBlock()
    : m_Tag(GRAPHIC_MEMORY_BLOCK_TAG::INVALID)
    , m_Offset(0)
    , m_FragmentSize(0)
{

}

//------------------------------------------------------------------------------
//! ~cGraphicMemoryBlock
//------------------------------------------------------------------------------
cGraphicMemoryBlock::~cGraphicMemoryBlock()
{

}

//------------------------------------------------------------------------------
//! ~cGraphicMemoryBlock::create
//------------------------------------------------------------------------------
void cGraphicMemoryBlock::create(ptrdiff_t Offset, GRAPHIC_MEMORY_BLOCK_TAG Tag)
{
    HW_ASSERT(Tag != GRAPHIC_MEMORY_BLOCK_TAG::INVALID);
    m_Tag = Tag;
    m_Offset = Offset;
}

//------------------------------------------------------------------------------
//! ~cGraphicMemoryBlock::release
//------------------------------------------------------------------------------
void cGraphicMemoryBlock::release()
{
    m_Tag = GRAPHIC_MEMORY_BLOCK_TAG::INVALID;
    m_Offset = 0;
}

//------------------------------------------------------------------------------
//! ~cGraphicMemoryBlock::getSize
//------------------------------------------------------------------------------
size_t cGraphicMemoryBlock::getSize()
{
    HW_ASSERT(!isInvalid());
    HW_ASSERT(!isTail());
    if (getNext())
    {
        return getNext()->getOffset() - m_Offset;
    }
    return 0;
}

//------------------------------------------------------------------------------
//! ~cGraphicMemoryBlock::checkAllocableSize
//------------------------------------------------------------------------------
bool cGraphicMemoryBlock::checkAllocableSize(size_t Size, size_t Alignment)
{
    bool Result = false;
    if (isAligned(getOffset(), Alignment))
    {
        Result = getSize() >= Size;
    }
    else
    {
        size_t extend = alignUp(getOffset(), Alignment) - getOffset();
        Result = getSize() >= (Size + extend);

    }

    return Result;
}

//------------------------------------------------------------------------------
//! cGraphicMemoryPool
//------------------------------------------------------------------------------
cGraphicMemoryPool::cGraphicMemoryPool()
    : cIGraphicMemoryPool()
    , m_HeapList(nullptr)
    , m_HeapCount(0)
    , m_Type(GRAPHIC_MEMORY_POOL_TYPE::NONE)
{

}

//------------------------------------------------------------------------------
//! ~cGraphicMemoryPool
//------------------------------------------------------------------------------
cGraphicMemoryPool::~cGraphicMemoryPool()
{

}

//------------------------------------------------------------------------------
//! ~cGraphicMemoryPool::create
//------------------------------------------------------------------------------
GRAPHIC_MEMORY_ERROR cGraphicMemoryPool::create(GRAPHIC_MEMORY_POOL_TYPE Type, cGraphicMemoryAllocator* pAllocator, bool EnableMultiLevelHeaps)
{
    m_Type = Type;
    m_pAllocator = pAllocator;
    m_Mutex.create();
    m_EnableMultiLevelHeaps = EnableMultiLevelHeaps;

    m_HeapList = (HeapLinkedList*)m_pAllocator->workAlloc(sizeof(HeapLinkedList) * getMaxLevelCount(), alignof(HeapLinkedList));
    for (int i = 0; i < getMaxLevelCount(); ++i)
    {
        new (&m_HeapList[i]) HeapLinkedList();
    }

    return GRAPHIC_MEMORY_ERROR::NONE;
}

//------------------------------------------------------------------------------
//! ~cGraphicMemoryPool::release
//------------------------------------------------------------------------------
GRAPHIC_MEMORY_ERROR cGraphicMemoryPool::release()
{

    for (int i = 0; i < getMaxLevelCount(); ++i)
    {
        for (cGraphicMemoryHeap* pHeap : m_HeapList[i])
        {
            pHeap->release();
        }
    }

    if (m_HeapList)
    {
        m_pAllocator->workDealloc(m_HeapList);
        m_HeapList = nullptr;
    }

    return GRAPHIC_MEMORY_ERROR::NONE;
}

//------------------------------------------------------------------------------
//! ~cGraphicMemoryPool::allocMemory
//------------------------------------------------------------------------------
GRAPHIC_MEMORY_ERROR cGraphicMemoryPool::allocMemory(GRAPHIC_MEMORY_RESOURCE_USAGE Usage, size_t Size, size_t Alignment, cGraphicMemoryAllocation* pAllocation)
{
    GRAPHIC_MEMORY_AUTO_LOCK(m_Mutex);

    GRAPHIC_MEMORY_ERROR error = GRAPHIC_MEMORY_ERROR::NONE;
    cGraphicMemoryAllocationRequest request = {};
    request.PoolType = m_Type;
    request.ResourceUsage = Usage;
    // create allocation request  
    while (true)
    {
        error = createAllocationRequest(request, Size, Alignment);
        if (error == GRAPHIC_MEMORY_ERROR::NO_ALLOCABLE_HEAP)
        {
            request.pTargetHeap = createHeap(getHeapLevel(Size + Alignment));
            if (request.pTargetHeap == nullptr)
            {
                return GRAPHIC_MEMORY_ERROR::OUT_OF_MEMORY;
            }
            request.pTargetHeap->lock();
            continue;
        }
        else if (error == GRAPHIC_MEMORY_ERROR::NONE)
        {
            break;
        }
        else
        {
            return error;
        }
    }

    // alloc from block
    {
        HW_ASSERT(error == GRAPHIC_MEMORY_ERROR::NONE);
        HW_ASSERT(request.pTargetHeap);
        HW_ASSERT(request.pTargetHeap->isLocked());
        HW_ASSERT(request.pTargetBlock);

        error = request.pTargetHeap->allocMemory(request, pAllocation);
        request.pTargetHeap->unlock();
    }
    return error;
}

//------------------------------------------------------------------------------
//! ~cGraphicMemoryPool::update
//------------------------------------------------------------------------------
GRAPHIC_MEMORY_ERROR cGraphicMemoryPool::shrink()
{
    GRAPHIC_MEMORY_AUTO_LOCK(m_Mutex);
    for (int i = 0; i < getMaxLevelCount(); ++i)
    {
        for (cGraphicMemoryHeap* pHeap : m_HeapList[i])
        {
            pHeap->updateUnusedFrameCount();
            if (pHeap->getUnusedFrameCount() >= pHeap->getCleanupFrameCount())
            {
                pHeap->release();
                m_HeapList[i].erase(pHeap);
                --m_HeapCount;
                break;
            }
        }
    }
    return GRAPHIC_MEMORY_ERROR::NONE;
}

//------------------------------------------------------------------------------
//! ~cGraphicMemoryPool::getTotalSize
//------------------------------------------------------------------------------
size_t cGraphicMemoryPool::getTotalSize()
{
    GRAPHIC_MEMORY_AUTO_LOCK(m_Mutex);
    size_t sum = 0;
    for (int i = 0; i < getMaxLevelCount(); ++i)
    {
        for (cGraphicMemoryHeap* pHeap : m_HeapList[i])
        {
            sum += pHeap->getTotalSize();
        }
    }
    return sum;
}

//------------------------------------------------------------------------------
//! ~cGraphicMemoryPool::getAllocatedSize
//------------------------------------------------------------------------------
size_t cGraphicMemoryPool::getAllocatedSize()
{ 
    GRAPHIC_MEMORY_AUTO_LOCK(m_Mutex);
    size_t sum = 0;
    for (int i = 0; i < getMaxLevelCount(); ++i)
    {
        for (cGraphicMemoryHeap* pHeap : m_HeapList[i])
        {
            sum += pHeap->getAllocatedSize();
        }
    }
    return sum;
}

//------------------------------------------------------------------------------
//! ~cGraphicMemoryPool::getFragmentSize
//------------------------------------------------------------------------------
size_t cGraphicMemoryPool::getFragmentSize()
{
    GRAPHIC_MEMORY_AUTO_LOCK(m_Mutex);
    size_t sum = 0;
    for (int i = 0; i < getMaxLevelCount(); ++i)
    {
        for (cGraphicMemoryHeap* pHeap : m_HeapList[i])
        {
            sum += pHeap->getFragmentSize();
        }
    }
    return sum;
}


//------------------------------------------------------------------------------
//! ~cGraphicMemoryPool::checkOverlap
//------------------------------------------------------------------------------
bool cGraphicMemoryPool::checkIsOverlap()
{
    GRAPHIC_MEMORY_AUTO_LOCK(m_Mutex);
    for (int i = 0; i < getMaxLevelCount(); ++i)
    {
        for (cGraphicMemoryHeap* pHeap : m_HeapList[i])
        {
            if (pHeap->checkIsOverlap())
            {
                return true;
            }
        }
    }
    return false;
}

//------------------------------------------------------------------------------
//! ~cGraphicMemoryPool::createHeap
//------------------------------------------------------------------------------
cGraphicMemoryHeap* cGraphicMemoryPool::createHeap(uint32_t Level)
{
    cGraphicMemoryHeap* pNewHeap = cGraphicMemoryInternalDataAllocator<cGraphicMemoryHeap>::getInstance()->pop();
    GRAPHIC_MEMORY_CONFIG config = scGraphicMemoryConfigs[ENUM2INT(m_Type)];
    if (m_EnableMultiLevelHeaps)
    {
        switch (Level)
        {
        case MINI_HEAP_ID:
        {
            config.TotalSize = GET_PREV_LEVEL_SIZE(config.TotalSize);
            config.MinimumBlockSize = GET_PREV_LEVEL_SIZE(config.MinimumBlockSize);
            break;
        }
        default:
            break;
        }
    }

    void* pVram = nullptr;
    nvn::MemoryPool* pMemoryPool = nullptr;

    GRAPHIC_MEMORY_ERROR error = m_pAllocator->createNVNMemoryPool(config.TotalSize, config.Alignment, m_Type, &pMemoryPool, &pVram);
    if (error != GRAPHIC_MEMORY_ERROR::NONE)
    {
        HW_ASSERT(false);
        return nullptr;
    }
    
    HW_ASSERT(pVram != nullptr);
    HW_ASSERT(pMemoryPool != nullptr);

    error = pNewHeap->create(config, this, pMemoryPool, pVram);
    HW_ASSERT(error == GRAPHIC_MEMORY_ERROR::NONE);
    m_HeapList[Level].insert(pNewHeap);

    ++m_HeapCount;

    return pNewHeap;
}

//------------------------------------------------------------------------------
//! ~cGraphicMemoryPool::createAllocationRequest
//------------------------------------------------------------------------------
GRAPHIC_MEMORY_ERROR cGraphicMemoryPool::createAllocationRequest(cGraphicMemoryAllocationRequest& Request, size_t Size, size_t Alignment)
{
    cGraphicMemoryHeap* DeferredCheckHeap[MAX_DEFERRED_BLOCKS];
    memset(DeferredCheckHeap, 0, sizeof(DeferredCheckHeap));
    uint32_t NumDeferredCheckHeap = 0;

    Request.Size = Size;
    Request.Alignment = Alignment;

    if (Request.pTargetHeap == nullptr)
    {
        Request.pTargetHeap = m_HeapList[getHeapLevel(Size + Alignment)].find(
            [Size, Alignment, &Request, &DeferredCheckHeap, &NumDeferredCheckHeap](cGraphicMemoryHeap* pHeap)
            {
                if (pHeap->tryLock())
                {
                    if (!pHeap->checkAllocableSize(Size))
                    {
                        pHeap->unlock();
                        return false;
                    }

                    Request.pTargetBlock = pHeap->findBlock(Size, Alignment);
                    if (Request.pTargetBlock == nullptr)
                    {
                        pHeap->unlock();
                        return false;
                    }
                    HW_ASSERT(Request.pTargetBlock->isFree());
                    return true;
                }
                else
                {
                    HW_ASSERT(NumDeferredCheckHeap < MAX_DEFERRED_BLOCKS);
                    DeferredCheckHeap[NumDeferredCheckHeap++] = pHeap;
                    return false;
                }
            }
        );
        

        if (Request.pTargetHeap != nullptr && Request.pTargetBlock != nullptr)
        {
            return GRAPHIC_MEMORY_ERROR::NONE;
        }
        else if (Request.pTargetHeap == nullptr && NumDeferredCheckHeap != 0)
        {
            for (int i = 0; i < NumDeferredCheckHeap; ++i)
            {
                DeferredCheckHeap[i]->lock();
                Request.pTargetBlock = DeferredCheckHeap[i]->findBlock(Size, Alignment);
                if (Request.pTargetBlock == nullptr)
                {
                    DeferredCheckHeap[i]->unlock();
                    continue;
                }
                else
                {
                    HW_ASSERT(Request.pTargetBlock->isFree());
                    Request.pTargetHeap = DeferredCheckHeap[i];
                    return GRAPHIC_MEMORY_ERROR::NONE;
                }
            }
            return GRAPHIC_MEMORY_ERROR::NO_ALLOCABLE_HEAP;
        }
        else
        {
            return GRAPHIC_MEMORY_ERROR::NO_ALLOCABLE_HEAP;
        }
    }
    else
    {
        HW_ASSERT(Request.pTargetHeap->isLocked());
        Request.pTargetBlock = Request.pTargetHeap->findBlock(Size, Alignment);
        if (Request.pTargetBlock == nullptr)
        {
            return GRAPHIC_MEMORY_ERROR::HEAP_INVALID;
        }
        else
        {
            HW_ASSERT(Request.pTargetBlock->isFree());
            return GRAPHIC_MEMORY_ERROR::NONE;
        }
    }

    HW_ASSERT(false);
    return GRAPHIC_MEMORY_ERROR::UNKOWN_ERROR;
}

//------------------------------------------------------------------------------
//! ~cGraphicMemoryPool::getHeapLevel
//------------------------------------------------------------------------------
uint32_t cGraphicMemoryPool::getHeapLevel(size_t Size)
{
    if (m_EnableMultiLevelHeaps)
    {
        const size_t MiddleLevelHeapSize = scGraphicMemoryConfigs[ENUM2INT(m_Type)].TotalSize;

        if (Size < (MiddleLevelHeapSize / AVERAGE_BLOCK_COUNT_IN_EACH_HEAP))
        {
            return MINI_HEAP_ID;
        }
        else
        {
            return GENERIC_HEAP_ID;
        }
    }

    return 0;
}

cGraphicMemoryDedicatedPool::cGraphicMemoryDedicatedPool()
    : cIGraphicMemoryPool()
    , m_Type(GRAPHIC_MEMORY_POOL_TYPE::NONE)
    , m_HeapList(nullptr)
    , m_HeapCount(0)
    , m_Mutex()
{

}

cGraphicMemoryDedicatedPool::~cGraphicMemoryDedicatedPool()
{

}

// generic
GRAPHIC_MEMORY_ERROR cGraphicMemoryDedicatedPool::create(cGraphicMemoryAllocator* pAllocator)
{
    m_Type = GRAPHIC_MEMORY_POOL_TYPE::DEDICATED;
    m_pAllocator = pAllocator;
    m_Mutex.create();

    m_HeapList = (HeapLinkedList*)m_pAllocator->workAlloc(sizeof(HeapLinkedList), alignof(HeapLinkedList));
    new (m_HeapList) HeapLinkedList();
    HW_ASSERT(m_HeapList);

    return GRAPHIC_MEMORY_ERROR::NONE;
}

GRAPHIC_MEMORY_ERROR cGraphicMemoryDedicatedPool::release()
{
    return GRAPHIC_MEMORY_ERROR::NONE;
}

GRAPHIC_MEMORY_ERROR cGraphicMemoryDedicatedPool::allocMemory(GRAPHIC_MEMORY_RESOURCE_USAGE Usage, size_t Size, size_t Alignment, cGraphicMemoryAllocation* pAllocation)
{
    cGraphicMemoryHeap* pNewHeap = cGraphicMemoryInternalDataAllocator<cGraphicMemoryHeap>::getInstance()->pop();
    void* pVram = nullptr;
    nvn::MemoryPool* pMemoryPool = nullptr;

    if (Alignment < NVN_MEMORY_POOL_STORAGE_ALIGNMENT)
    {
        Alignment = NVN_MEMORY_POOL_STORAGE_ALIGNMENT;
    }

    GRAPHIC_MEMORY_ERROR error = m_pAllocator->createNVNMemoryPool(Size, Alignment, m_pAllocator->getPoolType(Usage, (Size + Alignment), false), &pMemoryPool, &pVram);
    if (error != GRAPHIC_MEMORY_ERROR::NONE)
    {
        HW_ASSERT(false);
        return GRAPHIC_MEMORY_ERROR::CREATE_NVN_MEMORY_POOL_FAILED;
    }

    HW_ASSERT(pVram != nullptr);
    HW_ASSERT(pMemoryPool != nullptr);

    GRAPHIC_MEMORY_CONFIG config = {};
    config.TotalSize = Size;
    config.Alignment = Alignment;
    config.MinimumBlockSize = Size;

    error = pNewHeap->create(config, this, pMemoryPool, pVram, true);

    HW_ASSERT(error == GRAPHIC_MEMORY_ERROR::NONE);
    {
        GRAPHIC_MEMORY_AUTO_LOCK(m_Mutex);
        m_HeapList->insert(pNewHeap);
        ++m_HeapCount;
    }

    pAllocation->create(pNewHeap, nullptr, GRAPHIC_MEMORY_POOL_TYPE::DEDICATED, Usage);

    m_pAllocator->increaseBudget(Usage, pAllocation->m_pHeap->getTotalSize());

    return GRAPHIC_MEMORY_ERROR::NONE;
}

GRAPHIC_MEMORY_ERROR cGraphicMemoryDedicatedPool::freeMemory(cGraphicMemoryAllocation* pAllocation)
{
    HW_ASSERT(pAllocation);
    HW_ASSERT(pAllocation->isValid());
    HW_ASSERT(pAllocation->m_PoolType == GRAPHIC_MEMORY_POOL_TYPE::DEDICATED);

    pAllocation->m_pHeap->release();

    {
        GRAPHIC_MEMORY_AUTO_LOCK(m_Mutex);
        m_HeapList->erase(pAllocation->m_pHeap);
        --m_HeapCount;
    }

    return GRAPHIC_MEMORY_ERROR::NONE;
}

GRAPHIC_MEMORY_ERROR cGraphicMemoryDedicatedPool::update()
{
    return GRAPHIC_MEMORY_ERROR::NONE;
}

// statistics
size_t cGraphicMemoryDedicatedPool::getTotalSize()
{
    GRAPHIC_MEMORY_AUTO_LOCK(m_Mutex);
    size_t sum = 0;
    for (cGraphicMemoryHeap* pHeap : *m_HeapList)
    {
        sum += pHeap->getTotalSize();
    }
    return sum;
}

size_t cGraphicMemoryDedicatedPool::getAllocatedSize()
{
    GRAPHIC_MEMORY_AUTO_LOCK(m_Mutex);
    size_t sum = 0;
    for (cGraphicMemoryHeap* pHeap : *m_HeapList)
    {
        // all memory in dedicated pool will be used.
        sum += pHeap->getTotalSize();
    }
    return sum;
}

size_t cGraphicMemoryDedicatedPool::getFragmentSize()
{
    GRAPHIC_MEMORY_AUTO_LOCK(m_Mutex);
    size_t sum = 0;
    for (cGraphicMemoryHeap* pHeap : *m_HeapList)
    {
        sum += pHeap->getFragmentSize();
    }
    return sum;
}

//------------------------------------------------------------------------------
//! cGraphicMemoryAllocator
//------------------------------------------------------------------------------
cGraphicMemoryAllocator* cGraphicMemoryAllocator::ms_Instance = nullptr;

cGraphicMemoryAllocator::cGraphicMemoryAllocator()
    : m_pInitialized(false)
    , m_pWorkHeap(nullptr)
    , m_pVramHeap(nullptr)
{
    HW_ASSERT(!ms_Instance);
    ms_Instance = this;
}

//------------------------------------------------------------------------------
//! ~cGraphicMemoryAllocator
//------------------------------------------------------------------------------
cGraphicMemoryAllocator::~cGraphicMemoryAllocator()
{

}

//------------------------------------------------------------------------------
//! cGraphicMemoryAllocator::create
//------------------------------------------------------------------------------
GRAPHIC_MEMORY_ERROR cGraphicMemoryAllocator::create(const GRAPHIC_MEMORY_ALLOCATOR_INIT_INFO& Info)
{
    if (Info.pWorkHeap == nullptr || Info.pVramHeap == nullptr)
    {
        return GRAPHIC_MEMORY_ERROR::HW_HEAP_INVALID;
    }

    m_pWorkHeap = Info.pWorkHeap;
    m_pVramHeap = Info.pVramHeap;

    if (Info.pNVNDevice == nullptr)
    {
        return GRAPHIC_MEMORY_ERROR::NVN_DEVICE_INVAILD;
    }
    m_pNVNDevice = Info.pNVNDevice;

    {
        GRAPHIC_MEMORY_INTERNAL_DATA_ALLOCATOR_INIT_INFO InitInfo = {};
        InitInfo.Count = MAX_INTERNAL_DATA_COUNT;
        InitInfo.pWorkHeap = m_pWorkHeap;
        cGraphicMemoryInternalDataAllocator<cGraphicMemoryHeap>::createInstnace(InitInfo);
        cGraphicMemoryInternalDataAllocator<cGraphicMemoryBlock>::createInstnace(InitInfo);
    }

    for (int i = 0; i < ENUM2INT(GRAPHIC_MEMORY_POOL_TYPE::COUNT); ++i)
    {
        m_Pools[i].create(GRAPHIC_MEMORY_POOL_TYPE(i), this, scGraphicMemoryConfigs[i].AllowMultiLevelBlocks);
    }

    m_DedicatedPool.create(this);

    for (int i = 0; i < ENUM2INT(GRAPHIC_MEMORY_RESOURCE_USAGE::COUNT); ++i)
    {
        m_Budgets[i].Total = 0;
        m_Budgets[i].Used = 0;
    }


    m_pInitialized = true;
    return GRAPHIC_MEMORY_ERROR::NONE;
}

//------------------------------------------------------------------------------
//! cGraphicMemoryAllocator::release
//------------------------------------------------------------------------------
GRAPHIC_MEMORY_ERROR cGraphicMemoryAllocator::release()
{
    if (m_pInitialized)
    {
        for (int i = 0; i < ENUM2INT(GRAPHIC_MEMORY_POOL_TYPE::COUNT); ++i)
        {
            m_Pools[i].release();
        }

        cGraphicMemoryInternalDataAllocator<cGraphicMemoryHeap>::releaseInstance();
        cGraphicMemoryInternalDataAllocator<cGraphicMemoryBlock>::releaseInstance();

        m_pInitialized = false;
    }

    return GRAPHIC_MEMORY_ERROR::NONE;
}

//------------------------------------------------------------------------------
//! cGraphicMemoryAllocator::allocMemory
//------------------------------------------------------------------------------
GRAPHIC_MEMORY_ERROR cGraphicMemoryAllocator::allocMemory(GRAPHIC_MEMORY_RESOURCE_USAGE Usage, size_t Size, size_t Alignment, cGraphicMemoryAllocation* pAllocation)
{
    if (!m_pInitialized)
    {
        return GRAPHIC_MEMORY_ERROR::ALLOCATOR_IS_NOT_INITIALIZED;
    }

    if (Size == 0)
    {
        return GRAPHIC_MEMORY_ERROR::ALLOCATE_SIZE_ZERO_INVALID;
    }

    if (!isPow2(Alignment))
    {
        return GRAPHIC_MEMORY_ERROR::ALLOCATE_ALIGNMENT_INVALID;
    }

    GRAPHIC_MEMORY_POOL_TYPE poolType = getPoolType(Usage, (Size + Alignment));

    if (poolType == GRAPHIC_MEMORY_POOL_TYPE::NONE)
    {
        return GRAPHIC_MEMORY_ERROR::POOL_INVALID;
    }

    if (poolType == GRAPHIC_MEMORY_POOL_TYPE::DEDICATED)
    {
        return m_DedicatedPool.allocMemory(Usage, Size, Alignment, pAllocation);
    }
    else
    {
        if (Size > scGraphicMemoryConfigs[ENUM2INT(poolType)].TotalSize)
        {
            HW_ASSERT(false);
            return GRAPHIC_MEMORY_ERROR::ALLOCATE_SIZE_OVER_HEAP_SIZE;
        }

        return m_Pools[ENUM2INT(poolType)].allocMemory(Usage, Size, Alignment, pAllocation);
    }

    return GRAPHIC_MEMORY_ERROR::UNKOWN_ERROR;
}

//------------------------------------------------------------------------------
//! cGraphicMemoryAllocator::update
//------------------------------------------------------------------------------
GRAPHIC_MEMORY_ERROR cGraphicMemoryAllocator::beginRender()
{
    return GRAPHIC_MEMORY_ERROR::NONE;
}

//------------------------------------------------------------------------------
//! cGraphicMemoryAllocator::update
//------------------------------------------------------------------------------
GRAPHIC_MEMORY_ERROR cGraphicMemoryAllocator::endRender()
{
    for (int i = 0; i < ENUM2INT(GRAPHIC_MEMORY_POOL_TYPE::COUNT); ++i)
    {
        m_Pools[i].shrink();
    }
    return GRAPHIC_MEMORY_ERROR::NONE;
}


//------------------------------------------------------------------------------
//! cGraphicMemoryAllocator::getTotalSize
//------------------------------------------------------------------------------
size_t cGraphicMemoryAllocator::getTotalSize()
{
    size_t sum = 0;
    for (int i = 0; i < ENUM2INT(GRAPHIC_MEMORY_POOL_TYPE::COUNT); ++i)
    {
        sum += m_Pools[i].getTotalSize();
    }
    sum += m_DedicatedPool.getTotalSize();
    return sum;
}

//------------------------------------------------------------------------------
//! cGraphicMemoryAllocator::getAllocatedSize
//------------------------------------------------------------------------------
size_t cGraphicMemoryAllocator::getAllocatedSize()
{
    size_t sum = 0;
    for (int i = 0; i < ENUM2INT(GRAPHIC_MEMORY_POOL_TYPE::COUNT); ++i)
    {
        sum += m_Pools[i].getAllocatedSize();
    }
    sum += m_DedicatedPool.getAllocatedSize();
    return sum;
}

//------------------------------------------------------------------------------
//! cGraphicMemoryAllocator::getAllocatedSize
//------------------------------------------------------------------------------
size_t cGraphicMemoryAllocator::getFragmentSize()
{
    size_t sum = 0;
    for (int i = 0; i < ENUM2INT(GRAPHIC_MEMORY_POOL_TYPE::COUNT); ++i)
    {
        sum += m_Pools[i].getFragmentSize();
    }
    sum += m_DedicatedPool.getFragmentSize();
    return sum;
}



//------------------------------------------------------------------------------
//! cGraphicMemoryAllocator::checkOverlap
//------------------------------------------------------------------------------
bool cGraphicMemoryAllocator::checkIsOverlap()
{
    for (int i = 0; i < ENUM2INT(GRAPHIC_MEMORY_POOL_TYPE::COUNT); ++i)
    {
        if (m_Pools[i].checkIsOverlap())
        {
            return true;
        }
    }
    return false;
}

GRAPHIC_MEMORY_ERROR cGraphicMemoryAllocator::createNVNMemoryPool(size_t Size, size_t Alignment, GRAPHIC_MEMORY_POOL_TYPE Type, nvn::MemoryPool** ppMemoryPool, void** ppVramAddress)
{
    Size = GET_ALIGN(Size, NVN_MEMORY_POOL_STORAGE_GRANULARITY);
    *ppVramAddress = m_pVramHeap->alloc(Size, Alignment);
    if (*ppVramAddress == nullptr)
    {
        return GRAPHIC_MEMORY_ERROR::OUT_OF_MEMORY;
    }

    HW_ASSERT(*ppVramAddress != nullptr);

    nvn::MemoryPoolBuilder MemoryPoolBuilder;
    setupMemoryPoolBuilder(&MemoryPoolBuilder, Size, *ppVramAddress, Type);

    *ppMemoryPool = new (m_pWorkHeap->alloc(sizeof(nvn::MemoryPool), alignof(nvn::MemoryPool))) nvn::MemoryPool();
    HW_ASSERT(*ppMemoryPool != nullptr);

    if (!(*ppMemoryPool)->Initialize(&MemoryPoolBuilder))
    {
        return GRAPHIC_MEMORY_ERROR::CREATE_NVN_MEMORY_POOL_FAILED;
    }

#ifdef ENABLE_MEMORY_POOL_DEBUG_NAME
    (*ppMemoryPool)->SetDebugLabel(getMemoryPoolNameByType(Type));
#endif
    return GRAPHIC_MEMORY_ERROR::NONE;
}

GRAPHIC_MEMORY_ERROR cGraphicMemoryAllocator::destroyNVNMemoryPool(nvn::MemoryPool* pMemoryPool, void* pVramAddress)
{
    HW_ASSERT(pMemoryPool != nullptr);
    HW_ASSERT(pVramAddress != nullptr);

    pMemoryPool->Finalize();
    m_pWorkHeap->dealloc(pVramAddress);

    return GRAPHIC_MEMORY_ERROR::NONE;
}

//------------------------------------------------------------------------------
//! cGraphicMemoryAllocator::getPoolType
//------------------------------------------------------------------------------
GRAPHIC_MEMORY_POOL_TYPE cGraphicMemoryAllocator::getPoolType(GRAPHIC_MEMORY_RESOURCE_USAGE ResourceUsage, size_t aquireSize, bool isAllowDedicated/* = true*/)
{
    GRAPHIC_MEMORY_POOL_TYPE Type = GRAPHIC_MEMORY_POOL_TYPE::NONE;

    switch (ResourceUsage)
    {
    case Hw::GRAPHIC_MEMORY_RESOURCE_USAGE::SHADER_CODE:
        Type = GRAPHIC_MEMORY_POOL_TYPE::SHADER_CODE;
        break;
    case Hw::GRAPHIC_MEMORY_RESOURCE_USAGE::COMMANDS:
        Type = GRAPHIC_MEMORY_POOL_TYPE::COMMANDS;
        break;
    case Hw::GRAPHIC_MEMORY_RESOURCE_USAGE::SAMPLED_TEXTURE:
    case Hw::GRAPHIC_MEMORY_RESOURCE_USAGE::STORAGE_TEXTURE:
        Type = GRAPHIC_MEMORY_POOL_TYPE::TEXTURE;
        break;
    case Hw::GRAPHIC_MEMORY_RESOURCE_USAGE::COLOR_TARGET:
    case Hw::GRAPHIC_MEMORY_RESOURCE_USAGE::DEPTH_STENCIL:
        Type = GRAPHIC_MEMORY_POOL_TYPE::RENDER_TARGET;
        break;
    case Hw::GRAPHIC_MEMORY_RESOURCE_USAGE::UNIFORM_BUFFER:
    case Hw::GRAPHIC_MEMORY_RESOURCE_USAGE::STORAGE_BUFFER:
        Type = GRAPHIC_MEMORY_POOL_TYPE::BUFFER;
        break;
    case Hw::GRAPHIC_MEMORY_RESOURCE_USAGE::INDEX_BUFFER:
    case Hw::GRAPHIC_MEMORY_RESOURCE_USAGE::VERTEX_BUFFER:
        Type = GRAPHIC_MEMORY_POOL_TYPE::GEOMETRY;
        break;
    case Hw::GRAPHIC_MEMORY_RESOURCE_USAGE::INDIRECT_BUFFER:
    case Hw::GRAPHIC_MEMORY_RESOURCE_USAGE::QUERY_BUFFER:
        Type = GRAPHIC_MEMORY_POOL_TYPE::MISC;
        break;
    default:
        break;
    }

    if (isAllowDedicated && scGraphicMemoryConfigs[ENUM2INT(Type)].AllowDedicated && aquireSize >= DEDICATED_THRESHOLD)
    {
        Type = GRAPHIC_MEMORY_POOL_TYPE::DEDICATED;
    }

    HW_ASSERT(Type != GRAPHIC_MEMORY_POOL_TYPE::NONE);
    return Type;
}

//------------------------------------------------------------------------------
//! setupMemoryPoolBuilder
//------------------------------------------------------------------------------
void cGraphicMemoryAllocator::setupMemoryPoolBuilder(nvn::MemoryPoolBuilder* Builder, size_t Size, void* pVramAddress, GRAPHIC_MEMORY_POOL_TYPE Type)
{
    Builder->SetDefaults();
    Builder->SetDevice(m_pNVNDevice);
    Builder->SetStorage(pVramAddress, Size);
    Builder->SetFlags(sNVNMemoryPoolFlags[ENUM2INT(Type)]);
}

//------------------------------------------------------------------------------
//! getTotalSizeByPoolType
//------------------------------------------------------------------------------
size_t cGraphicMemoryAllocator::getTotalSizeByPoolType(GRAPHIC_MEMORY_POOL_TYPE Type)
{
    return m_Pools[ENUM2INT(Type)].getTotalSize();
}

//------------------------------------------------------------------------------
//! getAllocatedSizeByPoolType
//------------------------------------------------------------------------------
size_t cGraphicMemoryAllocator::getAllocatedSizeByPoolType(GRAPHIC_MEMORY_POOL_TYPE Type)
{
    return m_Pools[ENUM2INT(Type)].getAllocatedSize();
}

//------------------------------------------------------------------------------
//! dump
//------------------------------------------------------------------------------
void cGraphicMemoryAllocator::dump()
{
    LogMess("================ Graphic Memory Dump ================");
    float Total = 0;
    float Used = 0;
    float Fragment = 0;
    for (int i = 0; i < ENUM2INT(GRAPHIC_MEMORY_POOL_TYPE::COUNT); ++i)
    {
        LogMess("---- %s ----", getMemoryPoolNameByType(GRAPHIC_MEMORY_POOL_TYPE(i)));
        float PoolTotal = float(m_Pools[i].getTotalSize()) / 1024.0f / 1024.0f;
        float PoolUsed = float(m_Pools[i].getAllocatedSize()) / 1024.0f / 1024.0f;
        float PoolFragment = float(m_Pools[i].getFragmentSize()) / 1024.0f / 1024.0f;
        LogMess("Heap Count : %d", m_Pools[i].getHeapCount());
        LogMess("Total : %.2f MB", PoolTotal);
        LogMess("Used : %.2f MB", PoolUsed);
        LogMess("Fragment : %.2f MB", PoolFragment);
        if (Total != 0.0f)
        {
            LogMess("Rate : %.2f \%", PoolUsed / PoolTotal);
        }
        Total += PoolTotal;
        Used += PoolUsed;
        Fragment += PoolFragment;
    }
    {
        LogMess("---- DEDICATED ----");
        float PoolTotal = float(m_DedicatedPool.getTotalSize()) / 1024.0f / 1024.0f;
        float PoolUsed = float(m_DedicatedPool.getAllocatedSize()) / 1024.0f / 1024.0f;
        float PoolFragment = float(m_DedicatedPool.getFragmentSize()) / 1024.0f / 1024.0f;
        LogMess("Heap Count : %d", m_DedicatedPool.getHeapCount());
        LogMess("Total : %.2f MB", PoolTotal);
        LogMess("Used : %.2f MB", PoolUsed);
        LogMess("Fragment : %.2f MB", PoolFragment);
        if (Total != 0.0f)
        {
            LogMess("Rate : %.2f \%", PoolUsed / PoolTotal);
        }
        Total += PoolTotal;
        Used += PoolUsed;
        Fragment += PoolFragment;
    }

    LogMess("---- All ----");
    LogMess("Total : %.2f MB", Total);
    LogMess("Used : %.2f MB", Used);
    LogMess("Fragment : %.2f MB", Fragment);
    LogMess("=============== Graphic Memory Budget ===============");
    for (int i = 0; i < ENUM2INT(GRAPHIC_MEMORY_RESOURCE_USAGE::COUNT); ++i)
    {
        LogMess("---- %s ----", getMemoryUsageNameByType(GRAPHIC_MEMORY_RESOURCE_USAGE(i)));
        float BudgetTotal = float(m_Budgets[i].Total) / 1024.0f / 1024.0f;
        float BudgetUsed = float(m_Budgets[i].Used) / 1024.0f / 1024.0f;
        LogMess("Total : %.2f MB", BudgetTotal);
        LogMess("Used : %.2f MB", BudgetUsed);
    }
    
    LogMess("=====================================================");
}

//------------------------------------------------------------------------------
//! cGraphicMemoryAllocation
//------------------------------------------------------------------------------
cGraphicMemoryAllocation::cGraphicMemoryAllocation()
    : m_pHeap(nullptr)
    , m_pBlock(nullptr)
    , m_PoolType(GRAPHIC_MEMORY_POOL_TYPE::NONE)
    , m_ResourceUsage(GRAPHIC_MEMORY_RESOURCE_USAGE::NONE)
{

}

//------------------------------------------------------------------------------
//! ~cGraphicMemoryAllocation
//------------------------------------------------------------------------------
cGraphicMemoryAllocation::~cGraphicMemoryAllocation()
{

}

//------------------------------------------------------------------------------
//! cGraphicMemoryAllocation::create
//------------------------------------------------------------------------------
void cGraphicMemoryAllocation::create(cGraphicMemoryHeap* pHeap, cGraphicMemoryBlock* pBlock, GRAPHIC_MEMORY_POOL_TYPE PoolType, GRAPHIC_MEMORY_RESOURCE_USAGE Usage)
{
    m_pHeap = pHeap;
    m_pBlock = pBlock;
    m_PoolType = PoolType;
    m_ResourceUsage = Usage;
}

//------------------------------------------------------------------------------
//! cGraphicMemoryAllocation::release
//------------------------------------------------------------------------------
void cGraphicMemoryAllocation::release()
{
    if (m_pHeap != nullptr && m_pBlock != nullptr)
    {
        m_pHeap->freeMemory(this);
        m_pHeap = nullptr;
        m_pBlock = nullptr;
    }
}
