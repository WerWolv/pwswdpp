TOPDIR ?= $(CURDIR)

LINUX_KERNEL_PATH   := /mnt/d/_Dev/_RG350/RG350_linux/
CROSS_COMPILER_PATH := /mnt/d/_Dev/_RG350/buildroot-2020.05-rc1/buildroot-2020.05-rc1/output/host/bin/
PREFIX 				:= mipsel-linux

CROSS_GCC 			:= $(CROSS_COMPILER_PATH)$(PREFIX)-gcc
CROSS_G++ 			:= $(CROSS_COMPILER_PATH)$(PREFIX)-g++
CROSS_OBJCOPY 		:= $(CROSS_COMPILER_PATH)$(PREFIX)-objcopy
CROSS_LD 			:= $(CROSS_COMPILER_PATH)$(PREFIX)-g++


TOPDIR ?= $(CURDIR)

BUILD			:=	build
SOURCES_TOP		:=	source
SOURCES			+=  $(foreach dir,$(SOURCES_TOP),$(shell find $(dir) -type d 2>/dev/null))
INCLUDES		:=	include

ARCH		:=

null      	:=
SPACE     	:=  $(null) $(null)

DEFINES 	+=

COMMONFLAGS	:=  -g -O3 -ffunction-sections -fPIE \
				$(ARCH) $(DEFINES) 				 \
				$(INCLUDE) -static -lstdc++ -lc -lpthread -lm -I$(LINUX_KERNEL_PATH)include -I$(LINUX_KERNEL_PATH)arch/mips/include -I$(LINUX_KERNEL_PATH)arch/mips/include/asm/mach-generic

CFLAGS		:=	$(COMMONFLAGS)

CPPFLAGS	:=  $(COMMONFLAGS) -std=gnu++2a

ASFLAGS		:=	-g $(ARCH)
LDFLAGS		:=	-g $(COMMONFLAGS)


ifneq ($(BUILD),$(notdir $(CURDIR)))

export TOPDIR	:=	$(CURDIR)

export VPATH	:=	$(foreach dir,$(SOURCES),$(CURDIR)/$(dir)) \
					$(foreach dir,$(DATA),$(CURDIR)/$(dir))

export DEPSDIR	:=	$(CURDIR)/$(BUILD)

CFILES			:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
SFILES			:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))


export OFILES 		:=	$(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(SFILES:.s=.o)

export INCLUDE		:=	$(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
						$(foreach dir,$(LIBDIRS),-I$(dir)/include) \
						-I$(CURDIR)/$(BUILD)


.PHONY: $(BUILD) clean all

all: $(BUILD)

$(BUILD):
	@[ -d $@ ] || mkdir -p $@ $(BUILD)
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

clean:
	rm -fr $(BUILD) $(TOPDIR)/pwswdpp.elf

else

.PHONY:	all

DEPENDS	:=	$(OFILES:.o=.d)

all: $(TOPDIR)/pwswdpp.elf

$(TOPDIR)/pwswdpp.elf: $(OFILES)

%.o: %.cpp
	@mkdir -p $(@D)
	$(CROSS_G++) -MMD -MP -MF $*.d $(CPPFLAGS) -c $< -o $@

%.o: %.c
	@mkdir -p $(@D)
	$(CROSS_GCC) -MMD -MP -MF $*.d $(CFLAGS) -c $< -o $@

%.o: %.s
	@mkdir -p $(@D)
	$(CROSS_GCC) -MMD -MP -MF $*.d -x assembler-with-cpp $(ASFLAGS) -c $< -o $@

%.elf:
	$(CROSS_LD) $(LDFLAGS) $(OFILES) -o $@

-include $(DEPENDS)

endif