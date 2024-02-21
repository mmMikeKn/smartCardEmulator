# ��������� ������� ������� �������
ROOT_DIR=$(SRC_DIR)/..
OUT_DIR=$(ROOT_DIR)/out
LIB_DIR=$(ROOT_DIR)/lib
DOC_DIR=$(ROOT_DIR)/doc
SCRIPT_DIR=$(ROOT_DIR)/scripts

# ����������� ������
TOOLS_VARIIANT=-kgp-eabi
CC      = arm$(TOOLS_VARIIANT)-gcc
CXX     = arm$(TOOLS_VARIIANT)-g++
FC      = arm$(TOOLS_VARIIANT)-gfortran
LD      = arm$(TOOLS_VARIIANT)-ld
AR      = arm$(TOOLS_VARIIANT)-ar
AS      = arm$(TOOLS_VARIIANT)-as
CP      = arm$(TOOLS_VARIIANT)-objcopy
OD		= arm$(TOOLS_VARIIANT)-objdump
SZ      = arm$(TOOLS_VARIIANT)-size
SR      = arm$(TOOLS_VARIIANT)-strip
GDB     = arm$(TOOLS_VARIIANT)-gdb
RM		= rm
TAR     = tar
TOUCH   = touch

TARGET=TE_STM32F103

F_OCS=8000000
LD_SCRIPT=stm32f103c8t6_rom.ld
FLASHSIZE=65536
RAMSIZE=20480

HARDWARE_DEFS=-D$(TARGET) -DF_OCS=$(F_OCS) 

# ���������
CPU=cortex-m3

#�������� � crt ���� ��� gdb
DELAY_FOR_GDB=1
APP_DEFS=-DDELAY_FOR_GDB=$(DELAY_FOR_GDB)

# �������� ������
DATE		= $$(date +%Y%m%d)
PKG_NAME	= $(LIBNAME)$(PRJNAME)

# ����� ������������
ARFLAGS = -rcs

CPFLAGS = -O ihex
ODFLAGS	= -x --syms

# ��������������
COMPILE_FLAGS=-W -Wall -Wno-unused-parameter 
# �����������
COMPILE_FLAGS+=-Os -finline-functions -fomit-frame-pointer -ffunction-sections -fdata-sections -funroll-loops -fgraphite
# ������� 
COMPILE_DEBUG_FLAGS=-ggdb3
COMPILE_FLAGS+=$(COMPILE_DEBUG_FLAGS)

# ���������
COMPILE_CPU_FLAGS=-mcpu=$(CPU) -mfloat-abi=soft -mlittle-endian
COMPILE_FLAGS+= -mtune=$(CPU) $(COMPILE_CPU_FLAGS)
# ������ �����
#COMPILE_FLAGS+=-mlong-calls
# �����
COMPILE_FLAGS+=-mthumb
# ����� ����������
COMPILE_FLAGS+=-Wa,-adhlns=$(<:.c=.lst)
# ��������� ������������
COMPILE_FLAGS+=-Wp,-M,-MP,-MT,$(*F).o,-MF,.dep/$(@F).dep

# ����� ���������� ���������� �� ����� C
CFLAGS =   $(COMPILE_FLAGS) -Wa,-adhlns=$(<:.c=.lst) 
# ������ �������� ����� �
CFLAGS+= -std=gnu99


LDFLAGS= -T $(SCRIPT_DIR)/$(LD_SCRIPT)  -nostartfiles  -mcpu=$(CPU) -mthumb  -mfpu=fpa -Wl,-gc-sections -L$(LIB_DIR)
#--warn-common

# ����� ���������������
ASFLAGS = $(COMPILE_CPU_FLAGS) $(COMPILE_DEBUG_FLAGS) -mapcs-32 -adhlns=$(<:.s=.lst) $(CRT_CONFIG)

