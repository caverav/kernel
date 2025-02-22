SOURCEDIR := $(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))
ROOTDIR := $(shell dirname $(SOURCEDIR))

EXTLIBDIR := $(ROOTDIR)/extlibs
LIMINEDIR = $(EXTLIBDIR)/limine
ILARDIR = $(EXTLIBDIR)/ilar

BACKGROUND = $(SOURCEDIR)/misc/bg.bmp
TERMFONT = $(SOURCEDIR)/misc/unifont.bin
SFNFONT = $(SOURCEDIR)/misc/unifont.sfn

INITRD := $(SOURCEDIR)/initrd.img.gz
KERNEL := $(SOURCEDIR)/kernel.elf

ISO = $(ROOTDIR)/image.iso
DISK0 = $(ROOTDIR)/disk0.img
DISK1 = $(ROOTDIR)/disk1.img

LOGFILE = $(ROOTDIR)/log.txt

VERSION = 0
NET_DEBUG = 1
LVL5_PAGING = 0

CC = clang
CPP = clang++
LD = ld.lld
ASM = nasm

CXXFILT = llvm-cxxfilt

LIMINE = $(LIMINEDIR)/limine-deploy
ILAR = $(ILARDIR)/bin/ilar
TAR = tar
XORRISO = xorriso
QEMU = qemu-system-x86_64

QEMUFLAGS = -M q35 -serial stdio \
	-cpu max -smp 4 -m 512M \
	-boot d -rtc base=localtime \
	-drive format=raw,file=$(DISK0) \
	-device piix3-ide,id=ide \
	-drive id=disk,file=$(DISK1),format=raw,if=none \
	-device ide-hd,drive=disk,bus=ide.0 \
	-audiodev id=audio,driver=alsa \
	-machine pcspk-audiodev=audio \
	-net nic,model=rtl8139 -net user,hostfwd=tcp::1234-:1234
#	-net nic -net tap,ifname=tap0,script=no

XORRISOFLAGS = -as mkisofs -b limine-cd.bin \
		-no-emul-boot -boot-load-size 4 -boot-info-table \
		--efi-boot limine-cd-efi.bin -efi-boot-part \
		--efi-boot-image --protective-msdos-label

CFLAGS = -Ofast -pipe -Werror -Wall -Wextra \
	-DGIT_VERSION=\"$(shell git rev-parse --short HEAD)\" \
	-DKERNEL_VERSION=\"$(VERSION)\" -DNET_DEBUG=$(NET_DEBUG) -DLVL5_PAGING=$(LVL5_PAGING) -g

CPPFLAGS = $(CFLAGS) -Wno-c99-designator -Wno-unused-parameter -Wno-deprecated-volatile -Wno-register

LDFLAGS = 
ASMFLAGS = -f elf64 -g

INCLUDES := \
	-I$(SOURCEDIR)/ \
	-I$(SOURCEDIR)/wrappers/ \
	-I$(EXTLIBDIR)/limine/ \
	-I$(EXTLIBDIR)/printf/src/ \
	-I$(EXTLIBDIR)/lai/include/ \
	-I$(EXTLIBDIR)/cwalk/include/ \
	-I$(EXTLIBDIR)/scalable-font2/ \
	-I$(EXTLIBDIR)/cxxshim/stage2/include/ \
	-I$(EXTLIBDIR)/cxxshim/std-coroutine/include/

override INTERNALLDFLAGS :=  \
	-T$(SOURCEDIR)/linker.ld \
	-nostdlib                \
	-zmax-page-size=0x1000   \
	-static

override INTERNALCFLAGS :=     \
	-target x86_64-pc-none-elf \
	-std=gnu17                 \
	-ffreestanding             \
	-fno-exceptions            \
	-fno-stack-protector       \
	-fno-omit-frame-pointer    \
	-fno-rtti                  \
	-fno-pic                   \
	-mabi=sysv                 \
	-mno-80387                 \
	-mno-mmx                   \
	-mno-3dnow                 \
	-mno-sse                   \
	-mno-sse2                  \
	-mno-red-zone              \
	-mcmodel=kernel            \
	$(INCLUDES)

override INTERNALCPPFLAGS :=   \
	-target x86_64-pc-none-elf \
	-std=gnu++20               \
	-ffreestanding             \
	-fno-exceptions            \
	-fno-stack-protector       \
	-fno-omit-frame-pointer    \
	-fno-rtti                  \
	-fno-pic                   \
	-mabi=sysv                 \
	-mno-80387                 \
	-mno-mmx                   \
	-mno-3dnow                 \
	-mno-sse                   \
	-mno-sse2                  \
	-mno-red-zone              \
	-mcmodel=kernel            \
	$(INCLUDES)

override CFILES := $(shell find $(SOURCEDIR)/ -type f -name '*.c' -not -path "$(SOURCEDIR)/initrd/*") \
	$(shell find $(EXTLIBDIR)/lai/ -type f -name '*.c' 2> /dev/null) \
	$(shell find $(EXTLIBDIR)/cwalk/src/ -type f -name '*.c' 2> /dev/null)

override CPPFILES := $(shell find $(SOURCEDIR)/ -type f -name '*.cpp' -not -path "$(SOURCEDIR)/initrd/*")
override ASMFILES := $(shell find $(SOURCEDIR)/ -type f -name '*.asm' -not -path "$(SOURCEDIR)/initrd/*")

