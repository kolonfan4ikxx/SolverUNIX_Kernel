#!/usr/bin/env python3
"""mkkk - собирает образ диска Kolon Kiwi.

Раскладка (сектора по 512 байт):
  LBA 0              stage1 (MBR)
  LBA 1 ..           stage2 (KK_STAGE2_SECTORS секторов)
  LBA 64             таблица ОС: "KKOSTBL1", count, записи по 128 байт
  LBA 128 ..         ядро: ELF64 КАК ЕСТЬ (загрузчик сам разбирает program headers)

Запись ОС (128 байт): name[64] | kernel_lba u32 | kernel_bytes u32 | cmdline[56]
"""
import argparse, os, re, struct, sys

SECTOR = 512

def consts(path):
    d = {}
    for line in open(path):
        m = re.match(r'\s*\.set\s+(\w+)\s*,\s*(0x[0-9a-fA-F]+|\d+)', line)
        if m:
            d[m.group(1)] = int(m.group(2), 0)
    return d

def main():
    here = os.path.dirname(os.path.abspath(__file__))
    ap = argparse.ArgumentParser()
    ap.add_argument('--stage1'); ap.add_argument('--stage2')
    ap.add_argument('--kernel', required=True)
    ap.add_argument('--name', required=True)
    ap.add_argument('--cmdline', default='')
    ap.add_argument('--data-only', action='store_true',
                    help='диск только с ОС (без загрузчика), для демонстрации "ОС на другом диске"')
    ap.add_argument('--size-mb', type=int, default=4)
    ap.add_argument('-o', '--out', required=True)
    a = ap.parse_args()

    c = consts(os.path.join(here, '..', 'boot', 'kk.inc'))
    s2_lba, s2_secs = c['KK_STAGE2_LBA'], c['KK_STAGE2_SECTORS']
    tbl_lba, k_lba = c['KK_OSTBL_LBA'], c['KK_KERNEL_LBA']

    kernel = open(a.kernel, 'rb').read()
    if kernel[:4] != b'\x7fELF' or kernel[4] != 2:
        sys.exit('mkkk: kernel is not an ELF64 file')

    need = k_lba * SECTOR + len(kernel)
    size = max(a.size_mb << 20, (need + (1 << 20) - 1) >> 20 << 20)
    img = bytearray(size)

    if not a.data_only:
        if not (a.stage1 and a.stage2):
            sys.exit('mkkk: --stage1 and --stage2 are required (or use --data-only)')
        s1 = open(a.stage1, 'rb').read()
        s2 = open(a.stage2, 'rb').read()
        if len(s1) != SECTOR or s1[510:512] != b'\x55\xaa':
            sys.exit('mkkk: stage1 must be exactly 512 bytes with 0x55AA signature')
        if len(s2) > s2_secs * SECTOR:
            sys.exit('mkkk: stage2 is %d bytes, limit is %d' % (len(s2), s2_secs * SECTOR))
        img[0:SECTOR] = s1
        img[s2_lba * SECTOR: s2_lba * SECTOR + len(s2)] = s2

    name = a.name.encode()[:63].ljust(64, b'\0')
    cmd = a.cmdline.encode()[:55].ljust(56, b'\0')
    entry = name + struct.pack('<II', k_lba, len(kernel)) + cmd
    assert len(entry) == 128
    table = (b'KKOSTBL1' + struct.pack('<II', 1, 0) + entry).ljust(SECTOR, b'\0')
    img[tbl_lba * SECTOR: tbl_lba * SECTOR + SECTOR] = table
    img[k_lba * SECTOR: k_lba * SECTOR + len(kernel)] = kernel

    open(a.out, 'wb').write(img)
    kind = 'data disk (no loader)' if a.data_only else 'bootable disk'
    print('mkkk: %s -> %s, %d KiB, OS "%s", kernel %d bytes @ LBA %d'
          % (kind, a.out, size >> 10, a.name, len(kernel), k_lba))

if __name__ == '__main__':
    main()
