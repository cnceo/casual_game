objects = \
		  main.o\
          lobby.o\
		  deck.o\
          room.o\
          game13.o\
		  client_manager.o\
		  redis_handler.o\
		  register_timer_handler.o\
		  register_tcp_msg_handler.o\



CC = g++
cc = g++

cc_flags = $(CPP_STD_VER) -ggdb -Wall -Werror \
		   $(optimization_flag)\
		   $(google_protobuf_cc_flag)\
		   $(base_cc_flag)\
		   $(cpp_redis_cc_flag)\

ln_flags = -Wl,-dn\
		   -L$(PROTOCOL_DIR)/protobuf -lprotobuf_message\
		   -L$(PROTOCOL_DIR)/rawmsg -lrawmsg\
		   $(base_ln_flag)\
		   $(google_protobuf_ln_flag)\
		   $(cpp_redis_ln_flag)\
		   -Wl,-dy\
		   -lpthread\

target_ln_dependents = $(dependent_moudles)\
				$(PROTOCOL_DIR)/protobuf/libprotobuf_message.a\
				$(PROTOCOL_DIR)/rawmsg/librawmsg.a


target = lobby_exec

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