override OBJ = $(CFILES:.c=.o)
override OBJ += $(CPPFILES:.cpp=.o)
override OBJ += $(ASMFILES:.asm=_asm.o)

.PHONY: all
all: $(KERNEL) initrd
	$(MAKE) iso
	$(MAKE) clean run

.PHONY: bios
bios: $(KERNEL) initrd
	$(MAKE) iso
	$(MAKE) clean run-bios

.PHONY: vnc
vnc: $(KERNEL) initrd
	$(MAKE) iso
	$(MAKE) clean run-vnc

$(KERNEL):
	$(MAKE) $(OBJ)
	@printf "LD\t%s\n" $(KERNEL:$(ROOTDIR)/%=%)
	$(LD) $(LDFLAGS) $(INTERNALLDFLAGS) $(OBJ) -o $@

%.o: %.c
	@printf "CC\t%s\n" $(<:$(ROOTDIR)/%=%)
ifndef NOUBSAN
	$(CC) $(CFLAGS) $(INTERNALCFLAGS) -fsanitize=undefined -c $< -o $@
else
	$(CC) $(CFLAGS) $(INTERNALCFLAGS) -c $< -o $@
endif

%.o: %.cpp
	@printf "CPP\t%s\n" $(<:$(ROOTDIR)/%=%)
ifndef NOUBSAN
	$(CPP) $(CPPFLAGS) $(INTERNALCPPFLAGS) -fsanitize=undefined -c $< -o $@
else
	$(CPP) $(CPPFLAGS) $(INTERNALCPPFLAGS) -c $< -o $@
endif

%_asm.o: %.asm
	@printf "ASM\t%s\n" $(<:$(ROOTDIR)/%=%)
	$(ASM) $(ASMFLAGS) $< -o $@

.PHONY: initrd
initrd:
ifndef USEILAR
	@printf "TAR\t%s\n" $(INITRD:$(ROOTDIR)/%=%)
	$(TAR) czf $(INITRD) -C $(SOURCEDIR)/initrd/ ./ --format=ustar
else
	@printf "ILAR\t%s\n" $(INITRD:$(ROOTDIR)/%=%)
	$(ILAR) create $(INITRD) $(SOURCEDIR)/initrd/*
endif

iso:
	cp $(KERNEL) $(BACKGROUND) $(TERMFONT) $(SFNFONT) $(INITRD) $(ROOTDIR)/limine.cfg $(LIMINEDIR)/limine.sys \
		$(LIMINEDIR)/limine-cd.bin $(LIMINEDIR)/limine-cd-efi.bin $(ROOTDIR)/iso_root/

	@printf "XORRISO\t%s\n" $(ISO:$(ROOTDIR)/%=%)
	$(XORRISO) $(XORRISOFLAGS) $(ROOTDIR)/iso_root -o $(ISO) 2> /dev/null || echo "\e[31mFailed to build iso!\e[0m"

	@printf "LIMINE\t%s\n" $(ISO:$(ROOTDIR)/%=%)
	$(LIMINE) $(ISO) 2> /dev/null || echo "\e[31mFailed to install Limine!\e[0m"

clean:
ifndef NOCLEAN
	rm -rf $(KERNEL) $(OBJ) $(INITRD) $(ROOTDIR)/iso_root/*
endif

run:
ifndef NORUN
	@echo "\nBooting iso in uefi mode...\n"
ifndef DEBUG
	$(QEMU) $(QEMUFLAGS) -enable-kvm -cdrom $(ISO) -drive if=pflash,format=raw,unit=0,file=$(ROOTDIR)/ovmf/OVMF_CODE-pure-efi.fd,readonly=on -drive if=pflash,format=raw,unit=1,file=$(ROOTDIR)/ovmf/OVMF_VARS-pure-efi.fd | $(CXXFILT)
else
	$(QEMU) $(QEMUFLAGS) -no-reboot -no-shutdown -d int -D $(LOGFILE) -machine smm=off -monitor telnet:127.0.0.1:12345,server,nowait -cdrom $(ISO) -drive if=pflash,format=raw,unit=0,file=$(ROOTDIR)/ovmf/OVMF_CODE-pure-efi.fd,readonly=on -drive if=pflash,format=raw,unit=1,file=$(ROOTDIR)/ovmf/OVMF_VARS-pure-efi.fd | $(CXXFILT)
endif
endif

run-bios:
ifndef NORUN
	@echo "\nBooting iso in bios mode...\n"
ifndef DEBUG
	$(QEMU) $(QEMUFLAGS) -enable-kvm -cdrom $(ISO) | $(CXXFILT)
else
	$(QEMU) $(QEMUFLAGS) -no-reboot -no-shutdown -d int -D $(LOGFILE) -machine smm=off -monitor telnet:127.0.0.1:12345,server,nowait -cdrom $(ISO) | $(CXXFILT)
endif
endif

run-vnc:
ifndef NORUN
	@echo "\nBooting iso in bios mode... (VNC)\n"
ifndef DEBUG
	$(QEMU) $(QEMUFLAGS) -vnc 0.0.0.0:1 -cdrom $(ISO) | $(CXXFILT)
else
	$(QEMU) $(QEMUFLAGS) -vnc 0.0.0.0:1 -no-reboot -no-shutdown -d int -D $(LOGFILE) -machine smm=off -monitor telnet:127.0.0.1:12345,server,nowait -cdrom $(ISO) | $(CXXFILT)
endif
endif