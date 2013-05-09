TARGETS = asm os sim
CP = cp

all: $(TARGETS)

.PHONY: image
image: all
	$(CP) os/flash.bin sim
    
.PHONY: $(TARGETS)
$(TARGETS):
	$(MAKE) -C $@;
