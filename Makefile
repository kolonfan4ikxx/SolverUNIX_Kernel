# SolverUNIX + загрузчик Kolon Kiwi.
# Ядро: настоящий ELF64, собирается x86_64-elf-gcc / x86_64-elf-ld. Никаких objcopy-преобразований.
# Для проверки на машине без кросс-компилятора можно временно: make CROSS=  (хостовый gcc/ld).
CROSS ?= x86_64-elf-
CC := $(CROSS)gcc
AS := x86_64-elf-as
LD := $(CROSS)ld

VERSION := $(shell sed -n 's/.*SOLVER_VERSION "\(.*\)"/\1/p' include/solver/kernel.h)
B := build

CFLAGS := -B C:/cross/gcc-bin/ -m64 -std=gnu11 -ffreestanding -O2 -Wall -Wextra -fno-pic -fno-pie -fno-stack-protector -fno-builtin -mno-red-zone -mno-mmx -mno-sse -mno-sse2 -mcmodel=small -Iinclude -I.
LDFLAGS := -nostdlib -static -z max-page-size=0x1000 -z noexecstack -T linker.ld

SRC_C := $(wildcard *.c)
SRC_S := $(wildcard *.S)

OBJ := $(addprefix $(B)/,$(SRC_C:.c=.o) $(SRC_S:.S=.o))

QEMU    ?= qemu-system-x86_64
QEMUOPT := -m 128M -serial stdio

all: solverunix.elf $(B)/boot/stage1.bin $(B)/boot/stage2.bin

.PHONY: toolcheck
toolcheck:
	@command -v $(CC) >/dev/null 2>&1 || { \
	  echo "ERROR: $(CC) not found. Install the x86_64-elf cross toolchain"; \
	  echo "       (or, for a quick check only: make CROSS=)"; exit 1; }

# ---- ядро (ELF64 как есть) ----
solverunix.elf: $(OBJ) linker.ld
	$(LD) $(LDFLAGS) -o $@ $(OBJ)

$(B)/%.o: %.c | toolcheck
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(B)/%.o: %.S | toolcheck
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# ---- загрузчик Kolon Kiwi (плоские бинарники 16/32 бит) ----
$(B)/boot/%.o: boot/%.s boot/kk.inc | toolcheck
	@mkdir -p $(dir $@)
	$(AS) --64 -I boot -o $@ $<

$(B)/boot/stage1.bin: $(B)/boot/stage1.o boot/boot.ld
	$(LD) -T boot/boot.ld --defsym=__org=0x7C00 --oformat binary -o $@ $<

$(B)/boot/stage2.bin: $(B)/boot/stage2.o boot/boot.ld
	$(LD) -T boot/boot.ld --defsym=__org=0x8000 --oformat binary -o $@ $<

# ---- образы дисков ----
solverunix.img: $(B)/boot/stage1.bin $(B)/boot/stage2.bin solverunix.elf tools/mkkk.py boot/kk.inc
	python3 tools/mkkk.py --stage1 $(B)/boot/stage1.bin --stage2 $(B)/boot/stage2.bin \
	    --kernel solverunix.elf --name "SolverUNIX $(VERSION)" -o $@

sample.img: solverunix.elf tools/mkkk.py
	python3 tools/mkkk.py --data-only --kernel solverunix.elf --name "This Is sample OS" -o $@

check: solverunix.elf
	python3 tools/kkcheck.py solverunix.elf
	readelf -hlW solverunix.elf | sed -n '/Class/p;/Type/p;/Machine/p;/Entry/p;/LOAD/p'

# ---- запуск в QEMU: настоящая загрузка с диска через BIOS -> Kolon Kiwi ----
run: solverunix.img
	$(QEMU) $(QEMUOPT) -drive file=solverunix.img,format=raw,if=ide,index=0

run-2disk: solverunix.img sample.img
	$(QEMU) $(QEMUOPT) -drive file=solverunix.img,format=raw,if=ide,index=0 \
	                   -drive file=sample.img,format=raw,if=ide,index=1

run-debug: solverunix.img
	$(QEMU) $(QEMUOPT) -drive file=solverunix.img,format=raw,if=ide,index=0 \
	        -d int,cpu_reset -D qemu.log -no-reboot -no-shutdown

# ---- альтернатива: GRUB (Multiboot2 понимает ELF64) ----
iso: solverunix.elf
	mkdir -p iso/boot/grub
	cp solverunix.elf iso/boot/
	printf 'menuentry "SolverUNIX" {\n  multiboot2 /boot/solverunix.elf\n  boot\n}\n' > iso/boot/grub/grub.cfg
	grub-mkrescue -o solverunix.iso iso

clean:
	rm -rf $(B) solverunix.elf solverunix.img sample.img iso solverunix.iso qemu.log

.PHONY: all check run run-2disk run-debug iso clean
