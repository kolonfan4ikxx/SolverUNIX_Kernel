# Kolon Kiwi - общие константы.
# Читаются ассемблером (.include) и tools/mkkk.py (разбирает строки ".set NAME, VALUE").
.set KK_STAGE2_LBA,      1        # stage2 лежит сразу после MBR
.set KK_STAGE2_SECTORS,  24       # 12 KiB максимум
.set KK_STAGE2_ADDR,     0x8000   # куда stage1 грузит stage2
.set KK_OSTBL_LBA,       64       # сектор с таблицей ОС (есть на каждом диске с ОС)
.set KK_KERNEL_LBA,      128      # куда mkkk кладёт ядро (ELF64 как есть)
