import trimesh
import sys

"""
full object: mtl, texture (images), obj
generate material_0.mtl, material_0.png
# uv.v = 1 - uv.v
"""
input_filename = sys.argv[1]

scene = trimesh.load(input_filename, process=False)

if isinstance(scene, trimesh.Scene):
    meshes = []
    for name, mesh in scene.geometry.items():
        meshes.append(mesh)

    combined_mesh = trimesh.util.concatenate(meshes)

    # uv = combined_mesh.visual.uv.copy()
    # uv[:, 1] = 1 - uv[:, 1]
    # combined_mesh.visual.uv = uv
   
    combined_mesh.export(sys.argv[2])

else:
    # uv = scene.visual.uv.copy()
    # uv[:, 1] = 1 - uv[:, 1]
    # scene.visual.uv = uv
    scene.export(sys.argv[2])
