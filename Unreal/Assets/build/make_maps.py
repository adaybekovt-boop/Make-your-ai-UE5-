"""Reproducible seamless material data, not photographs; normal maps use OpenGL +Y."""
from pathlib import Path
import numpy as np
from PIL import Image
out=Path(__file__).resolve().parents[1]/'textures';out.mkdir(exist_ok=True)
rng=np.random.default_rng(43017);N=512
y,x=np.mgrid[0:N,0:N]/N
for name,rough,strength in [('metal',.30,.11),('fabric',.76,.22),('concrete',.72,.20),('oak',.40,.16),('rubber',.63,.11),('skin',.52,.045),('leather',.44,.16)]:
 noise=np.zeros((N,N))
 for i in range(28):
  fx,fy=rng.integers(1,80,2);phase=rng.uniform(0,6.28);noise+=np.sin(2*np.pi*(fx*x+fy*y)+phase)/(i+4)
 noise/=max(abs(noise.min()),noise.max())
 if name=='fabric':h=.5*np.sin(x*2*np.pi*128)*np.cos(y*2*np.pi*128)+noise*.15
 elif name=='oak':h=np.sin((x*32+np.sin(y*2*np.pi)*.65+np.sin(y*6*np.pi)*.2)*2*np.pi)*.5+noise*.12
 elif name=='metal':h=np.sin(y*2*np.pi*170)*.3+noise*.15
 else:h=noise
 dx=(np.roll(h,-1,1)-np.roll(h,1,1))*strength;dy=(np.roll(h,-1,0)-np.roll(h,1,0))*strength
 normal=np.stack([-dx,-dy,np.ones_like(h)],2);normal/=np.linalg.norm(normal,axis=2,keepdims=True)
 Image.fromarray(np.uint8(np.clip(normal*.5+.5,0,1)*255)).save(out/f'T_{name}_Normal.png')
 Image.fromarray(np.uint8(np.clip(rough+h*.07,0,1)*255)).save(out/f'T_{name}_Roughness.png')
print('14 material maps written',out)
