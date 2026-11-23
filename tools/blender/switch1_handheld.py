# Switch 1 handheld model - original, built from scratch
# Blender 3.0+ : open the Scripting workspace, open this file, press Run Script.
# Units are metres. Real Switch 1 (original, NOT OLED, NOT Switch 2):
# whole handheld 239mm wide x 102mm tall x 13.9mm thick, 6.2 inch screen.
# All shapes and colours here are our own simple geometry - no Nintendo assets.

import bpy
import math

# --- clean scene -------------------------------------------------------------
bpy.ops.object.select_all(action='SELECT')
bpy.ops.object.delete(use_global=False)
for coll in (bpy.data.meshes, bpy.data.curves, bpy.data.materials):
    pass  # keep it simple: deleted objects are enough for a fresh file

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
    bpy.ops.mesh.primitive_cylinder_add(radius=radius, depth=depth, location=loc, rotation=rot)
    obj = bpy.context.active_object
    obj.name = name
    obj.data.materials.append(mat)
    return obj

# --- main tablet body --------------------------------------------------------
# X = width, Y = thickness (front is +Y), Z = height
W, H, T = 0.239, 0.102, 0.0139
box("Switch1_Body", (0, 0, 0), (W, T, H), MAT_BODY, bevel=0.003)

# --- screen, on the front face ----------------------------------------------
# 6.2 inch, 16:9 -> about 137mm x 77mm, with a black bezel around it
box("Switch1_Screen", (0, T / 2 + 0.0006, 0.002), (0.137, 0.0012, 0.077), MAT_SCREEN, bevel=0.0)

# --- Joy-Cons, left blue, right red ------------------------------------------
# Each Joy-Con is about 35.5mm wide; they sit on the two ends of the body.
JC_W = 0.0355
for side, x, mat, tag in ((-1, -W / 2 + JC_W / 2, MAT_BLUE, "L"),
                         (1, W / 2 - JC_W / 2, MAT_RED, "R")):
    box(f"JoyCon_{tag}", (x, T / 2 + 0.0004, 0), (JC_W, 0.004, H - 0.002), mat, bevel=0.002)
    # analogue stick: small cylinder standing out of the front face
    stick_x = x + side * 0.002
    stick_z = 0.028 if side < 0 else -0.028
    cylinder(f"Stick_{tag}", (stick_x, T / 2 + 0.003, stick_z), 0.009, 0.004, MAT_BLACK)
    # 4 face buttons on the right, D-pad cross on the left (simple cylinders)
    for i, dz in enumerate((-0.012, 0.0, 0.012, 0.024)):
        bx = x - side * 0.008 if side > 0 else x + 0.008
        bz = 0.028 - dz if side > 0 else -0.028 + dz
        cylinder(f"Button_{tag}_{i}", (bx, T / 2 + 0.0028, bz), 0.0045, 0.002, MAT_GREY)
    # + / - button, small
    cylinder(f"PlusMinus_{tag}", (x + side * 0.008, T / 2 + 0.0028, 0.042 if side > 0 else -0.042),
             0.003, 0.002, MAT_BLACK)

# --- camera and light, so the first render is not black ----------------------
bpy.ops.object.camera_add(location=(0.25, -0.35, 0.18), rotation=(math.radians(75), 0, math.radians(35)))
bpy.context.scene.camera = bpy.context.active_object
bpy.ops.object.light_add(type='SUN', location=(0.3, -0.3, 0.5))
bpy.context.active_object.data.energy = 3.0
bpy.ops.object.light_add(type='AREA', location=(0, -0.4, 0.2), rotation=(math.radians(60), 0, 0))
bpy.context.active_object.data.energy = 200.0
bpy.context.active_object.data.shape = 'DISK'
bpy.context.active_object.data.size = 0.4

print("Switch 1 handheld built: body 239 x 102 x 13.9 mm, screen 6.2 inch, blue left / red right Joy-Con.")
