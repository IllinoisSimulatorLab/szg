ALL = \
  AppFeatures.html \
  Architecture.html \
  arSZGClient.html \
  Bugs.html \
  ChangeLog.html \
  ClusterMode.html \
  ClusterTest.html \
  CommLayer.html \
  CommunicationDetails.html \
  CompileCPP.html \
	CompilerIdiosyncrasies.html \
  ExampleCode.html \
  GettingSoftware.html \
  Goals.html \
  GraphicsConfigExamples.html \
  GraphicsConfiguration.html \
  index.html \
  InputClasses.html \
  InputDeviceConfiguration.html \
  InputDevices-Drivers.html \
  InputDevices.html \
  InputFilters.html \
  InputSimulator.html \
  Interaction.html \
  Navigation.html \
  3DObjectFiles.html \
  PathConfiguration.html \
  PForth.html \
  PortabilityLayer.html \
  Programming.html \
  PythonBindings.html \
  QuickPort.html \
  RandomNumbers.html \
  Sound.html \
  Standalone.html \
  SupportingLibraries.html \
  SystemConfiguration.html \
  SyzygyEnvironmentVariables.html \
  szgview.html \
  Testing.html \
	TrackerCoordConversion.html \
  VirtCompConfiguration.html \
  VisualStudioBuildVars.html \
	Vrtest.html \
  UsingDocs.html

all: $(ALL)

ifeq ($(strip $(PYTHONEXE)),)
  PYTHONEXE=python
endif

%.html : ../docsrc/%.t2t t2t.rc
	$(PYTHONEXE) txt2tags.py -t html -C t2t.rc -o $@ $<
