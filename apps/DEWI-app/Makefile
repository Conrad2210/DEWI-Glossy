CONTIKI_PROJECT = dewi-app
all: $(CONTIKI_PROJECT)

PROJECT_SOURCEFILES += lwb-print.c common.c switch.c host.c server.c lamp.c
CFLAGS += -DRF_CHANNEL=11 -DLWB_CONF_HSLP=0 -DTX_POWER=31 -DLWB_CUSTOM_CONF_H=\"lwb-custom-conf.h\"

PROJECTDIRS += .

CONTIKI = ../..
include $(CONTIKI)/Makefile.include

