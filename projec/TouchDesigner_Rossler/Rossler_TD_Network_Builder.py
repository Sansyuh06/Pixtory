# Rossler Attractor - GPU Accelerated TouchDesigner Network Builder
# Paste into Text DAT in /project1, right-click -> Run Script

import math, random

# 1. Set global framerate to 120 FPS
project.frameRate = 120

root = op('/project1')

# Clean up
try:
    my_name = me.name
except:
    my_name = ''
for c in root.findChildren(depth=1):
    if c.name != my_name:
        try: c.destroy()
        except: pass

# ============================================================
# 1. GPU PARTICLE SIMULATION (GLSL TOP Feedback Loop)
# ============================================================
print("Step 1: Setting up GPU particle simulation...")

# Initial positions (Noise)
noise = root.create(noiseTOP, 'init_pos')
noise.nodeX, noise.nodeY = -400, 400
noise.par.resolutionw = 256
noise.par.resolutionh = 256
noise.par.monochrome = False
noise.par.amplitude = 25.0
# Ensure 32-bit float for accurate position coordinates
for name in noise.par.pixelformat.menuNames:
    if '32' in name and 'rgba' in name.lower():
        noise.par.pixelformat = name
        break

fb = root.create(feedbackTOP, 'fb_pos')
fb.nodeX, fb.nodeY = -200, 400
fb.inputConnectors[0].connect(noise.outputConnectors[0])

glsl = root.create(glslMultiTOP, 'glsl_rk4')
glsl.nodeX, glsl.nodeY = 0, 400
for name in glsl.par.pixelformat.menuNames:
    if '32' in name and 'rgba' in name.lower():
        glsl.par.pixelformat = name
        break

glsl_code = """
out vec4 fragColor;

uniform float uTime;

vec3 rossler_deriv(vec3 p) {
    float a = 0.2;
    float b = 0.2;
    float c = 5.7;
    return vec3(-p.y - p.z, p.x + a * p.y, b + p.z * (p.x - c));
}

vec3 thomas_deriv(vec3 p) {
    float b = 0.208186;
    float s = 5.0;
    vec3 ps = p / s;
    return vec3(
        sin(ps.y) - b * ps.x,
        sin(ps.z) - b * ps.y,
        sin(ps.x) - b * ps.z
    ) * s;
}

vec3 blended_deriv(vec3 p, float morph) {
    vec3 r = rossler_deriv(p);
    vec3 t = thomas_deriv(p) * 2.5; // Speed up Thomas to match visual flow
    return mix(r, t, morph);
}

void main()
{
    vec4 pos = texture(sTD2DInputs[0], vUV.st);
    vec3 p = pos.xyz;
    
    if (length(p) > 150.0 || length(p) < 0.1) {
        vec4 init = texture(sTD2DInputs[1], vUV.st);
        p = init.xyz;
    }
    
    // 40 second morph cycle (12s hold, 8s transition)
    float cycle = mod(uTime, 40.0);
    float morphBlend = 0.0;
    if (cycle < 12.0) morphBlend = 0.0;
    else if (cycle < 20.0) {
        float t = (cycle - 12.0) / 8.0;
        morphBlend = smoothstep(0.0, 1.0, smoothstep(0.0, 1.0, t));
    } else if (cycle < 32.0) morphBlend = 1.0;
    else {
        float t = (cycle - 32.0) / 8.0;
        morphBlend = 1.0 - smoothstep(0.0, 1.0, smoothstep(0.0, 1.0, t));
    }
    
    float dt = 0.012;
    vec3 k1 = blended_deriv(p, morphBlend);
    vec3 k2 = blended_deriv(p + 0.5 * dt * k1, morphBlend);
    vec3 k3 = blended_deriv(p + 0.5 * dt * k2, morphBlend);
    vec3 k4 = blended_deriv(p + dt * k3, morphBlend);
    
    vec3 p_next = p + (dt / 6.0) * (k1 + 2.0 * k2 + 2.0 * k3 + k4);
    fragColor = vec4(p_next, 1.0);
}
"""
glsl_text = root.create(textDAT, 'glsl_code')
glsl_text.nodeX, glsl_text.nodeY = 0, 550
glsl_text.text = glsl_code
glsl.par.pixeldat = glsl_text.path

glsl.par.uniname0 = 'uTime'
glsl.par.value0x.mode = ParMode.EXPRESSION
glsl.par.value0x.expr = "absTime.seconds"

glsl.inputConnectors[0].connect(fb.outputConnectors[0])
glsl.inputConnectors[1].connect(noise.outputConnectors[0]) # For respawning
fb.par.targettop = glsl.path

null_pos = root.create(nullTOP, 'pos_out')
null_pos.nodeX, null_pos.nodeY = 200, 400
null_pos.inputConnectors[0].connect(glsl.outputConnectors[0])

