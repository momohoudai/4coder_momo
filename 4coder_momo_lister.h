typedef i32 Momo_Lister_Activation_Code;
enum{
    Momo_ListerActivation_Finished = 0,
    Momo_ListerActivation_Continue = 1,
    Momo_ListerActivation_ContinueAndRefresh = 2,
};

typedef void Momo_Lister_Regenerate_List_Function_Type(Application_Links *app, struct Momo_Lister *lister);

struct Momo_Lister_Node{
    Momo_Lister_Node *next;
    Momo_Lister_Node *prev;
    String_Const_u8 string;
    union{
        String_Const_u8 status;
        i32 index;
    };
    void *user_data;
    i32 raw_index;
};

struct Momo_Lister_Node_List{
    Momo_Lister_Node *first;
    Momo_Lister_Node *last;
    i32 count;
};

struct Momo_Lister_Node_Ptr_Array{
    Momo_Lister_Node **node_ptrs;
    i32 count;
};

typedef Momo_Lister_Activation_Code Momo_Lister_Write_Character_Function(Application_Links *app);
typedef Momo_Lister_Activation_Code Momo_Lister_Key_Stroke_Function(Application_Links *app);
typedef void Momo_Lister_Navigate_Function(Application_Links *app,
                                      View_ID view, struct Momo_Lister *lister,
                                      i32 index_delta);

struct Momo_Lister_Handlers{
    Momo_Lister_Regenerate_List_Function_Type *refresh;
    Momo_Lister_Write_Character_Function *write_character;
    Custom_Command_Function *backspace;
    Momo_Lister_Navigate_Function *navigate;
    Momo_Lister_Key_Stroke_Function *key_stroke;
};

struct Momo_Lister_Result{
    b32 canceled;
    b32 activated_by_click;
    String_Const_u8 text_field;
    void *user_data;
};

struct Momo_Lister{
    Arena *arena;
    Temp_Memory restore_all_point;
    
    Momo_Lister_Handlers handlers;
    
    Mapping *mapping;
    Command_Map *map;
    
    u8 query_space[256];
    u8 text_field_space[256];
    u8 key_string_space[256];
    String_u8 query;
    String_u8 text_field;
    String_u8 key_string;
    
    Momo_Lister_Node_List options;
    Temp_Memory filter_restore_point;
    Momo_Lister_Node_Ptr_Array filtered;
    
    b32 set_vertical_focus_to_item;
    Momo_Lister_Node *highlighted_node;
    void *hot_user_data;
    i32 item_index;
    i32 raw_item_index;
    
    Basic_Scroll scroll;
    i32 visible_count;
    
    Momo_Lister_Result out;
    b32 is_insert_mode; // switches between 'input mode' and 'scrolling' mode
};

struct Momo_Lister_Prev_Current{
    Momo_Lister *prev;
    Momo_Lister *current;
};

struct Momo_Lister_Block{
    Application_Links *app;
    Momo_Lister_Prev_Current lister;
    Momo_Lister_Block(Application_Links *app, Arena *arena);
    ~Momo_Lister_Block();
    operator Momo_Lister *();
};

struct Momo_Lister_Prealloced_String{
    String_Const_u8 string;
};

struct Momo_Lister_Filtered{
    Momo_Lister_Node_Ptr_Array exact_matches;
    Momo_Lister_Node_Ptr_Array before_extension_matches;
    Momo_Lister_Node_Ptr_Array substring_matches;
};

////////////////////////////////

struct Momo_Lister_Choice{
    Momo_Lister_Choice *next;
    String_Const_u8 string;
    String_Const_u8 status;
    Key_Code key_code;
    union{
        u64 user_data;
        void *user_data_ptr;
    };
};

struct Momo_Lister_Choice_List{
    Momo_Lister_Choice *first;
    Momo_Lister_Choice *last;
};
