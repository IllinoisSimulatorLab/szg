buttonfly.jar: WindowInterface.java LightClient.java SyzygyInterface.java BasicWindowMonitor.java
	make clean
	javac WindowInterface.java
	jar cmvf manifest.mft $@ *.class

clean:
	rm -f *.class *.jar
