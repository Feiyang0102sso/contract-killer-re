// CGraphicsAbstract_PS_AquireVtxBuffer.i - shared display-program vertex buffer recording
// (original file name and place; CGraphics2d_Lite_OGLES.cpp includes it as ../../src/...).
#ifndef CGRAPHICSABSTRACT_PS_AQUIREVTXBUFFER_I
#define CGRAPHICSABSTRACT_PS_AQUIREVTXBUFFER_I

#include "CGraphicsAbstract.h"
#include "CVertexBuffer.h"
#include "NPNew.h"

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

template <bool ReserveTerminator>
inline __attribute__((always_inline)) CVertexBuffer*
CGraphicsAbstract::DisplayProgram::PriorityStacks::AquireVtxBufferInternal(
    DisplayProgram& program, const int* levelSizes, int bufferKind, int vertexBytes, unsigned int texture,
    const CVertexAttribute::Decl* decl, int indexBytes, unsigned int indexType)
{
    // 1. walk the recorded chains level by level, following the components equal to the stack tops
    Component* walk = program.m_firstComponent;
    CVertexBuffer* buffer = NULL;
    Component* previous = NULL;         // last component looked at on the failing level
    Component* firstFalse = NULL;       // first sibling whose conditional is false
    Component* lastTrue = NULL;         // last sibling whose conditional is true
    int conditionResult = 1;
    int level = 0;
    for (;;) {
        if (level >= m_stackCount) {
            return buffer;
        }
        previous = NULL;
        firstFalse = NULL;
        lastTrue = NULL;
        if (walk == NULL) {
            break;
        }
        TCStack<Component>& stack = m_stacks[level];
        Component* top = &stack.m_data[stack.m_top - 1];
        if (*top == *walk) {
            conditionResult = m_conditionals.GetResult(level, walk, NULL);
            walk = (Component*)((unsigned char*)walk + walk->m_flag6);
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
                buffer = (CVertexBuffer*)candidate->m_data[0];
                unsigned int expectedType = CVertexBuffer::Type_Vertex;
                if (indexType != 0) {
                    expectedType = CVertexBuffer::Type_IndexVertex;
                }
                if ((unsigned int)(buffer->m_pool.m_cursor + vertexBytes) <=
                        (unsigned int)(buffer->m_pool.m_data + buffer->m_pool.m_capacity) &&
                    candidate->m_flag5 == (unsigned char)bufferKind) {
                    bool hasRoom = true;
                    if (indexType != 0) {
                        CIndexVertexBuffer* indexBuffer = (CIndexVertexBuffer*)buffer;
                        if ((unsigned int)(indexBuffer->m_indices.m_data + indexBuffer->m_indices.m_capacity) <
                            (unsigned int)(indexBuffer->m_indices.m_cursor + indexBytes)) {
                            hasRoom = false;
                        }
                    }
                    if (hasRoom && buffer->m_field4 == expectedType &&
                        (unsigned int)candidate->m_data[1] == texture) {
                        return buffer;
                    }
                }
            } else if (*top == *candidate) {
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
    }
    int vertexCapacity = program.m_textCapacity;
    int attributeCount = 0;
    while (decl[attributeCount].m_type != 0) {
        attributeCount++;
    }
    size_t attributeBytes = attributeCount * sizeof(CVertexBuffer::Attribute);
    if (ReserveTerminator) {
        attributeBytes += sizeof(CVertexBuffer::Attribute);
    }
    int alignedVertexBytes = vertexCapacity + ((0 - vertexCapacity) & 3);
    int alignedIndexBytes = indexCapacity + ((0 - indexCapacity) & 3);
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
    const CVertexBuffer::Attribute* attributes = reinterpret_cast<const CVertexBuffer::Attribute*>(decl);
    switch (attributeCount) {
    case 1:
        buffer->m_attributes[0] = attributes[0];
        buffer->m_attributes[1].m_field8 = 0;
        break;
    case 2:
        buffer->m_attributes[0] = attributes[0];
        buffer->m_attributes[1] = attributes[1];
        buffer->m_attributes[2].m_field8 = 0;
        break;
    default:
        for (int index = 0; index < attributeCount; index++) {
            buffer->m_attributes[index] = attributes[index];
        }
        buffer->m_attributes[attributeCount].m_field8 = 0;
        break;
    }

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
        // Advance through the old chain as each component is moved.
        int* read = oldChain;
        for (int index = startLevel; index < m_stackCount; index++) {
            int size = ((Component*)read)->m_flag6;
            if ((size >> 2) != 0) {
                int* end = copy + ((size & 0xfc) >> 2);
                do {
                    *copy = *read;
                    read++;
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

#endif
