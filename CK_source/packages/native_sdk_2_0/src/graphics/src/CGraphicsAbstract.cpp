// CGraphicsAbstract.cpp - display program core shared by the renderers.
// gfx 2026-11-11: partial unit, only the self-contained functions are recovered so far.
#include "CGraphicsAbstract.h"
#include "NPMalloc.h"
#include "NPNew.h"
#include "CVertexBuffer.h"

void CGraphicsAbstract::DisplayProgram::DestroyCore()
{
    // objects owned by the program
    while (m_objects != NULL) {
        ObjectNode* node = m_objects;
        if (node->m_object != NULL) {
            delete node->m_object;
        }
        m_objects = node->m_next;
        np_free(node);
    }
    m_objects = NULL;

    // pools: the first block is embedded, the others are chained to it
    Pool* pool = m_pool.m_next;
    m_pool.Destroy();
    while (pool != NULL) {
        m_poolCursor = pool->m_next;
        pool->Destroy();
        np_free(pool);
        pool = m_poolCursor;
    }

    if (m_buffer.m_data != NULL) {
        np_free(m_buffer.m_data);
        m_buffer.m_data = NULL;
    }
    m_bufferSize = 0;
    m_buffer.m_pos = NULL;
    m_currPos = 0;
    m_poolCursor = NULL;
    m_poolPos = NULL;
    m_capacity1 = 0;
    m_mode1 = 0;
    m_mode2 = 0;
    m_field22 = 0;
    m_textCapacity = 0;
    m_specialCapacity = 0;
    m_layer = 0;
    m_firstComponent = NULL;
}

bool operator==(const CGraphicsAbstract::DisplayProgram::Component& left,
                const CGraphicsAbstract::DisplayProgram::Component& right)
{
    if ((left.m_flag4 | right.m_flag4) == 0) {
        return true;
    }
    if (left.m_flag4 != right.m_flag4) {
        return false;
    }
    if (left.m_flag4 == 1 && left.m_flag5 != right.m_flag5) {
        return false;
    }
    if (left.m_flag6 != right.m_flag6) {
        return false;
    }
    // m_flag6 is the size in bytes including the 8 byte header
    int wordCount = (unsigned int)(left.m_flag6 - 8) >> 2;
    switch (wordCount) {
    case 1:
        return left.m_data[0] == right.m_data[0];
    case 2:
        if (left.m_data[0] != right.m_data[0]) {
            return false;
        }
        return left.m_data[1] == right.m_data[1];
    default:
        for (int index = 0; index < wordCount; index++) {
            if (left.m_data[index] != right.m_data[index]) {
                return false;
            }
        }
        return true;
    }
}

void CGraphicsAbstract::DisplayProgram::Pool::Initialize(int count)
{
    Destroy();
    m_entries = (int*)np_malloc(count * sizeof(int));
    m_next = NULL;
    m_count = count;
}

void CGraphicsAbstract::DisplayProgram::Pool::Destroy()
{
    if (m_entries != NULL) {
        np_free(m_entries);
        m_entries = NULL;
    }
    m_next = NULL;
    m_count = 0;
}

void CGraphicsAbstract::DisplayProgram::PriorityStacks::ConditionalMgr::Destroy()
{
    if (m_results != NULL) {
        // the block starts 8 bytes before m_results
        np_free((unsigned char*)m_results - 8);
        m_results = NULL;
    }
    m_count = 0;
    if (m_values != NULL) {
        np_free(m_values);
        m_values = NULL;
    }
}

int CGraphicsAbstract::DisplayProgram::PriorityStacks::ConditionalMgr::GetResult(int index, const Component* left,
                                                                               const Component* right)
{
    if (left->m_flag4 == 0) {
        return m_results[index].m_true;
    }
    const Condition& condition = m_values[index];
    int leftValue = left->m_data[condition.m_wordIndex];
    const int* rightValue = &condition.m_value;
    if (right != NULL) {
        rightValue = &right->m_data[condition.m_wordIndex];
    }
    switch (condition.m_op) {
    case 0:
        return m_results[index].m_true;
    case 1:
        if (leftValue == *rightValue) {
            return m_results[index].m_true;
        }
        break;
    case 2:
        if (leftValue > *rightValue) {
            return m_results[index].m_true;
        }
        break;
    case 3:
        if (leftValue < *rightValue) {
            return m_results[index].m_true;
        }
        break;
    }
    return m_results[index].m_false;
}

