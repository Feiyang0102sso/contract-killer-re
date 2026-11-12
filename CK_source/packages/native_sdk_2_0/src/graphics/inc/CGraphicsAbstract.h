// CGraphicsAbstract.h - common CK graphics configuration, independent of OpenGL ES.
#ifndef CGRAPHICSABSTRACT_H
#define CGRAPHICSABSTRACT_H

#include "ICGraphics.h"
#include "ICGraphicsResource.h"
#include "TCStack.h"
#include "CVertexAttribute.h"
#include "NPMalloc.h"

class CEventListener;
class CVertexBuffer;

class CGraphicsAbstract : public ICGraphics {
public:
    CGraphicsAbstract();
    virtual ~CGraphicsAbstract() {}

    virtual void SetDefaultActiveAbstractionLayer(Abstraction abstraction)
    {
        if (abstraction == Abstraction_Hardware) {
            m_abstraction = Abstraction_Hardware;
        }
    }
    virtual Abstraction GetDefaultActiveAbstractionLayer() const { return m_abstraction; }
    virtual void SetDefaultColorFormat(ColorFormat format) { m_colorFormat = format; }
    virtual ColorFormat GetDefaultColorFormat() const { return m_colorFormat; }

    void ResetConfiguration();

    /**
     * Display program core shared by the 2D/3D renderers (CGraphicsAbstract.cpp).
     * Layout from the constructor and DestroyCore (gfx 2026-11-11, partial).
     */
    class DisplayProgram {
    public:
        /** One recorded command (0x20 bytes). */
        struct Component {
            Component()
            {
                m_flag4 = 0;
                m_flag5 = 0;
                m_flag6 = 0;
                m_next = NULL;
            }

            Component* m_next;          // 0x00 next sibling one level up (Run)
            unsigned char m_flag4;      // 0x04 kind: 0 reference to the stack top, 1 special, 2 text
            unsigned char m_flag5;      // 0x05
            unsigned char m_flag6;      // 0x06 size in bytes (the next component follows)
            unsigned char m_pad7;       // 0x07
            int m_data[6];              // 0x08
        };

        /** Block of int entries; extra blocks are chained through m_next. */
        class Pool {
        public:
            ~Pool() { Destroy(); }

            void Initialize(int count);
            void Destroy();

            Pool* m_next;       // 0x00
            int m_count;        // 0x04
            int* m_entries;     // 0x08 np_malloc'ed
        };

        class PriorityStacks {
        public:
            /** Results of conditional components. */
            class ConditionalMgr {
            public:
                /** Test of one conditional: compare a data word with a constant or another component. */
                struct Condition {
                    Condition() : m_op(0), m_wordIndex(0), m_value(0) {}

                    int m_op;           // 0x00 0: always, 1: ==, 2: >, 3: <
                    int m_wordIndex;    // 0x04 Component::m_data index
                    int m_value;        // 0x08 constant used without a second component
                };
                /** Values of one conditional for a true / false test. */
                struct Result {
                    Result() : m_true(1), m_false(1) {}
                    ~Result() {}

                    int m_true;         // 0x00
                    int m_false;        // 0x04
                };

                void Destroy();
                /** Value of conditional `index` for `left` (compared with `right`, or the constant). */
                int GetResult(int index, const Component* left, const Component* right);

                int m_count;            // 0x00
                Condition* m_values;    // 0x04 np_malloc'ed
                Result* m_results;      // 0x08 allocation starts 8 bytes before (header)
            };

            /** `stackCount` stacks of `stackDepth` components and an order stack of `orderCount`. */
            void Initialize(int stackCount, int stackDepth, int orderCount);
            /** Free every stack and the conditionals. */
            void Destroy();
            /** Walk the components of `program` level by level and execute them. */
            void Run(DisplayProgram& program);
            /**
             * Vertex buffer for a draw with the current state stacks: reuse the buffer of a matching
             * component chain with room left, or create one in the program buffer and record a new
             * component chain for it.
             */
            CVertexBuffer* AquireVtxBuffer(DisplayProgram& program, const int* levelSizes, int bufferKind,
                                           int vertexBytes, unsigned int texture,
                                           const CVertexAttribute::Decl* decl, int indexBytes,
                                           unsigned int indexType);

