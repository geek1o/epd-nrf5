#!/usr/bin/env python3
"""Install the tested Debian Arm GNU toolchain into a user-owned x86_64 Linux prefix."""
import argparse,hashlib,pathlib,subprocess,urllib.request
p=argparse.ArgumentParser();p.add_argument('--prefix',required=True);a=p.parse_args()
root=pathlib.Path(a.prefix).resolve();root.mkdir(parents=True,exist_ok=True)
packages=[
 ('g/gcc-arm-none-eabi/gcc-arm-none-eabi_12.2.rel1-1_amd64.deb','2bbd4f4145e9a3669f58ddab6a3190b768949f07cc444d23e98743d905f83e16'),
 ('b/binutils-arm-none-eabi/binutils-arm-none-eabi_2.40-2+18+b1_amd64.deb','c8f9da2a434366bfe5a66a8267cb3b1df028f1d95278715050c222b43e1c221c'),
 ('n/newlib/libnewlib-arm-none-eabi_3.3.0-1.3+deb12u1_all.deb','c80278e8332940eaf9738910258bb85b21c30aacf246a5e363fee230d3605e84'),
 ('n/newlib/libnewlib-dev_3.3.0-1.3+deb12u1_all.deb','ab5e6e80d66a6f2f8b9b22b9297154a676a5434ac82e48b88e8164548bc72115'),
]
for name,digest in packages:
 path=root/pathlib.Path(name).name
 if not path.exists(): urllib.request.urlretrieve('https://deb.debian.org/debian/pool/main/'+name,path)
 if hashlib.sha256(path.read_bytes()).hexdigest()!=digest: raise RuntimeError('SHA256 mismatch: '+str(path))
 subprocess.run(['dpkg-deb','-x',str(path),str(root)],check=True)
for name,target in [('include','../../include/newlib'),('lib','newlib')]:
 path=root/'usr/lib/arm-none-eabi'/name
 if not path.exists(): path.symlink_to(target)
subprocess.run([str(root/'usr/bin/arm-none-eabi-gcc'),'--version'],check=True)
print('ARM_GCC='+str(root/'usr/bin/arm-none-eabi-gcc'))