print("  OK: 65,536 GPU particles ready")

# ============================================================
# 2. COLOR & CINEMATIC LIGHTING
# ============================================================
print("Step 2: Setting up cinematic lighting and colors...")

# Generative vibrant colors mapping for instances
color_noise = root.create(noiseTOP, 'particle_color')
color_noise.nodeX, color_noise.nodeY = 200, 250
color_noise.par.resolutionw = 256
color_noise.par.resolutionh = 256
color_noise.par.monochrome = False
color_noise.par.tz = "absTime.seconds * 0.05"
color_noise.par.period = 0.8

level = root.create(levelTOP, 'color_level')
level.nodeX, level.nodeY = 350, 250
level.inputConnectors[0].connect(color_noise.outputConnectors[0])
level.par.brightness1 = 1.3
level.par.contrast = 1.5

# Premium Phong Material instead of flat Constant MAT
mat = root.create(phongMAT, 'mat_cinematic')
mat.nodeX, mat.nodeY = -200, 100
mat.par.diffuser = 0.9
mat.par.diffuseg = 0.9
mat.par.diffuseb = 0.9
mat.par.specularr = 1.0
mat.par.specularg = 1.0
mat.par.specularb = 1.0
mat.par.shininess = 50.0
# Beautiful rim lighting for depth
mat.par.rim = True
mat.par.rimcolorr = 1.0
mat.par.rimcolorg = 0.3
mat.par.rimcolorb = 0.1
mat.par.rimcenter = 0.3
mat.par.rimwidth = 0.7
mat.par.emitr = 0.1
mat.par.emitg = 0.1
mat.par.emitb = 0.1

print("  OK: Lighting material configured")

# ============================================================
# 3. ATTRACTOR GEOMETRY (TOP Instancing)
# ============================================================
print("Step 3: Creating attractor geometry...")
attr_geo = root.create(geometryCOMP, 'attractor')
attr_geo.nodeX, attr_geo.nodeY = 400, 400

for c in attr_geo.findChildren(depth=1):
    try: c.destroy()
    except: pass

tiny = attr_geo.create(sphereSOP, 'particle')
tiny.par.radx = 0.08
tiny.par.rady = 0.08
tiny.par.radz = 0.08
tiny.par.rows = 8
tiny.par.cols = 8

attr_geo.par.material = mat.path

# Use GPU TOP Instancing instead of CPU DAT
attr_geo.par.instancing = True
attr_geo.par.instanceop = null_pos.path
attr_geo.par.instancetx = 'r'
attr_geo.par.instancety = 'g'
attr_geo.par.instancetz = 'b'

# Apply dynamic generative colors
attr_geo.par.instancecolorop = level.path
attr_geo.par.instancer = 'r'
attr_geo.par.instanceg = 'g'
attr_geo.par.instanceb = 'b'

print("  OK: Geometry instancing setup")

# ============================================================
# 4. STARFIELD GEOMETRY
# ============================================================
print("Step 4: Creating starfield...")
star_table = root.create(tableDAT, 'star_data')
star_table.nodeX, star_table.nodeY = -400, 600
star_table.clear()
star_table.appendRow(['tx', 'ty', 'tz'])
rng = random.Random(42)
for i in range(800):
    star_table.appendRow([round(rng.uniform(-100,100),1),
                          round(rng.uniform(-100,100),1),
                          round(rng.uniform(-100,100),1)])

star_geo = root.create(geometryCOMP, 'starfield')
star_geo.nodeX, star_geo.nodeY = 400, 600
for c in star_geo.findChildren(depth=1):
    try: c.destroy()
    except: pass

star_tiny = star_geo.create(sphereSOP, 'particle')
star_tiny.par.radx = 0.15
star_tiny.par.rady = 0.15
star_tiny.par.radz = 0.15
star_tiny.par.rows = 4
star_tiny.par.cols = 4

star_mat = star_geo.create(constantMAT, 'mat_star')
star_mat.par.colorr = 0.8
star_mat.par.colorg = 0.9
star_mat.par.colorb = 1.0
star_geo.par.material = star_mat.path

star_geo.par.instancing = True
star_geo.par.instanceop = star_table.path
star_geo.par.instancetx = 'tx'
star_geo.par.instancety = 'ty'
star_geo.par.instancetz = 'tz'

# ============================================================
# 5. BLACK HOLE CORE
# ============================================================
print("Step 5: Creating black hole...")
bh_geo = root.create(geometryCOMP, 'blackhole')
bh_geo.nodeX, bh_geo.nodeY = 400, 200

for c in bh_geo.findChildren(depth=1):
    try: c.destroy()
    except: pass

