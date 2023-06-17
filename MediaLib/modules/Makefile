all_module_dir := access \
				demux \
				packetizer \
				codec \
				meta_engine \
				video_chroma \
				video_output \
				audio_filter \
				audio_output \
				crypt 

#all_module_dir := audio_output

default: all
all clean distclean install:
	for d in $(all_module_dir); \
	do \
		$(MAKE) --directory=$$d $@;\
	done


