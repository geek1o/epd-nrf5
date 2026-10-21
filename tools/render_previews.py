#!/usr/bin/env python3
"""Real paged firmware rendering under ASan/UBSan; compare MCU heap sizes byte-for-byte."""
import datetime as dt, os, pathlib, subprocess
from PIL import Image
root=pathlib.Path(__file__).resolve().parents[1];os.chdir(root)
out=root/'build';out.mkdir(exist_ok=True)
files=['tests/render.c','GUI/GUI_RU.c','GUI/calendar.c','GUI/fonts_ru.c','GUI/Adafruit_GFX.c','GUI/u8g2_font.c']
for heap in (2800,8192):
 subprocess.run(['gcc','-std=c99','-Wall','-Wextra','-Werror','-fno-pie','-no-pie','-fsanitize=address,undefined','-IGUI','-DCALENDAR_LAYOUT_CHECK',f'-D__HEAP_SIZE={heap}',*files,'-o',str(out/f'render{heap}')],check=True)
dates=['2026-10-21','2026-01-09','2026-03-09','2026-05-11','2026-12-31','2027-01-01','2027-02-20','2028-02-29','2100-03-01']
dates+= [f'2026-{m:02d}-01' for m in range(1,13)]
preview=root/'docs/previews';preview.mkdir(exist_ok=True)
for date in dates:
 t=int(dt.datetime.fromisoformat(date).replace(tzinfo=dt.timezone.utc).timestamp())
 for color in (1,2,3):
  contents=[]
  for heap in (2800,8192):
   path=out/f'frame{heap}.ppm'
   subprocess.run([str(out/f'render{heap}'),str(t),str(color),str(path)],check=True)
   contents.append(path.read_bytes())
  assert contents[0]==contents[1], (date,color,'page seams')
  if date in dates[:9]:
   Image.open(out/'frame2800.ppm').save(preview/(f'{date}-'+{1:'bw',2:'bwr',3:'bwry'}[color]+'.png'))
# Unsynchronised reset and the upstream large screen geometries.
for w,h in [(600,448),(648,480),(800,480),(640,384),(880,528)]:
 for c in (1,2,3):
  subprocess.run([str(out/'render2800'),'1792540800',str(c),str(out/'large.ppm'),str(w),str(h)],check=True)
subprocess.run([str(out/'render2800'),'1735689600','2',str(out/'unsynced.ppm'),'400','300','0'],check=True)
Image.open(out/'unsynced.ppm').save(preview/'unsynchronised.png')
subprocess.run([str(out/'render2800'),'1792540800','2',str(out/'clock.ppm'),'400','300','1','2'],check=True)
print(f'{len(dates)*3*2+17} ASan/UBSan renderings passed; two MCU heap sizes produce identical pixels')
