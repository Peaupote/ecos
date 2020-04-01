ROOT=../..

.PHONY:clean sys home

home: sys
	mkdir -p home/test
	cp $(ROOT)/src/sys/init1.out home/test/init1.out

home.img: home
	$(ROOT)/tools/build-ext2.sh home home.img

sys:
	$(MAKE) -C $(ROOT)/src/sys all

clean:
	rm -rf *.o *.img *.out *.s home
	$(MAKE) -C prg clean
