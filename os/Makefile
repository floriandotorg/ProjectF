RM=rm
FASM_PATH = ../asm/
FASM = $(FASM_PATH)fasm
SOURCE = os.fasm
IMAGE = flash.bin

all: $(IMAGE)

$(IMAGE): $(SOURCE)
	$(FASM) $(SOURCE) $(IMAGE)
    
clean:
	$(RM) -f $(IMAGE) 