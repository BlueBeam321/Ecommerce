all_project_dir := 	$(CORE_DIR) 	\
					$(MODULE_DIR)	\
					$(LIBVLC_DIR)

#all_project_dir := $(CORE_DIR)
#all_project_dir := $(MODULE_DIR)
#all_project_dir := $(LIBVLC_DIR)

default: all
all clean distclean install:
	for d in $(all_project_dir); \
	do \
		$(MAKE) --directory=$$d $@;\
	done
	
