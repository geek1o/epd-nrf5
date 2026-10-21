#!/usr/bin/env python3
"""Generate and verify application-only candidates for the upstream secure DFU bootloaders.
This does not establish compatibility with a seller's bootloader or program a device.
"""
import argparse, hashlib, pathlib, zipfile
from nordicsemi.dfu.package import Package
from nordicsemi.dfu.signing import Signing
from nordicsemi.dfu.init_packet_pb import InitPacketPB
p=argparse.ArgumentParser()
p.add_argument('--chip', choices=['51','52'], required=True)
p.add_argument('--application', required=True)
p.add_argument('--key', required=True)
p.add_argument('--out', required=True)
p.add_argument('--version',type=lambda s:int(s,0),default=0x1d)
a=p.parse_args()
signer=Signing();signer.load_key(a.key)
softdevice=0x126 if a.chip=='52' else 0x87
pkg=Package(hw_version=int(a.chip),app_version=a.version,sd_req=[softdevice],sd_id=[softdevice],app_fw=a.application,signer=signer)
pkg.generate_package(a.out)
with zipfile.ZipFile(a.out) as z:
    packet=InitPacketPB(from_bytes=z.read(next(n for n in z.namelist() if n.endswith('.dat'))))
    command=packet.packet.signed_command
    sig=command.signature
    assert signer.verify(packet.get_init_command_bytes(),sig[31::-1]+sig[63:31:-1])
    init=packet.init_command
    binary=z.read(next(n for n in z.namelist() if n.endswith('.bin')))
    assert init.hash.hash==hashlib.sha256(binary).digest()[::-1]
    assert init.app_size==len(binary) and init.hw_version==int(a.chip) and list(init.sd_req)==[softdevice]
print(f'Verified signature, SHA256, size and DFU identifiers: {a.out}')