            int m_stackCount;                   // 0x00
            TCStack<Component>* m_stacks;       // 0x04 np_malloc'ed array of m_stackCount stacks
            TCStack<Component*> m_order;        // 0x08 components being walked
            int* m_buffer14;                    // 0x14 per level: the stack top was already run
            ConditionalMgr m_conditionals;      // 0x18
        };

        /** np_malloc'ed byte buffer with a write position. */
        struct ByteBuffer {
            ByteBuffer() : m_data(NULL), m_pos(NULL) {}
            ~ByteBuffer()
            {
                if (m_data != NULL) {
                    np_free(m_data);
                    m_data = NULL;
                }
            }

            unsigned char* m_data;      // 0x00
            unsigned char* m_pos;       // 0x04
            int m_field8;               // 0x08
        };

        /** Node of the list of objects owned by the program. */
        struct ObjectNode {
            ObjectNode* m_next;             // 0x00
            ICGraphicsResource* m_object;   // 0x04 deleted with the program
        };

        DisplayProgram();
        inline virtual ~DisplayProgram()
        {
            DestroyCore();
        }

        virtual void SetLayer(int layer) { m_layer = layer; }
        virtual int GetLayer() { return m_layer; }
        virtual void ExecuteText(Component* component) = 0;
        virtual void ExecuteSpecial(Component* component) = 0;
        // The zero word after ExecuteSpecial in the linked tables aligns the next table.

        /** Free the objects, the pools and the component stack. */
        void DestroyCore();
        void InitializeCore(int capacity0, int capacity1, unsigned char mode1, unsigned char mode2, int layers);
        /** Make room for `count` more pool entries, chaining a new block when the current one is full. */
        void AdjustCurrPos(int count);
        /** Forget the recorded components, keeping the first pool size and the buffer. */
        void ClearCore();

        int m_currPos;                  // 0x04
        Pool m_pool;                    // 0x08 first block
        Pool* m_poolCursor;             // 0x14 block being filled
        int* m_poolPos;                 // 0x18 next entry of m_poolCursor
        int m_capacity1;                // 0x1c
        unsigned char m_mode1;          // 0x20 CreationInfo mode 1
        unsigned char m_mode2;          // 0x21 CreationInfo mode 2
        unsigned char m_field22;        // 0x22
        unsigned short m_textCapacity;  // 0x24 set by the 2D Initialize (not by the constructor)
        unsigned short m_specialCapacity;   // 0x26
        ObjectNode* m_objects;          // 0x28
        int m_layer;                    // 0x2c
        Component* m_firstComponent;    // 0x30
        int m_bufferSize;               // 0x34
        ByteBuffer m_buffer;            // 0x38
    };

    bool m_initialized;          // +0x04
    bool m_softwareReady;        // +0x05
    bool m_hardwareReady;        // +0x06
    bool m_rendering;            // +0x07
    ColorFormat m_colorFormat;   // +0x08
    Abstraction m_abstraction;  // +0x0c
    CEventListener* m_listener;  // +0x10
    unsigned int m_colorBits;    // +0x14
    int m_scaleX;                // +0x18 16.16
    int m_scaleY;                // +0x1c 16.16
    unsigned int m_background;  // +0x20 packed ARGB
    bool m_softwareOnly;        // +0x24
    bool m_windowFlag;           // +0x25
    unsigned int m_samples;      // +0x28
    ICRenderSurface* m_window;   // +0x2c
    ICRenderSurface* m_target;   // +0x30
};

/** Same component: kinds (and the sub kind of kind 1), sizes and data words equal. */
bool operator==(const CGraphicsAbstract::DisplayProgram::Component& left,
                const CGraphicsAbstract::DisplayProgram::Component& right);

#endif
