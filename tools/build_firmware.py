#!/usr/bin/env python3
"""Build the upstream Keil application source lists with Arm GNU GCC (no bootloader)."""
import argparse, os, pathlib, subprocess, xml.etree.ElementTree as ET
ROOT = pathlib.Path(__file__).resolve().parents[1]
p = argparse.ArgumentParser()
p.add_argument('--chip', choices=['51', '52'], required=True)
p.add_argument('--locale', choices=['ru', 'zh', 'baseline'], default='ru')
p.add_argument('--out', default='build')
a = p.parse_args()
os.chdir(ROOT)
project = ET.parse(f'Keil/EPD-nRF{a.chip}.uvprojx').find('.//Target')
def path(s):
    return pathlib.Path(os.path.normpath('Keil/' + s.replace('\\', '/')))
incs = [path(s) for s in project.find('.//Cads/VariousControls/IncludePath').text.split(';')]
defs = project.find('.//Cads/VariousControls/Define').text.split()
sdk = pathlib.Path('SDK') / ('17.1.0_ddde560' if a.chip == '52' else '12.3.0_d7731ad')
src = [path(e.text) for e in project.findall('.//FilePath') if e.text.endswith('.c')]
src = [s for s in src if not ('Syscalls' in str(s) or 'Syscalls' in s.name or 'app_error_handler_keil' in str(s))]
if a.chip == '52':
    src += [sdk/'components/libraries/util/app_error_handler_gcc.c']
    startup = sdk/'modules/nrfx/mdk/gcc_startup_nrf52811.S'
    linkdir = sdk/'modules/nrfx/mdk'
else:
    src = [s for s in src if 'RTT_Syscalls_KEIL' not in str(s)]
    incs += [sdk/'components/device']
    startup = sdk/'components/toolchain/gcc/gcc_startup_nrf51.s'
    linkdir = sdk/'components/toolchain/gcc'
if a.locale != 'baseline':
    src += [pathlib.Path('tools/gcc_syscalls.c')]
    if a.locale == 'ru':
        src = [s for s in src if s.name not in ('GUI.c', 'Lunar.c', 'fonts.c')]
        src += list(map(pathlib.Path, ['GUI/GUI_RU.c', 'GUI/calendar.c', 'GUI/fonts_ru.c']))
    if a.chip == '51':
        defs = ['__HEAP_SIZE=2800' if d.startswith('__HEAP_SIZE=') else d for d in defs]
    defs += ['CALENDAR_RUSSIAN='+('1' if a.locale == 'ru' else '0')]
# GCC nano newlib; nRF51 heap is reduced to fit its larger static RAM use.
flags = ['-mcpu='+('cortex-m4' if a.chip == '52' else 'cortex-m0'), '-mthumb', '-mfloat-abi=soft', '-Os', '-g3', '-std=gnu99', '-ffunction-sections', '-fdata-sections', '-fno-strict-aliasing', '-fshort-enums', '-Wall', '-Wextra', '-Wno-unused-parameter', '-Wno-missing-field-initializers', '-Wno-expansion-to-defined']
flags += ['-I'+str(i) for i in incs] + ['-D'+d for d in defs]
out = ROOT/a.out/('nrf'+a.chip+'-'+a.locale)
out.mkdir(parents=True, exist_ok=True)
cc = os.environ.get('ARM_GCC', 'arm-none-eabi-gcc')
objects=[]
for i,s in enumerate(src+[startup]):
    obj=out/f'{i:03d}_{s.stem}.o'
    subprocess.run([cc,*flags,*(['-x','assembler-with-cpp'] if s.suffix.lower()=='.s' else []),'-c',str(s),'-o',str(obj)],check=True)
    objects.append(str(obj))
elf=out/'application.elf'
# SDK's original linker range extends through bootloader/FDS. Never emit a RU
# application overlapping the shipped bootloader or its two FDS pages.
linkscript=sdk/f'gcc_nrf{a.chip}.ld'
if a.locale != 'baseline':
    content=linkscript.read_text()
    content=content.replace('LENGTH = 0x17000','LENGTH = 0xE000') if a.chip=='52' else content.replace('LENGTH = 0x25000','LENGTH = 0x20400')
    linkscript=out/'application.ld'
    linkscript.write_text(content)

subprocess.run([cc,*flags,*objects,'--specs=nano.specs','--specs=nosys.specs','-L'+str(linkdir),'-T'+str(linkscript),'-Wl,--gc-sections,-Map='+str(out/'application.map'),'-Wl,--print-memory-usage','-o',str(elf)],check=True)
for fmt, ext in [('ihex','hex'),('binary','bin')]:
    subprocess.run([cc.replace('gcc','objcopy'),'-O',fmt,str(elf),str(out/f'application.{ext}')],check=True)
subprocess.run([cc.replace('gcc','size'),str(elf)],check=True)

nm = subprocess.check_output([cc.replace('gcc','nm'),'-n',str(elf)], text=True)
symbols = {line.split()[2]:int(line.split()[0],16) for line in nm.splitlines() if len(line.split())==3}
ram_origin = 0x200022d8 if a.chip=='52' else 0x20001ff8
static = symbols['__HeapBase']-ram_origin
heap = symbols['__HeapLimit']-symbols['__HeapBase']
stack = symbols['__StackTop']-symbols['__StackLimit']
print(f'RAM incl. alignment, heap and stack: {static}+{heap}+{stack} = {static+heap+stack} bytes')

import json, hashlib
image=(out/'application.bin').read_bytes()
limit=0xE000 if a.chip=='52' else 0x20400
if a.locale != 'baseline':
    assert len(image)<=limit
info={'chip_family':'nrf'+a.chip, 'locale':a.locale, 'application_origin':hex(0x19000 if a.chip=='52' else 0x1b000), 'flash_image_bytes':len(image), 'ram_application_including_heap_stack':static+heap+stack, 'ram_static_aligned':static, 'heap':heap, 'stack':stack, 'sha256_bin':hashlib.sha256(image).hexdigest(), 'compiler':subprocess.check_output([cc,'-dumpfullversion'],text=True).strip(), 'source_commit':subprocess.check_output(['git','rev-parse','HEAD'],text=True).strip(), 'hardware_tested':False}
(out/'application-info.json').write_text(json.dumps(info,indent=2)+'\n')
print(f'Flash image including data initializers: {len(image)} bytes')
