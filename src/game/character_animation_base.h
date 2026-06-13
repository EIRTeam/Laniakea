#pragma once

#include "godot_cpp/classes/object.hpp"
#include "godot_cpp/classes/ref.hpp"
#include "godot_cpp/classes/static_body3d.hpp"
#include "movement_shared.h"

using namespace godot;

class CharacterModel;
class MovementSettings;
class BaseCharacter;
class CharacterAnimationBase;

class AnimationSequenceFuture : public RefCounted {
	GDCLASS(AnimationSequenceFuture, RefCounted);

public:
	enum AnimationSequenceFutureStatus {
		PLAYING,
		ABORTED,
		FINISHED
	};
	AnimationSequenceFutureStatus status = PLAYING;
	double duration = 0.0;
	double position = 0.0;
	static void _bind_methods() {}
};

class CharacterAnimationBase : public Object {
	GDCLASS(CharacterAnimationBase, Object);

public:
	enum CharacterAnimationSequence {
		SEQUENCE_RELOAD,
		ANIMATION_SEQUENCE_MAX
	};
	static void _bind_methods();
	virtual void initialize(Ref<MovementSettings> p_movement_settings, CharacterModel *p_model) = 0;
	virtual void physics_update(BaseCharacter *p_movement, float p_delta) = 0;
	virtual void update(Movement::MovementSpeed p_desired_movement_speed, float p_delta) = 0;
	virtual Ref<AnimationSequenceFuture> trigger_sequence(const CharacterAnimationSequence p_sequence) = 0;
	virtual void abort_sequence(Ref<AnimationSequenceFuture> p_animation_sequence) = 0;
};
