all: $(TARGET)

$(TARGET): $(OBJFILES) $(OBJFILES_CPP)
	$(AR) $@ $^
	$(RANLIB) $@

$(OBJFILES): %.o: %.c $(HEADERS) 
	@echo Compiling $@ from $<..
	$(CC) -std=c99 $(CPP_FLAGS) -c -o $@ $<

$(OBJFILES_CPP): %.o: %.cpp $(HEADERS) 
	@echo Compiling $@ from $<..
	$(CPP) -std=c++11 $(CPP_FLAGS) -c -o $@ $<

install:
	mkdir -p $(INSTALL_LIB_DIR)
	install -m 0755 $(TARGET) $(INSTALL_LIB_DIR)

clean:
	@rm -f $(TARGET) $(OBJFILES) $(OBJFILES_CPP)

distclean: clean
	@rm -f $(INSTALL_LIB_DIR)/$(TARGET)