void CGraphicsAbstract::DisplayProgram::PriorityStacks::Initialize(int stackCount, int stackDepth, int orderCount)
{
    Destroy();
    m_stacks = new TCStack<Component>[stackCount];
    for (int index = 0; index < stackCount; index++) {
        m_stacks[index].Initialize(stackDepth);
    }
    m_order.Initialize(orderCount);
    m_buffer14 = new int[stackCount];
    m_stackCount = stackCount;

    // conditionals: one per stack
    m_conditionals.Destroy();
    m_conditionals.m_values = new ConditionalMgr::Condition[stackCount];
    m_conditionals.m_results = new ConditionalMgr::Result[stackCount];
    m_conditionals.m_count = stackCount;
}

void CGraphicsAbstract::DisplayProgram::PriorityStacks::Destroy()
{
    m_conditionals.Destroy();
    if (m_buffer14 != NULL) {
        np_free(m_buffer14);
        m_buffer14 = NULL;
    }
    m_order.Destroy();
    if (m_stacks != NULL) {
        for (int index = 0; index < m_stackCount; index++) {
            m_stacks[index].Destroy();
        }
        if (m_stacks != NULL) {
            np_free(m_stacks);
        }
        m_stacks = NULL;
    }
    m_stackCount = 0;
}

void CGraphicsAbstract::DisplayProgram::PriorityStacks::Run(DisplayProgram& program)
{
    program.m_currPos = 0;
    m_order.m_data[m_order.m_top - 1] = NULL;
    for (int index = 0; index < m_stackCount; index++) {
        m_buffer14[index] = 0;
    }

    Component* component = program.m_firstComponent;
    int level = 0;
    for (;;) {
        if (component == NULL) {
            return;
        }
        if (level < m_stackCount) {
            // run the component and its followers, one level deeper each
            for (;;) {
                m_order.PushTop();
                m_order.m_data[m_order.m_top - 1] = component;
                int nextLevel = level + 1;
                Component* target = component;
                bool execute = true;
                if (component->m_flag4 == 0) {
                    if (m_buffer14[level] != 0) {
                        execute = false;
                    } else {
                        m_buffer14[level] = 1;
                        TCStack<Component>& stack = m_stacks[level];
                        target = &stack.m_data[stack.m_top - 1];
                    }
                } else {
                    m_buffer14[level] = 0;
                }
                if (execute) {
                    if (target->m_flag4 == 2) {
                        program.ExecuteText(target);
                    } else if (target->m_flag4 == 1) {
                        program.ExecuteSpecial(target);
                    }
                }
                level = nextLevel;
                if (level >= m_stackCount) {
                    break;
                }
                component = (Component*)((unsigned char*)component + component->m_flag6);
            }
        }
        // climb back to the first level with a next sibling
        int remaining = level + 1;
        do {
            remaining--;
            if (remaining == 0) {
                return;
            }
            m_order.m_top--;
            component = m_order.m_data[m_order.m_top]->m_next;
        } while (component == NULL);
        level = remaining - 1;
    }
}

/** Same kind, size, sub kind (kind 1) and data words (see operator==, without the both-empty case). */
static inline bool SameComponent(const CGraphicsAbstract::DisplayProgram::Component* top,
                          const CGraphicsAbstract::DisplayProgram::Component* other)
{
    if (top->m_flag4 != other->m_flag4) {
        return false;
    }
    if (top->m_flag6 != other->m_flag6) {
        return false;
    }
    if (top->m_flag4 == 1 && top->m_flag5 != other->m_flag5) {
        return false;
    }
    int wordCount = (unsigned int)(top->m_flag6 - 8) >> 2;
    for (int index = 0; index < wordCount; index++) {
        if (top->m_data[index] != other->m_data[index]) {
            return false;
        }
    }
    return true;
}

/** Bytes per index of an index type key (-1 for unknown types). */
static inline int IndexSizeOfType(unsigned int indexType)
{
    switch (indexType) {
    case 0x2f978:
    case 0x23f978:
        return 1;
    case 0x3f9726:
    case 0x13f9726:
        return 2;
    case 0x3f9702:
    case 0x13f9702:
        return 4;
    default:
        return -1;
    }
}

