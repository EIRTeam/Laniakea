#pragma once

#include "godot_cpp/classes/global_constants.hpp"
#include "godot_cpp/classes/ref_counted.hpp"
#include "godot_cpp/core/memory.hpp"
#include "godot_cpp/core/type_info.hpp"
#include "godot_cpp/variant/string.hpp"
#include "godot_cpp/variant/string_name.hpp"
#include "godot_cpp/variant/variant.hpp"
#include <functional>
#include <variant>

using namespace godot;

class ConsoleSystem;

class CVarProxy : public RefCounted {
    GDCLASS(CVarProxy, RefCounted);
    static void _bind_methods();
};

class CVar {
    using DefaultValueRaw = std::variant<const char*, int, float, bool, Vector3>;
    enum CVarFlags {
        FLAG_IS_COMMAND = 1
    };
public:
    struct DelayedPropertyInfo {
        int type;
        const char *name;
    };
private:
    // We are not allowed to initialize StringNames, Strings or Variants before the engine has initialized us, hence this hack.
    struct DelayedInitData {
        const char *cvar_name_cstr = nullptr;
        int type = GDEXTENSION_VARIANT_TYPE_NIL;
        DefaultValueRaw default_value_raw;
        const char *description_cstr = nullptr;
        int property_hint = PROPERTY_HINT_NONE;
        const char *property_hint_text_cstr = nullptr;
        BitField<CVarFlags> flags = 0;
        CVar *cvar = nullptr;
        std::vector<DelayedPropertyInfo> command_arguments;
    };

    static constexpr int DELAYED_INIT_CVAR_MAX = 64;
    
    static std::vector<DelayedInitData> &get_delayed_init_arr();

    struct CVarData {
        StringName cvar_name;
        String cvar_name_str;
        int type;
        Variant default_value;
        String description;
        int property_hint;
        String property_hint_text;
        BitField<CVarFlags> flags = 0;
        Variant current_value;
        Vector<PropertyInfo> command_arguments;
    };

    CVarData *cvar_data = nullptr;
    LocalVector<Callable> method_callbacks;


    void _delayed_init(const DelayedInitData &p_delayed_init);
    String _get_value_display_string(const Variant &p_variant) const;
    
    CVar(const DelayedInitData &p_delayed_init);

public:
    CVar(CVar &p_other) = delete;
    CVar(const CVar &&p_other) = delete;
    CVar &operator=(const CVar &p_other) = delete;
    CVar &operator=(CVar &&p_other) = delete;
    
    ~CVar();
    
    static CVar create_variable(const char *p_name, int p_type, DefaultValueRaw p_default, const char *p_description, int p_property_hint = PROPERTY_HINT_NONE, const char *p_property_hint_text = "");
    static CVar create_command(const char *p_name, const char *p_description, std::initializer_list<DelayedPropertyInfo> p_command_arguments = {});

    String get_cvar_name_string() const;
    StringName get_cvar_name() const;
    String get_value_display_string() const;
    String get_default_value_display_string() const;
    bool is_command() const;
    void execute_command();
    void execute_command_with_args(Vector<Variant> p_args);
    void connect_command_callback(Callable p_callable);
    void connect_cvar_changed_callback(Callable p_callable);
    void notify_cvar_changed();
    const Vector<PropertyInfo> &get_command_arguments() const;

    float get_float() const;
    bool get_bool() const;
    Vector3 get_vector3() const;

    friend class ConsoleSystem;
};