bh_sphere = bh_geo.create(sphereSOP, 'core')
bh_sphere.par.radx = 5.0
bh_sphere.par.rady = 5.0
bh_sphere.par.radz = 5.0
bh_sphere.par.rows = 30
bh_sphere.par.cols = 30

bh_mat = bh_geo.create(constantMAT, 'mat_dark')
bh_mat.par.colorr = 0.0
bh_mat.par.colorg = 0.0
bh_mat.par.colorb = 0.0
bh_mat.par.alpha = 1.0
bh_geo.par.material = bh_mat.path
bh_geo.par.sx = 0.01
bh_geo.par.sy = 0.01
bh_geo.par.sz = 0.01

# ============================================================
# 6. CAMERA + DUAL LIGHTING
# ============================================================
print("Step 6: Creating camera and dual lights...")
cam = root.create(cameraCOMP, 'cam1')
cam.nodeX, cam.nodeY = 600, 400
cam.par.tz = 60.0
cam.par.rx = -25.0
cam.par.ry.mode = ParMode.EXPRESSION
cam.par.ry.expr = "absTime.seconds * 5.0"

# Main Light (Warm Orange)
light1 = root.create(lightCOMP, 'light_main')
light1.nodeX, light1.nodeY = 600, 200
light1.par.tx = 100
light1.par.ty = 100
light1.par.tz = 100
light1.par.colorr = 1.0
light1.par.colorg = 0.6
light1.par.colorb = 0.3
light1.par.dimmer = 1.2

# Fill Light (Cool Teal)
light2 = root.create(lightCOMP, 'light_fill')
light2.nodeX, light2.nodeY = 600, 100
light2.par.tx = -100
light2.par.ty = -50
light2.par.tz = -100
light2.par.colorr = 0.1
light2.par.colorg = 0.4
light2.par.colorb = 1.0
light2.par.dimmer = 0.8

# ============================================================
# 7. RENDER + POST FX
# ============================================================
print("Step 7: Creating render pipeline...")
render = root.create(renderTOP, 'render1')
render.nodeX, render.nodeY = 800, 400
render.par.resolutionw = 1280
render.par.resolutionh = 720
render.par.camera = cam.path
try:
    render.par.lights = "light_main light_fill"
except:
    try:
        render.par.light = "light_main light_fill"
    except: pass

blur = root.create(blurTOP, 'bloom_blur')
blur.nodeX, blur.nodeY = 1000, 300
blur.inputConnectors[0].connect(render.outputConnectors[0])
blur.par.size1 = 20.0
blur.par.size2 = 20.0

bloom = root.create(compositeTOP, 'bloom_add')
bloom.nodeX, bloom.nodeY = 1000, 400
bloom.par.operand = 'add'
bloom.inputConnectors[0].connect(render.outputConnectors[0])
bloom.inputConnectors[1].connect(blur.outputConnectors[0])

# ============================================================
# 8. TEXT OVERLAY
# ============================================================
print("Step 8: Creating overlay...")
title = root.create(textTOP, 'title_text')
title.nodeX, title.nodeY = 1000, 500
title.par.resolutionw = 1280
title.par.resolutionh = 720
title.par.bgcolorr = 0.0
title.par.bgcolorg = 0.0
title.par.bgcolorb = 0.0
title.par.bgalpha = 0.0
title.par.fontsizex = 20
title.par.text = ""
title.par.text.mode = ParMode.EXPRESSION
title.par.text.expr = "'THOMAS ATTRACTOR (GPU)\\n\\ndx/dt = sin(y) - bx\\ndy/dt = sin(z) - by\\ndz/dt = sin(x) - bz\\n\\nb = 0.208186' if (absTime.seconds % 40) > 16 and (absTime.seconds % 40) < 36 else 'ROSSLER ATTRACTOR (GPU)\\n\\ndx/dt = -y - z\\ndy/dt = x + ay\\ndz/dt = b + z(x - c)\\n\\na=0.2  b=0.2  c=5.7'"

final = root.create(compositeTOP, 'final_comp')
final.nodeX, final.nodeY = 1200, 400
final.par.operand = 'over'
final.inputConnectors[0].connect(title.outputConnectors[0])
final.inputConnectors[1].connect(bloom.outputConnectors[0])

# ============================================================
# 9. OUTPUT
# ============================================================
print("Step 9: Output connected...")
out = root.create(outTOP, 'out1')
out.nodeX, out.nodeY = 1400, 400
out.inputConnectors[0].connect(final.outputConnectors[0])

root.par.opviewer = out.path

print("")
print("=" * 60)
print("  GPU SIMULATION BUILD COMPLETE!")
print("  - 65,536 particles simulated via GLSL TOP Feedback")
print("  - Project running at 120 FPS")
print("  - Cinematic dual-lighting & phong materials applied")
print("=" * 60)
