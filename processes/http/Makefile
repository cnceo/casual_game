objects = http_callback.o\
		  http_parse.o\
		  url_en_decode.o\
		  http.o\
		  main.o\



CC = g++
cc = g++

cc_flags = $(CPP_STD_VER) -ggdb -Wall -Werror \
		   $(optimization_flag)\
		   $(google_protobuf_cc_flag)\
		   $(base_cc_flag)\

ln_flags = -Wl,-dn\
		   -L$(PROTOCOL_DIR)/protobuf -lprotobuf_message\
		   $(base_ln_flag)\
		   $(google_protobuf_ln_flag)\
		   -Wl,-dy\
		   -lpthread\

target_ln_dependents = $(dependent_water_files)\
					   $(PROTOCOL_DIR)/protobuf/libprotobuf_message.a

target = http_exec

.PHONY:
all: $(target)

$(target) : $(objects) $(target_ln_dependents)
	@#此行用于调试   @set -e -x; \ #此行用于调试
	g++ $(objects) $(ln_flags) -o $(target) 

%.o: %.cpp
	$(cc) $(cc_flags) -c $< -o $@

%.d: %.cpp
	@#此行用于调试	@set -e -x; \ #此行用于调试
	@rm -f $@; \
		$(cc) $(cc_flags) -MM $< > $@.$$$$;  \
		sed -r 's,^(.*)\.o:,$@ $*\.o:,' $@.$$$$ > $@;  \
		rm -f $@.$$$$

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(MAKECMDGOALS),distclean)
-include $(objects:.o=.d)
endif
endif

.PHONY: clean distclean
clean:
	rm -f $(objects) $(objects:.o=.d) *.d *.d.* 
	rm -f $(target)

distclean: clean