CVertexBuffer* CGraphicsAbstract::DisplayProgram::PriorityStacks::AquireVtxBuffer(
    DisplayProgram& program, const int* levelSizes, int bufferKind, int vertexBytes, unsigned int texture,
    const CVertexAttribute::Decl* decl, int indexBytes, unsigned int indexType)
{
    // 1. walk the recorded chains level by level, following the components equal to the stack tops
    Component* walk = program.m_firstComponent;
    Component* previous = NULL;         // last component looked at on the failing level
    Component* firstFalse = NULL;       // first sibling whose conditional is false
    Component* lastTrue = NULL;         // last sibling whose conditional is true
    int conditionResult = 1;
    int level = 0;
    while (level < m_stackCount) {
        previous = NULL;
        firstFalse = NULL;
        lastTrue = NULL;
        if (walk == NULL) {
            break;
        }
        TCStack<Component>& stack = m_stacks[level];
        Component* top = &stack.m_data[stack.m_top - 1];
        if ((top->m_flag4 == 0 && walk->m_flag4 == 0) || SameComponent(top, walk)) {
            int size = walk->m_flag6;
            conditionResult = m_conditionals.GetResult(level, walk, NULL);
            walk = (Component*)((unsigned char*)walk + size);
            level++;
            continue;
        }

        // look through the siblings of this level
        bool found = false;
        Component* candidate = walk;
        while (candidate != NULL) {
            previous = candidate;
            Component* other = NULL;
            if (top->m_flag4 != 0 && candidate->m_flag4 != 0) {
                other = candidate;
            }
            conditionResult = m_conditionals.GetResult(level, top, other);
            if (conditionResult == 1) {
                lastTrue = candidate;
            } else if (firstFalse == NULL) {
                firstFalse = candidate;
            }
            if (level == m_stackCount - 1) {
                // last level: the component holds a vertex buffer, reuse it when it has room
                CVertexBuffer* buffer = (CVertexBuffer*)candidate->m_data[0];
                unsigned int expectedType = CVertexBuffer::Type_Vertex;
                if (indexType != 0) {
                    expectedType = CVertexBuffer::Type_IndexVertex;
                }
                if ((unsigned int)(buffer->m_pool.m_cursor + vertexBytes) <=
                        (unsigned int)(buffer->m_pool.m_data + buffer->m_pool.m_capacity) &&
                    candidate->m_flag5 == (unsigned char)bufferKind &&
                    buffer->m_field4 == expectedType &&
                    (unsigned int)candidate->m_data[1] == texture) {
                    bool hasRoom = true;
                    if (indexType != 0) {
                        CIndexVertexBuffer* indexBuffer = (CIndexVertexBuffer*)buffer;
                        if ((unsigned int)(indexBuffer->m_indices.m_data + indexBuffer->m_indices.m_capacity) <
                            (unsigned int)(indexBuffer->m_indices.m_cursor + indexBytes)) {
                            hasRoom = false;
                        }
                    }
                    if (hasRoom) {
                        return buffer;
                    }
                }
            } else if ((top->m_flag4 == 0 && candidate->m_flag4 == 0) || SameComponent(top, candidate)) {
                walk = (Component*)((unsigned char*)candidate + candidate->m_flag6);
                found = true;
                break;
            }
            candidate = candidate->m_next;
        }
        if (!found) {
            break;
        }
        level++;
    }

    // 2. a new vertex buffer (with an index pool when indexed) in the program buffer
    int indexCapacity = 0;
    if (indexType != 0) {
        indexCapacity = program.m_specialCapacity;
        if (indexCapacity < indexBytes) {
            indexCapacity = indexBytes;
        }
    }
    int vertexCapacity = program.m_textCapacity;
    if (vertexCapacity < vertexBytes) {
        vertexCapacity = vertexBytes;
    }
    int attributeCount = 0;
    while (decl[attributeCount].m_type != 0) {
        attributeCount++;
    }
    size_t bufferSize = sizeof(CVertexBuffer);
    if (indexType != 0) {
        bufferSize = sizeof(CIndexVertexBuffer);
    }
    size_t attributeBytes = (attributeCount + 1) * sizeof(CVertexBuffer::Attribute);
    int alignedVertexBytes = vertexCapacity + ((0 - vertexCapacity) & 3);
    int alignedIndexBytes = indexCapacity + ((0 - indexCapacity) & 3);
    size_t requiredBytes = bufferSize + attributeBytes + alignedVertexBytes + alignedIndexBytes;
    unsigned char* initialPosition = program.m_buffer.m_pos;
    if (initialPosition == NULL ||
        requiredBytes > static_cast<size_t>(program.m_buffer.m_data + program.m_bufferSize - initialPosition)) {
        return NULL;
    }
    CVertexBuffer* buffer;
    if (indexCapacity == 0) {
        buffer = new (program.m_buffer.m_pos) CVertexBuffer(CVertexBuffer::Type_Vertex);
        program.m_buffer.m_pos += sizeof(CVertexBuffer);
    } else {
        buffer = new (program.m_buffer.m_pos) CIndexVertexBuffer();
        program.m_buffer.m_pos += sizeof(CIndexVertexBuffer);
    }
    buffer->m_attributeCount = attributeCount;
    buffer->m_attributes = (CVertexBuffer::Attribute*)program.m_buffer.m_pos;
    program.m_buffer.m_pos += attributeBytes;
    for (int index = 0; index < attributeCount; index++) {
        buffer->m_attributes[index] = *(const CVertexBuffer::Attribute*)&decl[index];
    }
    buffer->m_attributes[attributeCount].m_field8 = 0;

    // vertex bytes: in the program buffer when there is one
    unsigned char* position = program.m_buffer.m_pos;
    buffer->m_pool.Destroy();
    if (position == NULL) {
        buffer->m_pool.ResizeInternal(vertexCapacity);
    } else {
        buffer->m_pool.m_data = position;
        buffer->m_pool.m_cursor = position;
        buffer->m_pool.m_capacity = vertexCapacity;
    }
    buffer->m_pool.m_reserve = decl->m_stride;
    program.m_buffer.m_pos += alignedVertexBytes;

    if (buffer->m_field4 == CVertexBuffer::Type_IndexVertex) {
        CIndexVertexBuffer* indexBuffer = (CIndexVertexBuffer*)buffer;
        position = program.m_buffer.m_pos;
        indexBuffer->m_indices.Destroy();
        if (position == NULL) {
            indexBuffer->m_indices.ResizeInternal(indexCapacity);
        } else {
            indexBuffer->m_indices.m_data = position;
            indexBuffer->m_indices.m_cursor = position;
            indexBuffer->m_indices.m_capacity = indexCapacity;
        }
        indexBuffer->m_indices.m_reserve = IndexSizeOfType(indexType);
        indexBuffer->m_indexType = indexType;
        program.m_buffer.m_pos += alignedIndexBytes;
    }

    // 3. push the component of the new buffer on the last stack
    TCStack<Component>& lastStack = m_stacks[m_stackCount - 1];
    lastStack.PushTop();
    Component* bufferComponent = &lastStack.m_data[lastStack.m_top - 1];
    bufferComponent->m_flag4 = 1;
    bufferComponent->m_flag6 = (unsigned char)levelSizes[m_stackCount - 1];
    bufferComponent->m_flag5 = (unsigned char)bufferKind;
    bufferComponent->m_data[0] = (int)buffer;
    bufferComponent->m_data[1] = texture;

    // 4. record the stack tops from the failing level on as a new chain
    int startLevel = level;
    if (program.m_firstComponent == NULL) {
        startLevel = 0;
    }
    int wordCount = 0;
    if (startLevel < m_stackCount) {
        for (int index = startLevel; index < m_stackCount; index++) {
            TCStack<Component>& stack = m_stacks[index];
            wordCount += stack.m_data[stack.m_top - 1].m_flag6;
        }
        wordCount = wordCount >> 2;
    }
    int* destination = program.m_poolPos;
    if (program.m_poolCursor->m_entries + program.m_poolCursor->m_count < destination + wordCount) {
        program.AdjustCurrPos(wordCount);
        destination = program.m_poolPos;
        if (program.m_poolCursor->m_entries + program.m_poolCursor->m_count < destination + wordCount) {
            --lastStack.m_top;
            program.m_buffer.m_pos = initialPosition;
            return NULL;
        }
    }

    int* oldChain = (int*)firstFalse;
    if (firstFalse == NULL) {
        oldChain = (int*)walk;
    }
    int** writer;
    int* chainStart;
    int* nextChain;
    if ((firstFalse == NULL && conditionResult != 0) || oldChain == NULL) {
        // the new chain goes to the pool position
        writer = &program.m_poolPos;
        chainStart = destination;
        nextChain = oldChain;
    } else {
        // move the old chain to the pool position, the new chain takes its place
        chainStart = oldChain;
        nextChain = destination;
        int* copy = destination;
        for (int index = startLevel; index < m_stackCount; index++) {
            int size = ((Component*)oldChain)->m_flag6;
            if ((size >> 2) != 0) {
                int* end = copy + ((size & 0xfc) >> 2);
                do {
                    *copy = *oldChain;
                    oldChain++;
                    program.m_poolPos++;
                    copy = program.m_poolPos;
                } while (copy < end);
            }
        }
        writer = &oldChain;
    }

    int* output = chainStart;
    for (int index = startLevel; index < m_stackCount; index++) {
        TCStack<Component>& stack = m_stacks[index];
        Component* top = &stack.m_data[stack.m_top - 1];
        int* end;
        int skipWords;
        if (top->m_flag4 == 0) {
            // reference to the stack top: only the header is stored
            end = output + 2;
            skipWords = (top->m_flag6 >> 2) - 2;
        } else {
            skipWords = 0;
            end = output + ((top->m_flag6 & 0xfc) >> 2);
        }
        const int* source = (const int*)top;
        while (output < end) {
            *output = *source;
            (*writer)++;
            output = *writer;
            source++;
        }
        output = output + skipWords;
        *writer = output;
    }

    Component* newChain = (Component*)chainStart;
    if (nextChain == NULL) {
        newChain->m_next = NULL;
        program.m_firstComponent = newChain;
    } else if (firstFalse == NULL && conditionResult != 0) {
        newChain->m_next = NULL;
        if (lastTrue != NULL) {
            lastTrue->m_next = newChain;
        }
        if (previous != NULL) {
            previous->m_next = newChain;
        }
    } else {
        if (lastTrue != NULL) {
            lastTrue->m_next = newChain;
        }
        newChain->m_next = (Component*)nextChain;
    }

    lastStack.m_top--;
    return buffer;
}

