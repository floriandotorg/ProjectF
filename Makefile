TARGETS = asm os sim
CP = cp
RM=rm

all: image

.PHONY: image
image: $(TARGETS)
	$(CP) os/flash.bin sim
    
.PHONY: $(TARGETS)
$(TARGETS):
	$(MAKE) -C $@;
    
.PHONY: clean    
clean:
	$(MAKE) clean -C asm;
	$(MAKE) clean -C os;
	$(MAKE) clean -C sim;
	$(RM) -f sim/flash.bin;
