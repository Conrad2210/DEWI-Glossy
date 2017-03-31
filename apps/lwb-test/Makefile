CONTIKI_PROJECT = lwb-test
all: $(CONTIKI_PROJECT)

PROJECT_SOURCEFILES += lwb-print.c
CFLAGS += -DRF_CHANNEL=26 -DLWB_CONF_HSLP=0 -DTX_POWER=31 -DLWB_CUSTOM_CONF_H=\"lwb-custom-conf.h\"

PROJECTDIRS += .

CONTIKI = ../..
include $(CONTIKI)/Makefile.include

