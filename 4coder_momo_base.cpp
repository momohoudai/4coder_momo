//~ NOTE(Momo): Momo's stuff
// Defer 
namespace zawarudo {
    template<class F> struct ScopeGuard {
        F f;
        ~ScopeGuard() { f(); }
    };
    struct defer_dummy {};
    template<class F> ScopeGuard<F> operator+(defer_dummy, F f) {
        return { f };
    }
}
#define AnonVarSub(x) zawarudo_ryojianon##x
#define AnonVar(x) AnonVarSub(x)
#define Defer auto AnonVar(__COUNTER__) = zawarudo::defer_dummy{} + [&]()


static Face_ID global_styled_title_face = 0;
static Face_ID global_styled_label_face = 0;
static Face_ID global_small_code_face = 0;
static Rect_f32 global_cursor_rect = {0};
static Rect_f32 global_last_cursor_rect = {0};
static Rect_f32 global_mark_rect = {0};
static Rect_f32 global_last_mark_rect = {0};
static b32 global_dark_mode = 1;
static b32 global_battery_saver = 0;
static View_ID global_compilation_view = 0;
static b32 global_compilation_view_expanded = 0;
global Arena permanent_arena = {};

#define MemorySet                 memset
#define MemoryCopy                memcpy
#define CalculateCStringLength    strlen
#define S8Lit(s)                  string_u8_litexpr(s)


static struct
{
    String_Const_u8 string;
    ARGB_Color color;
}
global_tooltips[32] = {0};
static int global_tooltip_count = 0;
static Arena global_frame_arena;

