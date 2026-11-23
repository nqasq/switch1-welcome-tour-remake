# Switch 1 handheld model - original, built from scratch
# First version by Muse; fixed for 1:1 proportions (see git history).
#
# In Blender 3.0+: open the Scripting workspace, open this file, press Run Script.
# Headless build + export (what the repo uses):
#   blender -b --factory-startup --python tools/blender/switch1_handheld.py -- \
#       --export assets/models/switch1_handheld.glb
#
# Units are metres. Real Switch 1 (standard model, NOT OLED, NOT Lite, NOT Switch 2).
# Sizes come from Nintendo's official spec pages, see docs/modeling-reference.md:
#   whole handheld 239 x 102 x 13.9 mm, 6.2 inch screen
#   each Joy-Con 102 tall x 35.9 wide x 28.4 thick (thickness incl. stick and ZL/ZR)
# Button positions and the stick/trigger split of the 28.4 mm are our own estimates
# (marked APPROX below) until they are measured on the real console.
# All shapes and colours here are our own simple geometry - no Nintendo assets.

import math
import sys

import bpy
from mathutils import Vector

# --- clean scene -------------------------------------------------------------
bpy.ops.object.select_all(action='SELECT')
bpy.ops.object.delete(use_global=False)


def material(name, rgb, roughness=0.45, metallic=0.1):
    mat = bpy.data.materials.new(name=name)
    mat.use_nodes = True
    bsdf = mat.node_tree.nodes["Principled BSDF"]
    bsdf.inputs["Base Color"].default_value = (*rgb, 1.0)
    bsdf.inputs["Roughness"].default_value = roughness
    bsdf.inputs["Metallic"].default_value = metallic
    return mat


MAT_BODY = material("Body_DarkGrey", (0.08, 0.08, 0.09), 0.5, 0.2)
MAT_SCREEN = material("Screen_Black", (0.005, 0.005, 0.006), 0.15, 0.0)
MAT_BLUE = material("JoyCon_Blue", (0.0, 0.55, 0.85), 0.4, 0.05)
MAT_RED = material("JoyCon_Red", (0.95, 0.15, 0.12), 0.4, 0.05)
MAT_BLACK = material("Button_Black", (0.02, 0.02, 0.02), 0.35, 0.1)
MAT_GREY = material("Button_Grey", (0.35, 0.35, 0.36), 0.5, 0.1)


def box(name, loc, size, mat, bevel=0.002):
    bpy.ops.mesh.primitive_cube_add(size=1.0, location=loc)
    obj = bpy.context.active_object
    obj.name = name
    obj.dimensions = size
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    if bevel > 0:
        mod = obj.modifiers.new("SoftEdges", 'BEVEL')
        mod.width = bevel
        mod.segments = 2
    obj.data.materials.append(mat)
    return obj


def cylinder(name, loc, radius, depth, mat, rot=(math.pi / 2, 0, 0)):
    # Default rotation stands the cylinder along Y, i.e. sticking out of the front.
    bpy.ops.mesh.primitive_cylinder_add(radius=radius, depth=depth, location=loc, rotation=rot)
    obj = bpy.context.active_object
    obj.name = name
    obj.data.materials.append(mat)
    return obj


# --- overall sizes -----------------------------------------------------------
# X = width, Y = thickness, Z = height. Front faces -Y, Blender's Front view
# (numpad 1), so the left Joy-Con (-X) is on the viewer's left. The glTF export
# turns -Y into +Z, which faces the game's camera.
W_TOTAL, H, T = 0.239, 0.102, 0.0139   # official, with Joy-Con attached
JC_W = 0.0359                          # official Joy-Con width
TAB_W = W_TOTAL - 2 * JC_W             # 167.2 mm of tablet between the two Joy-Cons
FRONT = -T / 2                         # y of the front face
BACK = T / 2                           # y of the back face

# The Joy-Con is 28.4 mm thick from stick tip to the top of ZL/ZR.
# Split of that beyond the 13.9 mm body: APPROX.
STICK_RISE = 0.0075                    # stick cap top, in front of the front face
TRIGGER_DEPTH = 0.0284 - T - STICK_RISE  # ZL/ZR stick out behind the back (7.0 mm)

# --- main tablet body --------------------------------------------------------
box("Switch1_Body", (0, 0, 0), (TAB_W, T, H), MAT_BODY, bevel=0.002)

# --- screen, on the front face ----------------------------------------------
# 6.2 inch, 16:9 -> about 137.3 mm x 77.2 mm, with a black bezel around it
box("Switch1_Screen", (0, FRONT - 0.0006, 0.0), (0.1373, 0.0012, 0.0772), MAT_SCREEN, bevel=0.0)


