extends RigidBody3D

@onready var camera = $Camera3D

@export var box_scene: PackedScene
@export var move_force: float = 1000.0
@export var turn_torque: float = 50.0
@export var spawn_distance: float = 3.0

# Called when the node enters the scene tree for the first time.
func _ready() -> void:
	camera.make_current()
	
func _input(event: InputEvent) -> void:
	if event is InputEventKey and event.pressed and not event.echo:
		if event.keycode == KEY_B:
			_spawn_box()
		
func _spawn_box() -> void:
	if not box_scene:
		return

	var instance: Node3D = box_scene.instantiate()
	var forward = -global_transform.basis.z
	var spawn_pos = global_transform.origin + forward * spawn_distance
	get_tree().current_scene.add_child(instance)
	instance.set_deferred("global_position", spawn_pos)


func _physics_process(delta: float) -> void:
	var forward_input := 0.0
	var turn_input := 0.0

	# W / S movement
	if Input.is_key_pressed(KEY_W):
		forward_input += 1.0
	if Input.is_key_pressed(KEY_S):
		forward_input -= 1.0

	# A / D turning
	if Input.is_key_pressed(KEY_A):
		turn_input += 1.0
	if Input.is_key_pressed(KEY_D):
		turn_input -= 1.0


	# Forward/back force
	if forward_input != 0.0:
		var forward_dir = -transform.basis.z
		apply_central_force(forward_dir * forward_input * move_force)

	# Rotation torque
	if turn_input != 0.0:
		apply_torque(Vector3.UP * turn_input * turn_torque)
