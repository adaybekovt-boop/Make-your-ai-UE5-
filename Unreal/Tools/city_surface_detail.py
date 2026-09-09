"""Distance-filtered world-space surface detail; no external texture dependency.

This is a new UE art treatment, not a numeric reproduction of Blender Noise/Bump.
World units are centimetres. Derivative filtering prevents subpixel mortar moire.
"""


def surface_profile(name):
    name = name.lower()
    if 'brick' in name:
        return (26., 9., .035, .22, 1.)
    if any(word in name for word in ('terracotta', 'slate')):
        return (32., 23., .025, .35, 1.)
    if any(word in name for word in ('stone', 'limestone')):
        return (95., 48., .012, .16, 0.)
    if 'asphalt' in name:
        return (0., 0., 0., .06, 0.)
    return None


def surface_code(profile):
    width, height, seam, relief, stagger = profile
    # Smooth analytic grain is band limited in screen space and disappears at
    # city overview distances. No time-varying noise / stochastic dither.
    code = '''
float3 n = abs(N);
float2 uv = n.z > max(n.x,n.y) ? P.xy : (n.x > n.y ? P.yz : P.xz);
float fade = 1.0-smoothstep(1600.0,5000.0,distance(P,Eye));
float pixel = max(length(ddx(uv)),length(ddy(uv)));
float grainFade = 1.0-smoothstep(0.4,2.0,pixel);
float grain = sin(uv.x*2.1)*sin(uv.y*1.7)*grainFade;
float joint=0.0;
float variation=0.0;
'''
    if width:
        code += f'''
float2 grid = uv/float2({width},{height});
grid.x += frac(floor(grid.y)*0.5)*{stagger};
float2 aa = max(fwidth(grid),float2(0.0001,0.0001));
float2 edge = min(frac(grid),1.0-frac(grid));
float2 lines = 1.0-smoothstep(float2({seam},{seam}),float2({seam},{seam})+aa,edge);
joint = max(lines.x,lines.y)*(1.0-smoothstep(0.12,0.35,max(aa.x,aa.y)));
variation = (frac(sin(dot(floor(grid),float2(12.9898,78.233)))*43758.5453)-0.5)*0.09;
variation *= 1.0-smoothstep(0.12,0.35,max(aa.x,aa.y));
'''
    code += f'''
return float3(1.0+fade*(variation-joint*0.18+grain*0.035),
              fade*(joint*0.08+grain*0.025),
              fade*(-joint*{relief}+grain*0.015));
'''
    return code


# Surface-gradient bump mapping (world space), see Epic's Bump Mapping Without
# Tangent Space. Degenerate/silhouette gradients retain the authored normal.
NORMAL_CODE = '''
float3 normal = normalize(N);
float3 dx = ddx(P), dy = ddy(P);
float3 rx = cross(dy,normal), ry = cross(normal,dx);
float det = dot(dx,rx);
float3 gradient = sign(det)*(ddx(H)*rx+ddy(H)*ry)/max(abs(det),0.00001);
gradient = gradient/(1.0+length(gradient));
return normalize(normal-gradient);
'''
