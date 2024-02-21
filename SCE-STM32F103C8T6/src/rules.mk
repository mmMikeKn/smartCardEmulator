include $(SRC_DIR)/options.mk

ASSRCS = $(wildcard *.s */*.s)
CSRCS = $(wildcard *.c */*.c)
CXXSRCS = $(wildcard *.cpp */*.cpp)
FSRCS = $(wildcard *.f */*.f)
HEADERS  = $(wildcard *.h */*.h)
DEPEND_LIBS = $(wildcard $(LIB_DIR)/*.stamp)

OBJS = $(ASSRCS:.s=.o)   $(FSRCS:.f=.o)   $(CSRCS:.c=.o)   $(CXXSRCS:.cpp=.o)   
LSTS = $(ASSRCS:.s=.lst) $(FSRCS:.f=.lst) $(CSRCS:.c=.lst) $(CXXSRCS:.cpp=.lst) 

# ������ ����������
%.a: $(OBJS)
	$(AR) $(ARFLAGS) $@ $^
	$(TOUCH) $@.stamp

# ������ ELF ���������
%.elf: $(OBJS) $(DEPEND_LIBS)
	$(CC) $(LDFLAGS) $(OBJS) $(LIBS) -o $@ -Wl,-Map,$(OUT_DIR)/$(APPNAME).map
	$(SZ) -B -t $(OUT_DIR)/$(APPNAME).elf | memutz $(FLASHSIZE) $(RAMSIZE)

# ��������� �������� ������
$(BINNAME): $(FULLNAME)
	$(CP) $(CPFLAGS) $(OUT_DIR)/$(APPNAME).elf $(OUT_DIR)/$(APPNAME).hex
	$(CP) -O elf32-littlearm -S $(OUT_DIR)/$(APPNAME).elf $(OUT_DIR)/$(APPNAME).bin
	$(OD) $(ODFLAGS) $(OUT_DIR)/$(APPNAME).elf > $(OUT_DIR)/$(APPNAME).dmp
	$(OD) -h -C -S $(OUT_DIR)/image.elf > $(OUT_DIR)/$(APPNAME).lss
#	$(SZ) -B -t -x $(OUT_DIR)/$(APPNAME).elf



# ���� ������
all: $(FULLNAME) $(BINNAME) $(MAKEFILE)

# �������
clean:
	$(RM) -f $(FULLNAME) $(OBJS) $(LSTS) $(CUSTOM_RM_FILES) .dep/*
#	$(RM) $(PKG_NAME)-$(DATE).tar.gz

# �������� ��������� ������������
dist: clean
	$(TAR) --exclude={CVS,cvs} -cvzf $(PKG_NAME)-$(DATE).tar.gz *

tools_version:
	$(CC)  -v
	$(LD)  -v
	$(GDB) -v

# phony targets
.PHONY: all clean depend

-include $(shell mkdir .dep 2>/dev/null) $(wildcard .dep/*)



