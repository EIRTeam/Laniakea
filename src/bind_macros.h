#pragma once

#include "godot_cpp/core/defs.hpp"

#define BIND_SETTER_GETTER(m_class, m_name) \
	ClassDB::bind_method(D_METHOD("set_" _STR(m_name), _STR(m_name)), &m_class::set_##m_name); \
	ClassDB::bind_method(D_METHOD("get_" _STR(m_name)), &m_class::get_##m_name);

#define MAKE_BIND_T(m_class, m_name, m_type, m_property_hint, m_hint_string) \
	BIND_SETTER_GETTER(m_class, m_name) \
	ADD_PROPERTY(PropertyInfo(m_type, _STR(m_name), m_property_hint, m_hint_string), "set_" _STR(m_name), "get_" _STR(m_name));

#define MAKE_BIND_STRING_NAME(m_class, m_name) MAKE_BIND_T(m_class, m_name, Variant::STRING_NAME, PROPERTY_HINT_NONE, "");
#define MAKE_BIND_FLOAT(m_class, m_name) MAKE_BIND_T(m_class, m_name, Variant::FLOAT, PROPERTY_HINT_NONE, "");
#define MAKE_BIND_VECTOR3_DEGREES(m_class, m_name) MAKE_BIND_T(m_class, m_name, Variant::VECTOR3, PROPERTY_HINT_NONE, "-360,360,0.1,or_less,or_greater,radians_as_degrees");
#define MAKE_BIND_VECTOR3(m_class, m_name) MAKE_BIND_T(m_class, m_name, Variant::VECTOR3, PROPERTY_HINT_NONE, "");
#define MAKE_BIND_INT(m_class, m_name) MAKE_BIND_T(m_class, m_name, Variant::INT, PROPERTY_HINT_NONE, "");
#define MAKE_BIND_BOOL(m_class, m_name) MAKE_BIND_T(m_class, m_name, Variant::BOOL, PROPERTY_HINT_NONE, "");
#define MAKE_BIND_RESOURCE(m_class, m_name, m_resource_type) MAKE_BIND_T(m_class, m_name, Variant::OBJECT, PROPERTY_HINT_RESOURCE_TYPE, _STR(m_resource_type));
#define MAKE_BIND_FLOAT32_ARRAY(m_class, m_name) MAKE_BIND_T(m_class, m_name, Variant::PACKED_FLOAT32_ARRAY, PROPERTY_HINT_NONE, "");
#define MAKE_BIND_NODE(m_class, m_name, m_node_type) MAKE_BIND_T(m_class, m_name, Variant::OBJECT, PROPERTY_HINT_NODE_TYPE, _STR(m_node_type));

#define MAKE_SETTER_GETTER_VALUE(m_type, m_name, m_target_var) \
	void set_##m_name(m_type p_##m_name) { \
		m_target_var = p_##m_name; \
	} \
	m_type get_##m_name() const { \
		return m_target_var; \
	}

#define MAKE_SETTER_GETTER_FLOAT_VALUE(m_name, m_target_var) \
	void set_##m_name(float p_##m_name) { \
		m_target_var = p_##m_name; \
	} \
	float get_##m_name() const { \
		return m_target_var; \
	}
