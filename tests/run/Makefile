home: sys
	mkdir -p home/test
	cp ../../src/sys/init1.out home/test/init1.out

home.img: home
	./build-home.sh

sys:
	$(MAKE) -C ../../src/sys all

clean:
	rm -rf *.o *.img *.out *.s home