void CGraphicsAbstract::DisplayProgram::AdjustCurrPos(int count)
{
    if (m_poolCursor->m_entries + m_poolCursor->m_count < m_poolPos + count) {
        if (m_capacity1 == 0) {
            return;
        }
        m_poolCursor->m_next = new Pool();
        m_poolCursor->m_next->Initialize(m_capacity1);
        // the next block grows (mode 1) or shrinks (mode 2)
        if (m_mode1) {
            m_capacity1 = m_capacity1 * 2;
        } else if (m_mode2) {
            m_capacity1 = m_capacity1 >> 1;
        }
        m_poolCursor = m_poolCursor->m_next;
        m_poolCursor->m_next = NULL;
        m_poolPos = m_poolCursor->m_entries;
    }
}

CGraphicsAbstract::DisplayProgram::DisplayProgram()
{
    m_currPos = 0;
    m_pool.m_next = NULL;
    m_pool.m_count = 0;
    m_pool.m_entries = NULL;
    m_poolCursor = NULL;
    m_poolPos = NULL;
    m_capacity1 = 0;
    m_mode1 = 0;
    m_mode2 = 0;
    m_field22 = 0;
    m_objects = NULL;
    m_layer = 0;
    m_firstComponent = NULL;
    m_bufferSize = 0;
}

