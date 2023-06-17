all_core_dir := compat          \
                audio_output 	\
				config 		 	\
				input			\
				misc			\
				modules			\
				network			\
				playlist		\
				stream_output	\
				text			\
				video_output

#all_core_dir := misc

default: all
all clean distclean install:
	for d in $(all_core_dir); \
	do \
		$(MAKE) --directory=$$d $@;\
	done


