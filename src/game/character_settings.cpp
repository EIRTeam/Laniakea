#include "character_settings.h"

#include "bind_macros.h"

void CharacterSettings::_bind_methods() {
	MAKE_BIND_INT(CharacterSettings, max_health);
}
