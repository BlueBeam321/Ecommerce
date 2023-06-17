include $(HEADER)

CPP_FLAGS += -I$(LIBVLC_DIR) -I$(MODULE_DIR)/crypt -I$(CONTRIB_DIR)/libdvbpsi/include
LD_FLAGS += -lstreamoutput -lvideooutput -lvideofilter -lvideochroma -ltextrenderer -laudiooutput -laudiofilter -lmetaengine -ldemux -lmux -lcodec -lpacketizer -laccess
LD_FLAGS += -lstreamoutput_core -laudiooutput_core -lplaylist_core -lnetwork_core -lconfig_core -lmodules_core -linput_core -ltext_core -lvideooutput_core -lmisc_core -lcompat_core -lcrypt
LD_FLAGS += -lavformat -lavcodec -lswscale -lavutil -ldvbpsi -lliveMedia -lgroupsock -lUsageEnvironment -lBasicUsageEnvironment -lmatroska -lebml -lass -lfontconfig -lfribidi -lfreetype -lxml2 -liconv -lx264
LD_FLAGS += -landroid -lEGL -lGLESv2 -llog -lm -lz

TARGET = libvlc.so

include $(SHARED_LIB)