def diamond(prefix, cx, cz, spacing, radius, mat, names):
    """Four round buttons in a diamond: names = (top, right, bottom, left)."""
    offsets = ((0, spacing), (spacing, 0), (0, -spacing), (-spacing, 0))
    for name, (dx, dz) in zip(names, offsets):
        cylinder(f"{prefix}_{name}", (cx + dx, FRONT - 0.0012, cz + dz), radius, 0.0024, mat)


def joycon(tag, side, mat):
    """side = -1 for the left Joy-Con, +1 for the right one."""
    x = side * (TAB_W / 2 + JC_W / 2)
    box(f"JoyCon_{tag}", (x, 0, 0), (JC_W, T, H), mat, bevel=0.004)

    # Positions on the Joy-Con face, APPROX. inner = toward the screen.
    inner = -side
    stick_r = 0.0085

    def stick(z):
        # Shaft plus cap, so the cap top sits STICK_RISE in front of the face.
        cylinder(f"Stick_{tag}_Shaft", (x, FRONT - 0.0025, z), 0.004, 0.005, MAT_BLACK)
        cylinder(f"Stick_{tag}_Cap", (x, FRONT - STICK_RISE + 0.00125, z), stick_r, 0.0025, MAT_BLACK)

    if side < 0:
        # Left: stick on top, D-pad diamond below it, minus top inner, capture below D-pad.
        stick(0.023)
        diamond("DPad", x, -0.013, 0.0085, 0.0034, MAT_GREY, ("Up", "Right", "Down", "Left"))
        cylinder("Minus", (x + inner * 0.010, FRONT - 0.001, 0.043), 0.0022, 0.002, MAT_BLACK)
        box("Capture", (x + inner * 0.004, FRONT - 0.001, -0.034), (0.0055, 0.002, 0.0055), MAT_BLACK, bevel=0.0008)
    else:
        # Right: ABXY diamond on top, stick below it, plus top inner, HOME below stick.
        diamond("Face", x, 0.023, 0.0085, 0.0034, MAT_GREY, ("X", "A", "B", "Y"))
        stick(-0.013)
        cylinder("Plus", (x + inner * 0.010, FRONT - 0.001, 0.043), 0.0025, 0.002, MAT_BLACK)
        cylinder("Home", (x + inner * 0.004, FRONT - 0.001, -0.034), 0.0032, 0.002, MAT_GREY)

    # L/R shoulder on the top edge, ZL/ZR trigger behind it on the back. APPROX sizes.
    shoulder = "L" if side < 0 else "R"
    box(f"Shoulder_{shoulder}", (x, -0.0015, H / 2 - 0.0005), (JC_W * 0.8, 0.008, 0.002), MAT_BLACK, bevel=0.001)
    box(f"Trigger_Z{shoulder}", (x, BACK + TRIGGER_DEPTH / 2, H / 2 - 0.012),
        (JC_W * 0.8, TRIGGER_DEPTH, 0.022), MAT_BLACK, bevel=0.002)


joycon("L", -1, MAT_BLUE)
joycon("R", 1, MAT_RED)

# --- camera and light, in front of the screen (front is -Y) ------------------
def look_at(obj, target=(0.0, 0.0, 0.0)):
    direction = Vector(target) - obj.location
    obj.rotation_euler = direction.to_track_quat('-Z', 'Y').to_euler()


bpy.ops.object.camera_add(location=(0.18, -0.38, 0.14))
cam = bpy.context.active_object
look_at(cam)
bpy.context.scene.camera = cam
bpy.ops.object.light_add(type='SUN', location=(0.3, -0.3, 0.5))
sun = bpy.context.active_object
sun.data.energy = 3.0
look_at(sun)
bpy.ops.object.light_add(type='AREA', location=(0, -0.4, 0.2))
area = bpy.context.active_object
area.data.energy = 20.0
area.data.shape = 'DISK'
area.data.size = 0.4
look_at(area)

print(f"Switch 1 handheld built: tablet {TAB_W * 1000:.1f} mm + 2 x {JC_W * 1000:.1f} mm Joy-Con = "
      f"{W_TOTAL * 1000:.1f} x {H * 1000:.0f} x {T * 1000:.1f} mm, max thickness "
      f"{(T + STICK_RISE + TRIGGER_DEPTH) * 1000:.1f} mm.")

# --- optional glTF export ----------------------------------------------------
# Pass `-- --export path/to/file.glb` on the command line, or set EXPORT_PATH here
# when running inside Blender's UI.
EXPORT_PATH = ""
argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
if "--export" in argv and argv.index("--export") + 1 < len(argv):
    EXPORT_PATH = argv[argv.index("--export") + 1]

if EXPORT_PATH:
    bpy.ops.export_scene.gltf(
        filepath=EXPORT_PATH,
        export_format='GLB',
        export_apply=True,       # bake the bevel modifiers into the mesh
        export_cameras=False,
        export_lights=False,
    )
    print(f"Exported {EXPORT_PATH}")