void CGraphicsAbstract::DisplayProgram::InitializeCore(int capacity0, int capacity1, unsigned char mode1,
                                                       unsigned char mode2, int bufferSize)
{
    DestroyCore();
    unsigned char* buffer = (unsigned char*)np_malloc(bufferSize);
    m_bufferSize = bufferSize;
    m_buffer.m_data = buffer;
    m_buffer.m_pos = buffer;
    m_currPos = 0;
    m_pool.Initialize(capacity0);
    m_poolCursor = &m_pool;
    m_poolPos = m_pool.m_entries;
    m_capacity1 = capacity1;
    m_mode1 = mode1;
    m_mode2 = mode2;
    m_objects = NULL;
    m_layer = 0;
    m_firstComponent = NULL;
}

void CGraphicsAbstract::DisplayProgram::ClearCore()
{
    m_buffer.m_pos = m_buffer.m_data;

    int count = m_pool.m_count;
    Pool* pool = m_pool.m_next;
    m_pool.Destroy();
    while (pool != NULL) {
        m_poolCursor = pool->m_next;
        pool->Destroy();
        np_free(pool);
        pool = m_poolCursor;
    }
    m_pool.Initialize(count);
    m_currPos = 0;
    m_poolCursor = &m_pool;
    m_poolPos = m_pool.m_entries;
    m_objects = NULL;
    m_firstComponent = NULL;
}
