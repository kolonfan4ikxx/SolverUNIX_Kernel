#!/usr/bin/env python3
"""kkcheck - проверка ядра перед загрузкой Kolon Kiwi / GRUB (Multiboot2)."""
import struct, sys

LOAD_MIN, STAGE_BASE = 0x100000, 0x800000
MB2_MAGIC = 0xE85250D6

def fail(msg):
    print('FAIL:', msg); sys.exit(1)

d = open(sys.argv[1], 'rb').read()
if d[:4] != b'\x7fELF': fail('not an ELF file')
if d[4] != 2: fail('not ELFCLASS64 (kernel must be a real ELF64)')
if d[5] != 1: fail('not little-endian')
etype, mach, _, entry, phoff, _, _, _, phentsize, phnum = struct.unpack_from('<HHIQQQIHHH', d, 16)
if etype != 2: fail('e_type is not ET_EXEC')
if mach != 62: fail('e_machine is not EM_X86_64')
if entry >= 1 << 32: fail('entry point >= 4 GiB (Multiboot2 entry must be 32-bit code)')
print('ELF64 x86-64 ET_EXEC, entry 0x%x, %d program headers' % (entry, phnum))

loads, ok_entry = [], False
for i in range(phnum):
    t, fl, off, va, pa, fs, ms, al = struct.unpack_from('<IIQQQQQQ', d, phoff + i * phentsize)
    if t != 1: continue
    loads.append((off, va, pa, fs, ms, fl))
    print('  LOAD off=0x%06x vaddr=0x%08x paddr=0x%08x filesz=0x%06x memsz=0x%06x flags=%s%s%s' %
          (off, va, pa, fs, ms, 'R' if fl & 4 else '-', 'W' if fl & 2 else '-', 'X' if fl & 1 else '-'))
    if pa < LOAD_MIN: fail('segment below 1 MiB')
    if pa + ms > STAGE_BASE: fail('segment reaches Kolon Kiwi staging area (8 MiB)')
    if fs > ms: fail('filesz > memsz')
    if off + fs > len(d): fail('segment extends past end of file')
    if fl & 1 and pa <= entry < pa + ms: ok_entry = True
if not loads: fail('no PT_LOAD segments')
if not ok_entry: fail('entry point is not inside an executable PT_LOAD segment')

hdr = None
for off in range(0, min(len(d), 32768) - 16, 8):
    m, arch, length, csum = struct.unpack_from('<IIII', d, off)
    if m == MB2_MAGIC:
        if (m + arch + length + csum) & 0xFFFFFFFF: fail('bad Multiboot2 header checksum')
        hdr = off; break
if hdr is None: fail('Multiboot2 header not found in first 32 KiB, 8-byte aligned')
print('Multiboot2 header at file offset 0x%x, checksum OK' % hdr)
print('OK')
