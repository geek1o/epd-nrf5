#!/usr/bin/env python3
"""Archive locally built applications with checksums and explicit compatibility limits."""
import datetime,hashlib,json,pathlib,subprocess,zipfile
root=pathlib.Path(__file__).resolve().parents[1]
revision=subprocess.check_output(['git','rev-parse','HEAD'],cwd=root,text=True).strip()
files={}; profiles=[]
for chip in ('51','52'):
 folder=root/f'build/nrf{chip}-ru'
 info=json.loads((folder/'application-info.json').read_text())
 assert info['source_commit']==revision, 'Build the current source commit before archiving'
 profiles.append(info)
 for name in ['application.hex','application.bin','application.elf','application.map','application.ld','application-info.json','application-ota.zip']:
  data=(folder/name).read_bytes(); files[f'nrf{chip}-ru/{name}']=data
for path in (root/'docs/previews').glob('*.png'): files['previews/'+path.name]=path.read_bytes()
for name in ['docs/flashing_ru.md','docs/holidays_ru.md','docs/validation_ru.md']:
 files[name]=(root/name).read_bytes()
for source,name in [('LICENSE','GPL-3.txt'),('tools/fonts/LICENSE-DejaVu.txt','DejaVu.txt'),('SDK/17.1.0_ddde560/modules/nrfx/LICENSE','Nordic-nrfx.txt'),('SDK/17.1.0_ddde560/external/segger_rtt/license/license.txt','SEGGER.txt')]:
 files['LICENSES/'+name]=(root/source).read_bytes()
for source,name in [('GUI/Adafruit_GFX.c','Adafruit.txt'),('GUI/u8g2_font.c','U8g2.txt'),('SDK/12.3.0_d7731ad/components/libraries/timer/app_timer.c','Nordic-SDK12.txt'),('SDK/17.1.0_ddde560/components/libraries/timer/app_timer2.c','Nordic-SDK17.txt')]:
 s=(root/source).read_text(encoding='utf-8-sig');end=s.find('*/')
 files['LICENSES/'+name]=(s[:end+2]+'\n').encode()
manifest={'source_repository':'https://github.com/geek1o/epd-nrf5','source_commit':revision,'created_utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),'hardware_tested':False,'ota_acceptance_confirmed':False,'profiles':profiles,'files':{name:{'bytes':len(data),'sha256':hashlib.sha256(data).hexdigest()} for name,data in sorted(files.items())}}
encoded=(json.dumps(manifest,ensure_ascii=False,indent=2)+'\n').encode()
files['manifest.json']=encoded
files['README_RU.txt']=(f'''Кандидаты русской прошивки EPD-nRF5. Исходный коммит: {revision}
Исходники: https://github.com/geek1o/epd-nrf5/tree/{revision}
Это приложение для конфигураций upstream, не выбранный образ для неизвестного Hema.
Физических испытаний и записи на устройство не было. Сначала прочитайте docs/flashing_ru.md.
HEX содержит только приложение; BIN не содержит адреса. Загрузчик и SoftDevice не заменяются.
OTA ZIP подписан исходным опубликованным ключом, но совместимость с вашим загрузчиком не подтверждена.
Проверьте SoC, память, панель, GPIO, SoftDevice, ключ/версию DFU и резервную копию до записи.
Изображения previews/ созданы самим C-рендерером; часы обновляются полным циклом раз в минуту.
Лицензии и исходные уведомления сохранены в LICENSES/ и в исходном репозитории.
''').encode()
out=root/'artifacts';out.mkdir(exist_ok=True)
archive=out/'epd-russian-calendar-candidates.zip'
with zipfile.ZipFile(archive,'w',zipfile.ZIP_DEFLATED,compresslevel=9) as z:
 for name,data in sorted(files.items()): z.writestr(name,data)
(out/'manifest.json').write_bytes(encoded)
(out/'SHA256SUMS').write_text('\n'.join(hashlib.sha256(p.read_bytes()).hexdigest()+'  '+p.name for p in [archive,out/'manifest.json'])+'\n')
print(f'Archived {len(files)} files, {archive.stat().st_size} bytes; source {revision}')
