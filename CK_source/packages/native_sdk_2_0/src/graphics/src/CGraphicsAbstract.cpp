// CGraphicsAbstract.cpp - display program core shared by the renderers.
// gfx 2026-11-11: partial unit, only the self-contained functions are recovered so far.
#include "CGraphicsAbstract.h"
#include "NPMalloc.h"
#include "NPNew.h"
#include "CVertexBuffer.h"
#include "CGraphicsAbstract_PS_AquireVtxBuffer.i"

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
    const int* rightOperand = &condition.m_value;
    if (right != NULL) {
        rightOperand = &right->m_data[condition.m_wordIndex];
    }
    if ((unsigned int)condition.m_op > 3) {
        return m_results[index].m_false;
    }
    int rightValue = *rightOperand;
    switch (condition.m_op) {
    case 1:
        if (leftValue != rightValue) {
            return m_results[index].m_false;
        }
        break;
    case 2:
        if (leftValue <= rightValue) {
            return m_results[index].m_false;
        }
        break;
    case 3:
        if (leftValue >= rightValue) {
            return m_results[index].m_false;
        }
        break;
    }
    return m_results[index].m_true;
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
                    if (target->m_flag4 == 1) {
                        program.ExecuteSpecial(target);
                    } else if (target->m_flag4 == 2) {
                        program.ExecuteText(target);
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
CVertexBuffer* CGraphicsAbstract::DisplayProgram::PriorityStacks::AquireVtxBuffer(
    DisplayProgram& program, const int* levelSizes, int bufferKind, int vertexBytes, unsigned int texture,
    const CVertexAttribute::Decl* decl, int indexBytes, unsigned int indexType)
{
    return AquireVtxBufferInternal<true>(program, levelSizes, bufferKind, vertexBytes, texture, decl,
                                  indexBytes, indexType);
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
